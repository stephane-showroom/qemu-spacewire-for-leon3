/* SPDX-License-Identifier: GPL-2.0-or-later */
/********************************************************************************
 *                                                                              *
 *    ▀▄   ▄▀                                                                   *
 *   ▄█▀███▀█▄    Created on: May 18, 2016                                      *
 *  █▀███████▀█   Author: Stephane GONZALEZ                                     *
 *  █ █▀▀▀▀▀█ █                                                                 *
 *     ▀▀ ▀▀                                                                    *
 *                                                                              *
 * Copyright (C) 2015-2026 Stephane GONZALEZ                                    *
 * Released under the GNU GPL v2 or later, see LICENSE                          *
 ********************************************************************************/

#include "qemu/osdep.h"
#include "grlib_comgrspw.h"
#include "grlib_typesgrspw.h"
#include "grlib_utilgrspw.h"
#include "grlib_reggrspw.h"
#include "grlib_mqgrspw.h"
#include "grlib_coregrspw.h"
#include "grlib_loggrspw.h"
#include "grlib_rmapgrspw.h"

#define C_MIN_PACKET_LEN    (2U)    ///< Smaller packets are discarded, except in promiscuous mode
#define C_MAX_RX_PENDING    (64U)   ///< Maximum number of packets waiting for a descriptor
#define C_RMAP_PROTOCOL     (0x01U) ///< RMAP protocol identifier

/**
 * \brief packet waiting for a RX descriptor (no spill mode)
 **/
typedef struct
{
  t_recv_att attributes;
  uint32 len;
  uint8 data[];
} t_grspw_rx_packet;

typedef enum
{
  e_rx_done,    ///< the packet has been stored or discarded
  e_rx_wait     ///< the packet has to wait for a descriptor
} t_grspw_rx_result;

uint8 g_txpacket[C_MAX_HEADER_LEN + C_MAX_DATA_LEN + 2U];   ///< header, data and their CRC

static boolean grlib_comgrspw_linkrun(const t_grspw_regptr grspw_regptr)
{
  return (grspw_regptr->str & C_SPWSTR_LS) == C_SPWSTR_LSRUN;
}

/*
 * \brief handle an AHB error of the DMA channel [M 77.4.9, 77.5.7]
 * \param [in/out] deviceptr     The grspw device
 * \param [in]     status        The status bit to set (RA or TA)
 * \param [in]     enable        The enable bit to clear (RE or TE)
 */
static void grlib_comgrspw_ahberror(t_grsw_deviceptr deviceptr, const uint32 status, const uint32 enable)
{
  t_grspw_regptr grspw_regptr = &deviceptr->grspw_reg;

  grspw_regptr->chn = setbit(grspw_regptr->chn, status);
  grspw_regptr->chn = clearbit(grspw_regptr->chn, enable);

  if (checkbit(grspw_regptr->chn, C_SPWCHN_AI))
  {
    qemu_irq_pulse(deviceptr->irq);
  }
}

/*
 * \brief end the transmission of the current TX descriptor
 * \param [in/out] deviceptr     The grspw device
 * \param [in]     link_error    e_true if the packet could not be transmitted
 */
static void grlib_comgrspw_txcomplete(t_grsw_deviceptr deviceptr, const boolean link_error)
{
  t_grspw_regptr grspw_regptr = &deviceptr->grspw_reg;
  t_grspw_tx_descrptr txd = &deviceptr->tx_descr;
  uint32 txd_cur = (grspw_regptr->txd & C_SPWTXD_TX_DESC_SELECT) >> 4;

  deviceptr->tx_pending = e_false;

  // [GRSPWSim-xxx] write the TX descriptor in order to disable the used descriptor.
  // The core writes the status: LE is cleared unless the packet ended by a link error
  txd->ctrl = clearbit(txd->ctrl, C_SPWTDW0_EN | C_SPWTDW0_LE);
  if (link_error)
  {
    txd->ctrl = setbit(txd->ctrl, C_SPWTDW0_LE);
  }
  if (!write_leon3_ram(grspw_regptr->txd, (uint8ptr) &txd->ctrl, sizeof(txd->ctrl), e_true))
  {
    // the descriptor is not written and stays enabled [M 77.5.7]
    grlib_comgrspw_ahberror(deviceptr, C_SPWCHN_TA, C_SPWCHN_TE);
    return;
  }

  // [GRSPWSim-107] inform a packet has been sent by setting the bit PS of the SPWCHN register
  grspw_regptr->chn = setbit(grspw_regptr->chn, C_SPWCHN_PS);

  // [GRSPWSim-110] check if the current tx descriptor force to wrap around
  // [GRSPWSim-111] otherwise increment and auto wrap the current tx descriptor
  txd_cur = checkbit(txd->ctrl, C_SPWTDW0_WR) ? 0 : (txd_cur + 1) % C_NB_TX_DESCRIPTOR;
  grspw_regptr->txd = deposit(grspw_regptr->txd, C_SPWTXD_TX_DESC_SELECT, txd_cur << 4);

  // [GRSPWSim-109] check if the interrupt is requested and if the interrupt controller is enable
  if (checkbit(txd->ctrl, C_SPWTDW0_IE) && checkbit(grspw_regptr->chn, C_SPWCHN_TI))
  {
    qemu_irq_pulse(deviceptr->irq);
  }
}

