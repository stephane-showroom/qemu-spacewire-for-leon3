/* SPDX-License-Identifier: GPL-2.0-or-later */
/********************************************************************************
 *                                                                              *
 *    ▀▄   ▄▀                                                                   *
 *   ▄█▀███▀█▄    Created on: Nov 6, 2026                                       *
 *  █▀███████▀█   Author: Stephane GONZALEZ                                     *
 *  █ █▀▀▀▀▀█ █                                                                 *
 *     ▀▀ ▀▀                                                                    *
 *                                                                              *
 * Copyright (C) 2015-2026 Stephane GONZALEZ                                    *
 * Released under the GNU GPL v2 or later, see LICENSE                          *
 ********************************************************************************/

#include "qemu/osdep.h"
#include "system/memory.h"
#include "system/address-spaces.h"

#include "grlib_rmapgrspw.h"
#include "grlib_mqgrspw.h"
#include "grlib_reggrspw.h"
#include "grlib_utilgrspw.h"

#define C_RMAP_PROTOCOL_ID      (0x01U)

// instruction field
#define C_RMAP_TYPE_SHIFT       (6U)
#define C_RMAP_TYPE_COMMAND     (1U)
#define C_RMAP_INS_WRITE        (1U << 5)
#define C_RMAP_INS_VERIFY       (1U << 4)
#define C_RMAP_INS_ACK          (1U << 3)
#define C_RMAP_INS_INCREMENT    (1U << 2)
#define C_RMAP_INS_REPLY_LEN    (3U)
#define C_RMAP_INS_COMMAND_MASK (0x3FU)      ///< packet type cleared: instruction of the reply

// fields of a command, after the reply address
#define C_RMAP_CMD_INITIATOR    (4U)
#define C_RMAP_CMD_TID          (5U)
#define C_RMAP_CMD_EXT_ADDR     (7U)
#define C_RMAP_CMD_ADDR         (8U)
#define C_RMAP_CMD_LENGTH       (12U)
#define C_RMAP_CMD_HEADER_SIZE  (16U)       ///< without the reply address

#define C_RMAP_KEY_IDX          (3U)
#define C_RMAP_REPLY_ADDR_IDX   (4U)

#define C_RMAP_WRITE_REPLY_HEADER_SIZE  (8U)
#define C_RMAP_READ_REPLY_HEADER_SIZE   (12U)

#define C_RMAP_MAX_VERIFIED     (4U)
#define C_WORD_SIZE             (4U)

// status codes
#define C_RMAP_OK               (0U)
#define C_RMAP_GENERAL_ERROR    (1U)
#define C_RMAP_UNUSED_COMMAND   (2U)
#define C_RMAP_INVALID_KEY      (3U)
#define C_RMAP_INVALID_DATA_CRC (4U)
#define C_RMAP_EARLY_EOP        (5U)
#define C_RMAP_TOO_MUCH_DATA    (6U)
#define C_RMAP_VERIFY_OVERRUN   (9U)
#define C_RMAP_NOT_AUTHORISED   (10U)
#define C_RMAP_RMW_LENGTH_ERROR (11U)

typedef struct
{
  const uint8 *packet;
  const uint8 *data;        ///< what follows the header
  uint32 available;         ///< number of bytes following the header
  uint32 reply_addr_len;
  uint32 tail;              ///< offset of the fields which follow the reply address
  uint8 instruction;
  uint32 address;
  uint32 length;
} t_rmap_command;

static uint8 g_rmapreply[C_MAXQ_SIZE];

/*
 * \brief RMAP CRC: x^8 + x^2 + x + 1, least significant bit first, initial value 0
 */
uint8 grlib_rmapgrspw_crc(const uint8 data[], const uint32 len)
{
  uint8 crc = 0;
  uint32 i;
  uint32 bit;

  for (i = 0; i < len; ++i)
  {
    crc ^= data[i];
    for (bit = 0; bit < 8U; ++bit)
    {
      crc = (crc & 1U) ? (uint8) ((crc >> 1) ^ 0xE0U) : (uint8) (crc >> 1);
    }
  }

  return crc;
}

static uint32 grlib_rmapgrspw_be(const uint8 data[], const uint32 len)
{
  uint32 value = 0;
  uint32 i;

  for (i = 0; i < len; ++i)
  {
    value = (value << 8) | data[i];
  }

  return value;
}

