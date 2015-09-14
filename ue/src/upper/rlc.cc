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

#include "upper/rlc.h"

using namespace srslte;

namespace srsue{

rlc::rlc()
  :bcch_bch_queue(2)
  ,bcch_dlsch_queue(2)
{}

void rlc::init(pdcp_interface_rlc *pdcp_,
               ue_interface *ue_,
               srslte::log *rlc_log_)
{
  pdcp    = pdcp_;
  ue      = ue_;
  rlc_log = rlc_log_;

  rlc_array[0].init(rlc_log, RLC_MODE_TM, 0); // SRB0
}

/*******************************************************************************
  PDCP interface
*******************************************************************************/
void rlc::write_sdu(uint32_t lcid, srsue_byte_buffer_t *sdu)
{
  if(valid_lcid(lcid)) {
    rlc_array[lcid].write_sdu(sdu);
  }
}

/*******************************************************************************
  MAC interface
*******************************************************************************/
uint32_t rlc::get_buffer_state(uint32_t lcid)
{
  if(valid_lcid(lcid)) {
    return rlc_array[lcid].get_buffer_state();
  } else {
    return 0;
  }
}

int rlc::read_pdu(uint32_t lcid, uint8_t *payload, uint32_t nof_bytes)
{
  if(valid_lcid(lcid)) {
    rlc_array[lcid].read_pdu(payload, nof_bytes);
  }
}

void rlc::write_pdu(uint32_t lcid, uint8_t *payload, uint32_t nof_bytes)
{
  if(valid_lcid(lcid)) {
    rlc_array[lcid].write_pdu(payload, nof_bytes);
  }
}

void rlc::write_pdu_bcch_bch(uint8_t *payload, uint32_t nof_bytes)
{
  rlc_log->info_hex(payload, nof_bytes, "BCCH BCH message received.");
  bcch_bch_queue.write(payload, nof_bytes);
  ue->notify();
}

void rlc::write_pdu_bcch_dlsch(uint8_t *payload, uint32_t nof_bytes)
{
  rlc_log->info_hex(payload, nof_bytes, "BCCH DLSCH message received.");
  bcch_dlsch_queue.write(payload, nof_bytes);
  ue->notify();
}

/*******************************************************************************
  RRC interface
*******************************************************************************/
void rlc::add_rlc(RLC_MODE_ENUM mode, uint32_t lcid, LIBLTE_RRC_RLC_CONFIG_STRUCT *cnfg)
{
  if(lcid < 0 || lcid >= SRSUE_N_RADIO_BEARERS) {
    rlc_log->error("Logical channel index must be in [0:%d] - %d", SRSUE_N_RADIO_BEARERS, lcid);
    return;
  }
  rlc_array[lcid].init(rlc_log, mode, lcid);
  if(cnfg)
    rlc_array[lcid].configure(cnfg);
}

/*******************************************************************************
  UE interface
*******************************************************************************/
bool rlc::check_retx_buffers()
{
  return false;
}

bool rlc::check_dl_buffers()
{
  bool ret = false;

  if(bcch_bch_queue.try_read(mac_buf))
  {
    pdcp->write_pdu_bcch_bch(&mac_buf);
    ret = true;
  }
  if(bcch_dlsch_queue.try_read(mac_buf))
  {
    pdcp->write_pdu_bcch_dlsch(&mac_buf);
    ret = true;
  }
  // TODO: Check each of the RB buffers

  return ret;
}

/*******************************************************************************
  Helpers
*******************************************************************************/
bool rlc::valid_lcid(uint32_t lcid)
{
  if(lcid < 0 || lcid >= SRSUE_N_RADIO_BEARERS) {
    return false;
  }
  if(!rlc_array[lcid].is_active()) {
    return false;
  }
  return true;
}


} // namespace srsue