/*
 * \brief send the enabled TX descriptors on the bus
 * \param [in/out] deviceptr     The grspw device
 * \detail A packet stays pending until the peer acknowledges it, the guest is
 *         not blocked meanwhile. grlib_comgrspw_txack() then sends the next one.
 *         The caller holds the BQL.
 */
void grlib_comgrspw_txenable(t_grsw_deviceptr deviceptr)
{
  t_grspw_regptr grspw_regptr = &deviceptr->grspw_reg;
  t_grspw_tx_descrptr txd = &deviceptr->tx_descr;

  // [GRSPWSim-102] check if the transmission is enable
  while (checkbit(grspw_regptr->chn, C_SPWCHN_TE) && !deviceptr->tx_pending
      && grlib_comgrspw_linkrun(grspw_regptr))
  {
    uint32 data_len;
    uint32 header_len;
    uint32 packet_len;
    boolean crc_available = checkbit(grspw_regptr->ctr, C_SPWCTR_RC);

    // [GRSPWSim-103] read TX descriptor (base address tx descriptor + current descriptor)
    if (!read_leon3_ram(grspw_regptr->txd, (uint8ptr) txd, sizeof(*txd), e_true))
    {
      grlib_comgrspw_ahberror(deviceptr, C_SPWCHN_TA, C_SPWCHN_TE);
      return;
    }

    // [GRSPWSim-104] check if the Tx descriptor is enable
    if (!checkbit(txd->ctrl, C_SPWTDW0_EN))
    {
      // [GRSPWSim-111] otherwise set the TE bit of the SPWCHN to '0'
      grspw_regptr->chn = clearbit(grspw_regptr->chn, C_SPWCHN_TE);
      return;
    }

    data_len = txd->len & C_SPWTDW2_DATA_LENGTH;
    header_len = txd->ctrl & C_SPWTDW0_HEADER_LENGTH;

    // if both lengths are zero no packet is sent
    if (0 == data_len + header_len)
    {
      grlib_comgrspw_txcomplete(deviceptr, e_false);
      continue;
    }

    // [GRSPWSim-106] read & copy TX header buffer
    if (!read_leon3_ram(txd->addr_header, g_txpacket, header_len, e_false)) // straight copy without format
    {
      // nothing has been sent: the descriptor stays enabled, without error bit [M 77.5.7]
      grlib_comgrspw_ahberror(deviceptr, C_SPWCHN_TA, C_SPWCHN_TE);
      return;
    }
    packet_len = header_len;

    if (crc_available && checkbit(txd->ctrl, C_SPWTDW0_HC) && header_len > 0)
    {
      uint32 non_crc = MIN((txd->ctrl & C_SPWTDW0_NON_CRC_BYTES) >> 8, header_len);

      g_txpacket[packet_len++] = grlib_rmapgrspw_crc(&g_txpacket[non_crc], header_len - non_crc);
    }

    // [GRSPWSim-105] read & copy TX data buffer
    if (!read_leon3_ram(txd->data, &g_txpacket[packet_len], data_len, e_false)) // straight copy without format
    {
      grlib_comgrspw_ahberror(deviceptr, C_SPWCHN_TA, C_SPWCHN_TE);
      return;
    }

    if (crc_available && checkbit(txd->ctrl, C_SPWTDW0_DC))
    {
      g_txpacket[packet_len + data_len] = grlib_rmapgrspw_crc(&g_txpacket[packet_len], data_len);
      ++packet_len;
    }
    packet_len += data_len;

    // [GRSPWSim-107] send the message on the bus
    if (grlib_mqgrspw_send_spwpacket(deviceptr->grspw_id, C_MSGQ_DATA, NULL, 0, g_txpacket, packet_len))
    {
      deviceptr->tx_pending = e_true;
    }
    else
    {
      CHECK_WARNING(e_true, "Outgoing packet larger than the queue message size");
      grlib_comgrspw_txcomplete(deviceptr, e_true);
    }
  }
}