boolean grlib_rmapgrspw_enabled(const t_grspw_regptr grspw_regptr)
{
  return checkbit(grspw_regptr->ctr, C_SPWCTR_RA | C_SPWCTR_RE);
}

/*
 * \return e_true if the packet goes to the RMAP target instead of the DMA channel
 */
boolean grlib_rmapgrspw_is_command(const t_grspw_regptr grspw_regptr, const uint8 data[], const uint32 len)
{
  return grlib_rmapgrspw_enabled(grspw_regptr) && len >= 3U
      && C_RMAP_PROTOCOL_ID == data[1]
      && C_RMAP_TYPE_COMMAND == (data[2] >> C_RMAP_TYPE_SHIFT);
}

/*
 * \return e_true if the size and the address are allowed for one single bus access
 */
static boolean grlib_rmapgrspw_single_access_allowed(const uint32 address, const uint32 size)
{
  switch (size)
  {
    case 0:
    case 1:
      return e_true;
    case 2:
      return 0 == (address & 1U);
    case 4:
      return 0 == (address & 3U);
    default:
      return e_false;
  }
}

static boolean grlib_rmapgrspw_word_aligned(const uint32 address, const uint32 length)
{
  return 0 == (address & 3U) && 0 == (length & 3U);
}

static boolean grlib_rmapgrspw_read(const uint32 address, uint8 buffer[], const uint32 length, const boolean increment)
{
  MemTxAttrs attrs = MEMTXATTRS_UNSPECIFIED;
  uint32 offset;

  if (increment)
  {
    return MEMTX_OK == address_space_read(&address_space_memory, address, attrs, buffer, length);
  }

  for (offset = 0; offset < length; offset += C_WORD_SIZE)
  {
    if (MEMTX_OK != address_space_read(&address_space_memory, address, attrs, &buffer[offset], C_WORD_SIZE))
    {
      return e_false;
    }
  }

  return e_true;
}

static boolean grlib_rmapgrspw_write(const uint32 address, const uint8 buffer[], const uint32 length, const boolean increment)
{
  MemTxAttrs attrs = MEMTXATTRS_UNSPECIFIED;
  uint32 offset;

  if (increment)
  {
    return MEMTX_OK == address_space_write(&address_space_memory, address, attrs, buffer, length);
  }

  for (offset = 0; offset < length; offset += C_WORD_SIZE)
  {
    if (MEMTX_OK != address_space_write(&address_space_memory, address, attrs, &buffer[offset], C_WORD_SIZE))
    {
      return e_false;
    }
  }

  return e_true;
}

/*
 * \brief number of reply address bytes sent: leading zeros are ignored, but one zero
 *        is sent if there are only zeros
 * \return the index of the first byte sent in *first
 */
static uint32 grlib_rmapgrspw_reply_addr_size(const t_rmap_command *command, uint32 *first)
{
  const uint8 *reply_addr = &command->packet[C_RMAP_REPLY_ADDR_IDX];

  *first = 0;
  while (*first + 1U < command->reply_addr_len && 0 == reply_addr[*first])
  {
    ++*first;
  }

  return command->reply_addr_len > 0 ? command->reply_addr_len - *first : 0U;
}

/*
 * \brief write the beginning of a reply: reply address, then the common fields
 * \return the size of the reply so far, *header points to the fields covered by the header CRC
 */
static uint32 grlib_rmapgrspw_reply_start(const t_rmap_command *command, const uint8 status, uint8 **header)
{
  const uint8 *reply_addr = &command->packet[C_RMAP_REPLY_ADDR_IDX];
  uint32 first;
  uint32 size = grlib_rmapgrspw_reply_addr_size(command, &first);

  memcpy(g_rmapreply, &reply_addr[first], size);

  *header = &g_rmapreply[size];
  g_rmapreply[size++] = command->packet[command->tail + C_RMAP_CMD_INITIATOR];
  g_rmapreply[size++] = C_RMAP_PROTOCOL_ID;
  g_rmapreply[size++] = command->instruction & C_RMAP_INS_COMMAND_MASK;
  g_rmapreply[size++] = status;
  g_rmapreply[size++] = command->packet[0];
  g_rmapreply[size++] = command->packet[command->tail + C_RMAP_CMD_TID];
  g_rmapreply[size++] = command->packet[command->tail + C_RMAP_CMD_TID + 1U];

  return size;
}

