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

#include "upper/rlc_am.h"

using namespace srslte;

namespace srsue{

rlc_am::rlc_am()
{}

void rlc_am::init(srslte::log *log_, uint32_t lcid_, pdcp_interface_rlc *pdcp_)
{
  log  = log_;
  lcid = lcid_;
  pdcp = pdcp_;
}

void rlc_am::configure(LIBLTE_RRC_RLC_CONFIG_STRUCT *cnfg)
{
  t_poll_retx       = liblte_rrc_t_poll_retransmit_num[cnfg->ul_am_rlc.t_poll_retx];
  poll_pdu          = liblte_rrc_poll_pdu_num[cnfg->ul_am_rlc.poll_pdu];
  poll_byte         = liblte_rrc_poll_byte_num[cnfg->ul_am_rlc.poll_byte];
  max_retx_thresh   = liblte_rrc_max_retx_threshold_num[cnfg->ul_am_rlc.max_retx_thresh];

  t_reordering      = liblte_rrc_t_reordering_num[cnfg->dl_am_rlc.t_reordering];
  t_status_prohibit = liblte_rrc_t_status_prohibit_num[cnfg->dl_am_rlc.t_status_prohibit];
}

rlc_mode_t rlc_am::get_mode()
{
  return RLC_MODE_AM;
}

uint32_t rlc_am::get_bearer()
{
  return lcid;
}

// PDCP interface
void rlc_am::write_sdu(srsue_byte_buffer_t *sdu)
{
  tx_sdu_queue.write(sdu);
}

bool rlc_am::read_sdu()
{
  // Iterate through receive window
    // If data
      // Try to reassemble SDUs and give to PDCP
      // Update vr_r and vr_mr
      // Update receive state variables (for tx status packets)
    // If missing
      // Update vr_x and reordering_timeout if necessary
    // If control
      // Check if already handled
      // Handle NACKs - add to tx retx buffer
      // Handle ACKs - mark as ACKed, remove from tx window if possible
      // Mark as handled

  return false;
}

// MAC interface
uint32_t rlc_am::get_buffer_state(){return 0;}

int rlc_am::read_pdu(uint8_t *payload, uint32_t nof_bytes)
{
  // Is status_requested and !status_prohibit_timeout?
    // Read the receive state variables
    // Check if Poll is needed
    // Create status PDU
    // Add to tx window, set retx timer, set retx count
    // Send PDU
  // Check for retransmit PDUs
    // If it fits in opportunity
      // Set retx timer, set retx count
      // Remove from retx buffer
      // Check if Poll is needed
      // Send PDU
    // If it doesn't fit
      // For now, wait for a bigger opportunity
      // In future....
        // Segment the PDU
        // Recreate header
        // Set segment info in tx window
        // Set retx timer, set retx count
        // Send PDU
  // Create a PDU
    // While we have room
    // Pull SDU, add to PDU, check for space, repeat
    // Check Poll
    // Set Framing Info
    // Send PDU
}

void rlc_am::write_pdu(uint8_t *payload, uint32_t nof_bytes)
{
  // If poll bit
    // Set status_requested
  // If Data
    // Check SN is inside receive window
    // Check SN isn't already in receive window
    // Write to rx window, update vr_h, notify thread
  // If Control
    // Reset poll_retx_timeout
    // Write to rx window, update vr_h, notify thread
}

void rlc_am_read_data_pdu_header(srsue_byte_buffer_t *pdu, rlc_amd_pdu_header_t *header)
{
  uint8_t  ext;
  uint8_t *ptr = pdu->msg;

  header->dc = (rlc_dc_field_t)((*ptr >> 7) & 0x01);

  if(RLC_DC_FIELD_DATA_PDU == header->dc)
  {
    // Fixed part
    header->rf =                 ((*ptr >> 6) & 0x01);
    header->p  =                 ((*ptr >> 5) & 0x01);
    header->fi = (rlc_fi_field_t)((*ptr >> 3) & 0x03);
    ext        =                 ((*ptr >> 2) & 0x01);
    header->sn =                 (*ptr & 0x03) << 8; // 2 bits SN
    ptr++;
    header->sn |=                (*ptr & 0xFF);     // 8 bits SN
    ptr++;

    if(header->rf)
    {
      header->lsf = ((*ptr >> 7) & 0x01);
      header->so  = (*ptr & 0x7F) << 8; // 7 bits of SO
      ptr++;
      header->so |= (*ptr & 0xFF);      // 8 bits of SO
      ptr++;
    }

    // Extension part
    header->N_li = 0;
    while(ext)
    {
      if(header->N_li%2 == 0)
      {
        ext = ((*ptr >> 7) & 0x01);
        header->li[header->N_li]  = (*ptr & 0x7F) << 4; // 7 bits of LI
        ptr++;
        header->li[header->N_li] |= (*ptr & 0xF0) >> 4; // 4 bits of LI
        header->N_li++;
      }
      else
      {
        ext = (*ptr >> 3) & 0x01;
        header->li[header->N_li] = (*ptr & 0x07) << 8; // 3 bits of LI
        ptr++;
        header->li[header->N_li] |= (*ptr & 0xFF);     // 8 bits of LI
        header->N_li++;
        ptr++;
      }
    }
  }
}

void rlc_am_write_data_pdu_header(rlc_amd_pdu_header_t *header, srsue_byte_buffer_t *pdu)
{
  uint32_t i;
  uint8_t ext = (header->N_li > 0) ? 1 : 0;

  // Make room for the header
  uint32_t len = rlc_am_packed_length(header);
  pdu->msg -= len;
  uint8_t *ptr = pdu->msg;

  // Fixed part
  *ptr  = (header->dc & 0x01) << 7;
  *ptr |= (header->rf & 0x01) << 6;
  *ptr |= (header->p  & 0x01) << 5;
  *ptr |= (header->fi & 0x03) << 3;
  *ptr |= (ext        & 0x01) << 2;

  *ptr |= (header->sn & 0x300) >> 8; // 2 bits SN
  ptr++;
  *ptr  = (header->sn & 0xFF);       // 8 bits SN
  ptr++;

  // Segment part
  if(header->rf)
  {
    *ptr  = (header->lsf & 0x01) << 7;
    *ptr |= (header->so  & 0x7F00) >> 8; // 7 bits of SO
    ptr++;
    *ptr = (header->so  & 0x00FF);       // 8 bits of SO
    ptr++;
  }

  // Extension part
  i = 0;
  while(i < header->N_li)
  {
    ext = ((i+1) == header->N_li) ? 0 : 1;
    *ptr  = (ext           &  0x01) << 7; // 1 bit header
    *ptr |= (header->li[i] & 0x7F0) >> 4; // 7 bits of LI
    ptr++;
    *ptr  = (header->li[i] & 0x00F) << 4; // 4 bits of LI
    i++;
    if(i < header->N_li)
    {
      ext = ((i+1) == header->N_li) ? 0 : 1;
      *ptr |= (ext           &  0x01) << 3; // 1 bit header
      *ptr |= (header->li[i] & 0x700) >> 8; // 3 bits of LI
      ptr++;
      *ptr  = (header->li[i] & 0x0FF);      // 8 bits of LI
      ptr++;
      i++;
    }
  }
  // Pad if N_li is odd
  if(header->N_li%2 == 1)
    ptr++;

  pdu->N_bytes = ptr-pdu->msg;
}

uint32_t rlc_am_packed_length(rlc_amd_pdu_header_t *header)
{
  uint32_t len = 2;                 // Fixed part is 2 bytes
  if(header->rf) len += 2;          // Segment header is 2 bytes
  len += header->N_li * 1.5 + 0.5;  // Extension part - integer rounding up
  return len;
}

bool rlc_am_is_status_pdu(srsue_byte_buffer_t *pdu)
{
  return ((*(pdu->msg) >> 7) & 0x01) == RLC_DC_FIELD_CONTROL_PDU;
}

} // namespace srsue