/*
 * \brief handle the acknowledge of the pending packet
 * \param [in/out] deviceptr     The grspw device
 */
void grlib_comgrspw_txack(t_grsw_deviceptr deviceptr)
{
  if (deviceptr->tx_pending)
  {
    grlib_comgrspw_txcomplete(deviceptr, e_false);
    grlib_comgrspw_txenable(deviceptr);
  }
}

/*
 * \brief abort the pending transmission (DMACTRL.AT) [M 77.5.7]
 * \param [in/out] deviceptr     The grspw device
 * \detail The packet is truncated: its descriptor is completed with LE. TE has
 *         already been cleared by the register write.
 */
void grlib_comgrspw_txabort(t_grsw_deviceptr deviceptr)
{
  if (deviceptr->tx_pending)
  {
    grlib_comgrspw_txcomplete(deviceptr, e_true);
  }
}

/*
 * \brief handle a link error: the link has left the run state [M 77.5.7]
 * \param [in/out] deviceptr     The grspw device
 * \detail The rest of the pending packet is discarded and its descriptor completed
 *         with LE. With DMACTRL.LE the transmitter is disabled, otherwise it goes on
 *         when the link is in the run state again.
 */
void grlib_comgrspw_linkerror(t_grsw_deviceptr deviceptr)
{
  t_grspw_regptr grspw_regptr = &deviceptr->grspw_reg;

  if (deviceptr->tx_pending)
  {
    grlib_comgrspw_txcomplete(deviceptr, e_true);
    if (checkbit(grspw_regptr->chn, C_SPWCHN_LE))
    {
      grspw_regptr->chn = clearbit(grspw_regptr->chn, C_SPWCHN_TE);
    }
  }
}

/*
 * \brief handle a packet without available descriptor
 * \return e_rx_wait if no spill is enabled, otherwise the packet is discarded
 */
static t_grspw_rx_result grlib_comgrspw_rxnospill(const t_grspw_regptr grspw_regptr)
{
  // [GRSPWSim-019] check if no spill is disable
  if (checkbit(grspw_regptr->chn, C_SPWCHN_NS))
  {
    // [GRSPWSim-020] otherwise put the packet in the pending queue
    return e_rx_wait;
  }

  // discard the packet
  CHECK_WARNING(e_true, "Incoming packet has been discarded (SPWCHN : Bit NS is set to '0')");
  return e_rx_done;
}

/*
 * \brief store a received packet in the guest memory
 * \param [in/out] deviceptr     The grspw device
 * \param [in]     data          The received packet
 * \param [in]     len           The length of the received packet
 * \param [in]     attptr        The message attribute in order fake error
 * \return e_rx_wait if the packet has to wait for a descriptor
 */