static void grlib_rmapgrspw_send(t_grsw_deviceptr deviceptr, const uint32 size)
{
  grlib_mqgrspw_send_spwpacket(deviceptr->grspw_id, C_MSGQ_RMAP, NULL, 0, g_rmapreply, size);
}

static void grlib_rmapgrspw_write_reply(t_grsw_deviceptr deviceptr, const t_rmap_command *command, const uint8 status)
{
  uint8 *header;
  uint32 size = grlib_rmapgrspw_reply_start(command, status, &header);

  g_rmapreply[size++] = grlib_rmapgrspw_crc(header, C_RMAP_WRITE_REPLY_HEADER_SIZE - 1U);
  grlib_rmapgrspw_send(deviceptr, size);
}

/*
 * \brief send a reply with a data field
 * \param [in] data     The data to send, NULL if there is no data field: after an
 *                      error, or when a read fails after the header has been sent
 * \detail No data follows the header if the status is not zero. The data length
 *         field always holds the amount of data requested.
 */
static void grlib_rmapgrspw_read_reply(t_grsw_deviceptr deviceptr, const t_rmap_command *command,
    const uint8 status, const uint8 data[], const uint32 length)
{
  uint8 *header;
  uint32 size = grlib_rmapgrspw_reply_start(command, status, &header);

  g_rmapreply[size++] = 0; // reserved
  g_rmapreply[size++] = (uint8) (length >> 16);
  g_rmapreply[size++] = (uint8) (length >> 8);
  g_rmapreply[size++] = (uint8) length;
  g_rmapreply[size++] = grlib_rmapgrspw_crc(header, C_RMAP_READ_REPLY_HEADER_SIZE - 1U);

  if (NULL != data)
  {
    memcpy(&g_rmapreply[size], data, length);
    size += length;
    g_rmapreply[size++] = grlib_rmapgrspw_crc(data, length);
  }

  grlib_rmapgrspw_send(deviceptr, size);
}

static void grlib_rmapgrspw_reply(t_grsw_deviceptr deviceptr, const t_rmap_command *command, const uint8 status)
{
  if (!(command->instruction & C_RMAP_INS_ACK))
  {
    return;
  }

  if (command->instruction & C_RMAP_INS_WRITE)
  {
    grlib_rmapgrspw_write_reply(deviceptr, command, status);
  }
  else
  {
    uint32 length = command->length;

    if (command->instruction & C_RMAP_INS_VERIFY)
    {
      length /= 2U; // RMW: half of the command is the mask
    }

    grlib_rmapgrspw_read_reply(deviceptr, command, status, NULL, length);
  }
}

/*
 * \return the status of the packet tail: early EOP, then data CRC
 */
static uint8 grlib_rmapgrspw_check_data(const t_rmap_command *command)
{
  if (command->available < command->length + 1U)
  {
    return C_RMAP_EARLY_EOP;
  }

  if (0 != grlib_rmapgrspw_crc(command->data, command->length + 1U))
  {
    return C_RMAP_INVALID_DATA_CRC;
  }

  return C_RMAP_OK;
}

static uint8 grlib_rmapgrspw_too_much_data(const t_rmap_command *command, const uint8 status)
{
  if (C_RMAP_OK == status && command->available > command->length + 1U)
  {
    return C_RMAP_TOO_MUCH_DATA;
  }

  return status;
}

static uint8 grlib_rmapgrspw_do_write(const t_rmap_command *command)
{
  boolean verify = (command->instruction & C_RMAP_INS_VERIFY) != 0;
  boolean increment = (command->instruction & C_RMAP_INS_INCREMENT) != 0;
  uint8 status;

  if (verify)
  {
    // nothing is written unless the complete packet is correct
    status = grlib_rmapgrspw_check_data(command);
    if (C_RMAP_OK != status)
    {
      return status;
    }

    if (!grlib_rmapgrspw_write(command->address, command->data, command->length, e_true))
    {
      return C_RMAP_GENERAL_ERROR;
    }
  }
  else
  {
    // the data is written on the fly, before the end of the packet is known
    uint32 size = MIN(command->available, command->length);

    if (!grlib_rmapgrspw_write(command->address, command->data, size, increment))
    {
      return C_RMAP_GENERAL_ERROR;
    }

    status = grlib_rmapgrspw_check_data(command);
    if (C_RMAP_OK != status)
    {
      return status;
    }
  }

  return grlib_rmapgrspw_too_much_data(command, C_RMAP_OK);
}

