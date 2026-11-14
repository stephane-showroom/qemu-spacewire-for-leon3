/* SPDX-License-Identifier: GPL-2.0-or-later */
/********************************************************************************
 *                                                                              *
 *    ▀▄   ▄▀                                                                   *
 *   ▄█▀███▀█▄    Created on: Jul 5, 2015                                       *
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

#include "grlib_typesgrspw.h"
#include "grlib_utilgrspw.h"

#define FOUR_BYTE   (4)
#define THREE_BYTE  (3)
#define TWO_BYTE    (2)
#define ONE_BYTE    (1)

void endianness_swap(uint8 dest[], const uint8 src[], const uint32 len)
{
  int32 byte = len;
  int32 i = 0;

  // translate from big-endian to little-endian the content on the leon3 RAM
  while (byte > 0)
  {
    if ((byte - FOUR_BYTE) >= 0)
    {
      dest[i] = src[3 + i];
      dest[i + 1] = src[2 + i];
      dest[i + 2] = src[1 + i];
      dest[i + 3] = src[i];
      byte -= 4;
      i += 4;
    }
    else if ((byte - THREE_BYTE) >= 0)
    {
      dest[i] = src[2 + i];
      dest[i + 1] = src[1 + i];
      dest[i + 2] = src[i];
      byte -= 3;
      i += 3;
    }
    else if ((byte - TWO_BYTE) >= 0)
    {
      dest[i] = src[1 + i];
      dest[i + 1] = src[i];
      byte -= 2;
      i += 2;
    }
    else if ((byte - ONE_BYTE) >= 0)
    {
      dest[i] = src[i];
      byte = 0;
      i += 1;
    }
  }
}

/*
 * \return e_false on a bus error (an access outside guest memory), as an AHB error
 */
boolean write_leon3_ram(const hwaddr addr, const uint8 buff[], const uint32 len, const boolean sawp_endian)
{
  MemTxAttrs attrs = MEMTXATTRS_UNSPECIFIED; // default memory attribute
  MemTxResult result;

  uint8 * tmpbuff = malloc(len); // the temporary buffer

  // if the format flag in little endian is raised
  if (sawp_endian == e_true)
  {
    endianness_swap(tmpbuff, buff, len);
  }
  else
  {
    (void) memcpy(tmpbuff, buff, len);
  }

  // copy in the temporary buffer the content of the leon3 RAM
  result = address_space_write(&address_space_memory, addr, attrs, tmpbuff, len);

  // freed the temporary buffer
  free(tmpbuff);

  return (MEMTX_OK == result) ? e_true : e_false;
}

/*
 * \return e_false on a bus error (an access outside guest memory), as an AHB error
 */
boolean read_leon3_ram(const hwaddr addr, uint8 buff[], const uint32 len, const boolean sawp_endian)
{

  MemTxAttrs attrs = MEMTXATTRS_UNSPECIFIED; // default memory attribute
  uint8 * tmpbuff = malloc(len); // the temporary buffer
  MemTxResult result;

  // copy in the temporary buffer the content of the leon3 RAM
  result = address_space_read(&address_space_memory, addr, attrs, tmpbuff, len);

  int32 byte = len;

  // if the format flag in little endian is raised
  if (sawp_endian == e_true)
  {
    endianness_swap(buff, tmpbuff, len);
  }
  else
  {
    (void) memcpy(buff, tmpbuff, byte);
  }

  // freed the temporary buffer
  free(tmpbuff);

  return (MEMTX_OK == result) ? e_true : e_false;
}


/*
 * \brief register utility check bits
 * \param [in]  reg           The register value
 * \param [in] mask           The mask to test against
 * \return  e_true  if the mask matches with the register value
 *          e_false if the mask doesn't match
 * \detail This function checks if the register contains a bit or a particular set of bit values to '1'
 */
boolean checkbit(uint32 reg, uint32 mask)
{
  return (mask == (reg & mask));
}

/*
 * \brief register utility set bits
 * \param [in]  reg           The register value
 * \param [in] mask           The mask to set into
 * \return the set operation result
 * \detail This function set the mask value in the register value
 */
uint32 setbit(uint32 reg, uint32 mask)
{
  return (reg |= mask);
}

/*
 * \brief register utility clear bits
 * \param [in]  reg           The register value
 * \param [in]  mask           The mask to clear into
 * \return the clear operation result
 * \detail This function clear the register bits if where the mask value is '1'
 */
uint32 clearbit(uint32 reg, uint32 mask)
{
  return (reg &= ~mask);
}

/*
 * \brief register utility replace the content
 * \param [in]  reg           The register value
 * \param [in]  mask          The mask where the new content is got and deposed
 * \param [out] value         The new value
 * \return the register with the new value deposed into
 * \detail This function replace the register content masked by the mask by the new value
 */
uint32 deposit(uint32 reg, uint32 mask, uint32 value)
{
  return ( (reg & ~mask) | (value & mask) );
}