static t_grspw_rx_result grlib_comgrspw_rxstore(t_grsw_deviceptr deviceptr, const uint8 data[], const uint32 len, const t_recv_att *attptr)
{
  t_grspw_regptr grspw_regptr = &deviceptr->grspw_reg;
  t_grspw_rx_desc rxd;
  uint32 rxd_cur;
  uint32 length = len;
  boolean truncated = e_false;

  // [GRSPWSim-002] all packets are discarded silently if the link is down
  if (0 == len || !grlib_comgrspw_linkrun(grspw_regptr))
  {
    return e_rx_done;
  }

  if (!checkbit(grspw_regptr->ctr, C_SPWCTR_OPM))
  {
    // [GRSPWSim-xxx] packets smaller than the minimum size are discarded
    if (len < C_MIN_PACKET_LEN
        || (len == C_MIN_PACKET_LEN && C_RMAP_PROTOCOL == data[1] && grlib_rmapgrspw_enabled(grspw_regptr)))
    {
      grspw_regptr->str = setbit(grspw_regptr->str, C_SPWSTR_EE);
      return e_rx_done;
    }

    // [GRSPWSim-003] the packet is accepted in open packet mode or
    // [GRSPWSim-004] if the node address of the incoming matches with the configured node address
    if (grspw_regptr->ndr != data[0])
    {
      grspw_regptr->str = setbit(grspw_regptr->str, C_SPWSTR_IA);
      return e_rx_done;
    }
  }

  // RMAP commands do not go through the DMA channel
  if (grlib_rmapgrspw_is_command(grspw_regptr, data, len))
  {
    grlib_rmapgrspw_command(deviceptr, data, len);
    return e_rx_done;
  }

  // [GRSPWSim-002] all packets are discarded silently if the receiver is disabled
  if (!checkbit(grspw_regptr->chn, C_SPWCHN_RE))
  {
    return e_rx_done;
  }

  // [GRSPWSim-005] check if an RD descriptor is available
  if (!checkbit(grspw_regptr->chn, C_SPWCHN_RD))
  {
    return grlib_comgrspw_rxnospill(grspw_regptr);
  }

  // [GRSPWSim-006] read RX descriptor (base address rx descriptor + current descriptor)
  if (!read_leon3_ram(grspw_regptr->rxd, (uint8ptr) &rxd, sizeof(rxd), e_true))
  {
    // the packet is spilled and the receiver disabled [M 77.4.9]
    grlib_comgrspw_ahberror(deviceptr, C_SPWCHN_RA, C_SPWCHN_RE);
    return e_rx_done;
  }

  // [GRSPWSim-007] check if the Rx descriptor is enable
  if (!checkbit(rxd.ctrl, C_SPWTRW0_EN))
  {
    // [GRSPWSim-016] otherwise set the RD bit of the SPWCHN to '0'
    grspw_regptr->chn = clearbit(grspw_regptr->chn, C_SPWCHN_RD);
    return grlib_comgrspw_rxnospill(grspw_regptr);
  }

  // [GRSPWSim-008] check the data length of the incoming packet against the RX_MAX_LENGTH
  if (len > (grspw_regptr->rxl & C_SPWRXL_RX_MAX_LENGTH))
  {
    // [GRSPWSim-009] inform a truncated packet has been received
    truncated = e_true;
    length = (grspw_regptr->rxl & C_SPWRXL_RX_MAX_LENGTH);
  }

  // [GRSPWSim-010] the core writes the status bits: clear the values left in the descriptor,
  // then set message reception attribute in order to force error for simulation need
  rxd.ctrl = clearbit(rxd.ctrl, C_SPWTRW0_TR | C_SPWTRW0_DC | C_SPWTRW0_HC | C_SPWTRW0_EP);
  rxd.ctrl = setbit(rxd.ctrl, (truncated * C_SPWTRW0_TR));
  rxd.ctrl = setbit(rxd.ctrl, (attptr->dc * C_SPWTRW0_DC));
  rxd.ctrl = setbit(rxd.ctrl, (attptr->hc * C_SPWTRW0_HC));
  rxd.ctrl = setbit(rxd.ctrl, (attptr->ep * C_SPWTRW0_EP));
  rxd.ctrl = setbit(rxd.ctrl, grlib_rmapgrspw_rxflags(grspw_regptr, data, len));

  // [GRSPWSim-xxx] set the SPWSTS register EE bit if a message has been received with an EEP error status
  if (attptr->ep)
  {
    grspw_regptr->str = setbit(grspw_regptr->str, C_SPWSTR_EE);
  }

  // without unaligned receive accesses (no RMAP), only bits 31:2 of the address are used
  // [M 77.4.4]
  if (!checkbit(grspw_regptr->ctr, C_SPWCTR_RX))
  {
    rxd.addr &= ~3U;
  }

  // [GRSPWSim-011] copy the data according to according the incoming packet length or the truncated length
  if (!write_leon3_ram(rxd.addr, data, length, e_false)) // don't inverse the byte order
  {
    grlib_comgrspw_ahberror(deviceptr, C_SPWCHN_RA, C_SPWCHN_RE);
    return e_rx_done;
  }

  // [GRSPWSim-012] update the RX descriptor control word
  rxd.ctrl = deposit(rxd.ctrl, C_SPWTRW0_PACKET_LENGTH, length);

  // [GRSPWSim-xxx] write the RX descriptor in order to disable the used descriptor
  rxd.ctrl = clearbit(rxd.ctrl, C_SPWTRW0_EN);
  if (!write_leon3_ram(grspw_regptr->rxd, (uint8ptr) &rxd.ctrl, sizeof(rxd.ctrl), e_true))
  {
    grlib_comgrspw_ahberror(deviceptr, C_SPWCHN_RA, C_SPWCHN_RE);
    return e_rx_done;
  }

  // [GRSPWSim-013] inform a packet has been receive by setting the bit PR of the SPWCHN register
  grspw_regptr->chn = setbit(grspw_regptr->chn, C_SPWCHN_PR);

  // [GRSPWSim-015] wrap around if requested, otherwise increment and auto wrap the current rx descriptor
  rxd_cur = (grspw_regptr->rxd & C_SPWRXD_RX_DESC_SELECT) >> 3;
  rxd_cur = checkbit(rxd.ctrl, C_SPWTRW0_WR) ? 0 : (rxd_cur + 1) % C_NB_RX_DESCRIPTOR;
  grspw_regptr->rxd = deposit(grspw_regptr->rxd, C_SPWRXD_RX_DESC_SELECT, rxd_cur << 3);

  // [GRSPWSim-014] check if the interrupt is requested and if the interrupt controller is enable
  if (checkbit(rxd.ctrl, C_SPWTRW0_IE) && checkbit(grspw_regptr->chn, C_SPWCHN_RI))
  {
    qemu_irq_pulse(deviceptr->irq);
  }

  return e_rx_done;
}