static void grlib_rmapgrspw_do_read(t_grsw_deviceptr deviceptr, const t_rmap_command *command)
{
  static uint8 data[C_MAXQ_SIZE];
  boolean increment = (command->instruction & C_RMAP_INS_INCREMENT) != 0;

  if (command->available > 0)
  {
    grlib_rmapgrspw_reply(deviceptr, command, C_RMAP_TOO_MUCH_DATA);
  }
  else if (grlib_rmapgrspw_read(command->address, data, command->length, increment))
  {
    grlib_rmapgrspw_read_reply(deviceptr, command, C_RMAP_OK, data, command->length);
  }
  else
  {
    // the read is done on the fly: the header has already been sent with a
    // correct status and the packet is truncated
    grlib_rmapgrspw_read_reply(deviceptr, command, C_RMAP_OK, NULL, command->length);
  }
}

static void grlib_rmapgrspw_do_rmw(t_grsw_deviceptr deviceptr, const t_rmap_command *command)
{
  uint32 size = command->length / 2U;
  uint8 status = grlib_rmapgrspw_check_data(command);
  uint8 old_data[C_RMAP_MAX_VERIFIED];
  uint8 new_data[C_RMAP_MAX_VERIFIED];
  uint32 i;

  if (C_RMAP_OK != status)
  {
    grlib_rmapgrspw_reply(deviceptr, command, status);
    return;
  }

  if (!grlib_rmapgrspw_read(command->address, old_data, size, e_true))
  {
    grlib_rmapgrspw_reply(deviceptr, command, C_RMAP_GENERAL_ERROR);
    return;
  }

  for (i = 0; i < size; ++i)
  {
    uint8 mask = command->data[size + i];

    new_data[i] = (uint8) ((mask & command->data[i]) | (~mask & old_data[i]));
  }

  if (!grlib_rmapgrspw_write(command->address, new_data, size, e_true))
  {
    grlib_rmapgrspw_reply(deviceptr, command, C_RMAP_GENERAL_ERROR);
    return;
  }

  // too much data is detected after the bus accesses
  status = grlib_rmapgrspw_too_much_data(command, C_RMAP_OK);
  if (C_RMAP_OK == status)
  {
    grlib_rmapgrspw_read_reply(deviceptr, command, C_RMAP_OK, old_data, size);
  }
  else
  {
    grlib_rmapgrspw_reply(deviceptr, command, status);
  }
}

/*
 * \return the status of the checks done before any bus access, in the order of
 *         table 1335
 */
static uint8 grlib_rmapgrspw_check_command(const t_rmap_command *command, const uint8 key)
{
  uint8 ins = command->instruction;
  boolean write = (ins & C_RMAP_INS_WRITE) != 0;
  boolean verify = (ins & C_RMAP_INS_VERIFY) != 0;
  boolean increment = (ins & C_RMAP_INS_INCREMENT) != 0;
  boolean rmw = !write && verify;
  uint32 first;
  // reply address, header with its CRC, data CRC
  uint32 reply_overhead = grlib_rmapgrspw_reply_addr_size(command, &first)
                          + C_RMAP_READ_REPLY_HEADER_SIZE + 1U;

  if (rmw && !increment)
  {
    return C_RMAP_UNUSED_COMMAND;
  }

  if (key != command->packet[C_RMAP_KEY_IDX])
  {
    return C_RMAP_INVALID_KEY;
  }

  if (write && verify && command->length > C_RMAP_MAX_VERIFIED)
  {
    return C_RMAP_VERIFY_OVERRUN;
  }

  // all the lengths of the standard but 6, which would be a 3 bytes bus access
  if (rmw && 0 != command->length && 2U != command->length
      && 4U != command->length && 8U != command->length)
  {
    return C_RMAP_RMW_LENGTH_ERROR;
  }

  if (0 != command->packet[command->tail + C_RMAP_CMD_EXT_ADDR])
  {
    return C_RMAP_NOT_AUTHORISED; // 32-bit addresses only
  }

  if (verify)
  {
    uint32 size = rmw ? command->length / 2U : command->length;

    if (!grlib_rmapgrspw_single_access_allowed(command->address, size))
    {
      return C_RMAP_NOT_AUTHORISED;
    }
  }
  else if (!increment && !grlib_rmapgrspw_word_aligned(command->address, command->length))
  {
    return C_RMAP_NOT_AUTHORISED;
  }

  if (!write && command->length + reply_overhead > C_MAX_PACKET_SIZE)
  {
    return C_RMAP_NOT_AUTHORISED; // the reply would not fit in a queue message
  }

  return C_RMAP_OK;
}

