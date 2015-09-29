/**
 *
 * \section COPYRIGHT
 *
 * Copyright 2015 The srsUE Developers. See the
 * COPYRIGHT file at the top-level directory of this distribution.
 *
 * \section LICENSE
 *
 * This file is part of the srsUE library.
 *
 * srsUE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsUE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

/******************************************************************************
 * File:        interfaces.h
 * Description: Abstract base class interfaces provided by layers
 *              to other layers.
 *****************************************************************************/

#ifndef INTERFACES_H
#define INTERFACES_H

#include "liblte/hdr/liblte_rrc.h"
#include "common/common.h"
#include "mac_interface.h"
#include "phy_interface.h"

namespace srsue {

// UE interface
class ue_interface
{
public:
  virtual void notify() = 0;
};

// GW interface for PDCP
class gw_interface_pdcp
{
public:
  //virtual void write_sdu(uint32_t lcid, uint8_t *payload, uint32_t nof_bytes) = 0;
};

// NAS interface for RRC
class nas_interface_rrc
{
public:
  //virtual void write_sdu(uint32_t lcid, uint8_t *payload, uint32_t nof_bytes) = 0;
};

// RRC interface for NAS
class rrc_interface_nas
{
public:
  //virtual void write_sdu(uint32_t lcid, uint8_t *payload, uint32_t nof_bytes) = 0;
};

// RRC interface for PDCP
class rrc_interface_pdcp
{
public:
  virtual void write_pdu(uint32_t lcid, srsue_byte_buffer_t *pdu) = 0;
  virtual void write_pdu_bcch_bch(srsue_byte_buffer_t *pdu) = 0;
  virtual void write_pdu_bcch_dlsch(srsue_byte_buffer_t *pdu) = 0;
};

// PDCP interface for GW
class pdcp_interface_gw
{
public:
  virtual void write_sdu(uint32_t lcid, srsue_byte_buffer_t *sdu) = 0;
};

// PDCP interface for RRC
class pdcp_interface_rrc
{
public:
  virtual void write_sdu(uint32_t lcid, srsue_byte_buffer_t *sdu) = 0;
  virtual void add_bearer(uint32_t lcid) = 0;
};

// PDCP interface for RLC
class pdcp_interface_rlc
{
public:
  /* RLC calls PDCP to push a PDCP PDU. */
  virtual void write_pdu(uint32_t lcid, srsue_byte_buffer_t *sdu) = 0;
  virtual void write_pdu_bcch_bch(srsue_byte_buffer_t *sdu) = 0;
  virtual void write_pdu_bcch_dlsch(srsue_byte_buffer_t *sdu) = 0;
};

// RLC interface for RRC
class rlc_interface_rrc
{
public:
  virtual void add_bearer(uint32_t lcid, LIBLTE_RRC_RLC_CONFIG_STRUCT *cnfg) = 0;
};

// RLC interface for PDCP
class rlc_interface_pdcp
{
public:
  /* PDCP calls RLC to push an RLC SDU. SDU gets placed into the RLC buffer and MAC pulls
   * RLC PDUs according to TB size. */
  virtual void write_sdu(uint32_t lcid,  srsue_byte_buffer_t *sdu) = 0;
};

//RLC interface for MAC
class rlc_interface_mac
{
public:
  /* MAC calls RLC to get buffer state for a logical channel.
   * This function should return quickly. */
  virtual uint32_t get_buffer_state(uint32_t lcid) = 0;

  const static int MAX_PDU_SEGMENTS = 20;

  /* MAC calls RLC to get RLC segment of nof_bytes length.
   * Segmentation happens in this function. RLC PDU is stored in payload. */
  virtual int     read_pdu(uint32_t lcid, uint8_t *payload, uint32_t nof_bytes) = 0;

  /* MAC calls RLC to push an RLC PDU. This function is called from an independent MAC thread.
   * PDU gets placed into the buffer and higher layer thread gets notified. */
  virtual void write_pdu(uint32_t lcid, uint8_t *payload, uint32_t nof_bytes) = 0;
  virtual void write_pdu_bcch_bch(uint8_t *payload, uint32_t nof_bytes) = 0;
  virtual void write_pdu_bcch_dlsch(uint8_t *payload, uint32_t nof_bytes) = 0;
};

} // namespace srsue

#endif // INTERFACES_H