/*
 * \brief transmit the receive data to the user space
 * \param [in/out] deviceptr     The grspw device
 * \param [in]     data          The received data
 * \param [in]     len           The length of the received data
 * \param [in]     attptr        The message attribute in order fake error
 * \return None
 * \detail This function transmits the receive data to the user space and manage registers and RX descriptor do to this
 */
void grlib_comgrspw_rxenable(t_grsw_deviceptr deviceptr, const uint8 data[], const uint32 len, const t_recv_attptr attptr)
{
  t_grspw_rx_packet *packet;

  // packets keep their order: nothing goes past a waiting packet
  if (g_queue_is_empty(&deviceptr->rx_pending)
      && e_rx_done == grlib_comgrspw_rxstore(deviceptr, data, len, attptr))
  {
    return;
  }

  if (g_queue_get_length(&deviceptr->rx_pending) >= C_MAX_RX_PENDING)
  {
    CHECK_WARNING(e_true, "Incoming packet has been discarded (too many packets waiting for a descriptor)");
    return;
  }

  packet = g_malloc(sizeof(*packet) + len);
  packet->attributes = *attptr;
  packet->len = len;
  memcpy(packet->data, data, len);
  g_queue_push_tail(&deviceptr->rx_pending, packet);
}

/*
 * \brief store the packets waiting for a descriptor
 * \param [in/out] deviceptr     The grspw device
 */
void grlib_comgrspw_rxflush(t_grsw_deviceptr deviceptr)
{
  t_grspw_rx_packet *packet;

  while ((packet = g_queue_peek_head(&deviceptr->rx_pending)) != NULL
      && e_rx_done == grlib_comgrspw_rxstore(deviceptr, packet->data, packet->len, &packet->attributes))
  {
    g_queue_pop_head(&deviceptr->rx_pending);
    g_free(packet);
  }
}

/*
 * \brief forget the pending packets upon a core reset
 * \param [in/out] deviceptr     The grspw device
 */
void grlib_comgrspw_reset(t_grsw_deviceptr deviceptr)
{
  t_grspw_rx_packet *packet;

  deviceptr->tx_pending = e_false;

  while ((packet = g_queue_pop_head(&deviceptr->rx_pending)) != NULL)
  {
    g_free(packet);
  }
}

/*
 * \brief update the time code registers if needed
 * \param [in/out] deviceptr     The grspw device
 * \param [in]     timecode      The received time code
 * \return None
 * \detail No time code is received unless TR is set. The received counter is always
 *         stored, but only a counter value one more than the current one is a valid
 *         time code: it stores the control flags and generates a tick out.
 */
void grlib_comgrspw_rxtimecode(t_grsw_deviceptr deviceptr, const uint8 timecode)
{
  t_grspw_regptr grspw_regptr = &deviceptr->grspw_reg;
  uint32 next_counter = (grspw_regptr->tim + 1U) & C_SPWTIM_TIME_COUNTER;
  boolean valid = (timecode & C_SPWTIM_TIME_COUNTER) == next_counter;

  if (!checkbit(grspw_regptr->ctr, C_SPWCTR_TR))
  {
    return;
  }

  grspw_regptr->tim = deposit(grspw_regptr->tim, C_SPWTIM_TIME_COUNTER, timecode);

  if (valid)
  {
    grspw_regptr->tim = deposit(grspw_regptr->tim, C_SPWTIM_TIME_CTRL, timecode);
    // [GRSPWSim-xxx] raise the Tick Out bit in order to notice a new time code has been received
    grspw_regptr->str = setbit(grspw_regptr->str, C_SPWSTR_TO);

    // check if the Tick Out interrupt is enabled
    if (checkbit(grspw_regptr->ctr, C_SPWCTR_TQ) && checkbit(grspw_regptr->ctr, C_SPWCTR_IE))
    {
      qemu_irq_pulse(deviceptr->irq);
    }
  }
}