/*
 * \brief execute a RMAP command
 * \param [in/out] deviceptr     The grspw device
 * \param [in]     data          The packet, from the target logical address
 * \param [in]     len           The length of the packet
 */
void grlib_rmapgrspw_command(t_grsw_deviceptr deviceptr, const uint8 data[], const uint32 len)
{
  t_rmap_command command;
  uint32 header_size;
  uint8 status;
  uint8 ins = data[2];

  command.packet = data;
  command.instruction = ins;
  command.reply_addr_len = (ins & C_RMAP_INS_REPLY_LEN) * C_WORD_SIZE;
  header_size = C_RMAP_CMD_HEADER_SIZE + command.reply_addr_len;

  // an incomplete header, or a header with a bad CRC: the packet is discarded without reply
  if (len < header_size || 0 != grlib_rmapgrspw_crc(data, header_size))
  {
    return;
  }

  // table 1336: these commands do nothing and have no reply
  if (!(ins & C_RMAP_INS_WRITE) && !(ins & C_RMAP_INS_ACK))
  {
    return;
  }

  command.tail = command.reply_addr_len;
  command.address = grlib_rmapgrspw_be(&data[command.tail + C_RMAP_CMD_ADDR], C_WORD_SIZE);
  command.length = grlib_rmapgrspw_be(&data[command.tail + C_RMAP_CMD_LENGTH], 3U);
  command.data = &data[header_size];
  command.available = len - header_size;

  status = grlib_rmapgrspw_check_command(&command, (uint8) deviceptr->grspw_reg.key);
  if (C_RMAP_OK != status)
  {
    grlib_rmapgrspw_reply(deviceptr, &command, status);
  }
  else if (ins & C_RMAP_INS_WRITE)
  {
    grlib_rmapgrspw_reply(deviceptr, &command, grlib_rmapgrspw_do_write(&command));
  }
  else if (ins & C_RMAP_INS_VERIFY)
  {
    grlib_rmapgrspw_do_rmw(deviceptr, &command);
  }
  else
  {
    grlib_rmapgrspw_do_read(deviceptr, &command);
  }
}

/*
 * \brief RMAP CRC check of a packet stored to the DMA channel
 * \return the HC and DC bits of the RX descriptor
 * \detail The packet is assumed to be an RMAP packet, its third byte gives the
 *         length of the header. HC and DC are meaningless for other packets.
 */
uint32 grlib_rmapgrspw_rxflags(const t_grspw_regptr grspw_regptr, const uint8 data[], const uint32 len)
{
  uint32 flags = 0;
  uint32 header_size;

  if (!checkbit(grspw_regptr->ctr, C_SPWCTR_RC) || len < 3U)
  {
    return 0;
  }

  if (C_RMAP_TYPE_COMMAND == (data[2] >> C_RMAP_TYPE_SHIFT))
  {
    header_size = C_RMAP_CMD_HEADER_SIZE + (data[2] & C_RMAP_INS_REPLY_LEN) * C_WORD_SIZE;
  }
  else if (data[2] & C_RMAP_INS_WRITE)
  {
    header_size = C_RMAP_WRITE_REPLY_HEADER_SIZE;
  }
  else
  {
    header_size = C_RMAP_READ_REPLY_HEADER_SIZE;
  }

  if (len >= header_size && 0 != grlib_rmapgrspw_crc(data, header_size))
  {
    flags |= C_SPWTRW0_HC;
  }

  if (0 != grlib_rmapgrspw_crc(data, len))
  {
    flags |= C_SPWTRW0_DC;
  }

  return flags;
}
