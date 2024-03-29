/**
 *
 * \section COPYRIGHT
 *
 * Copyright 2013-2015 Software Radio Systems Limited
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
#include "upper/rlc_tm.h"
#include "upper/rlc_um.h"
#include "upper/rlc_am.h"

using namespace srslte;

namespace srsue{

rlc::rlc()
{
  pool = buffer_pool::get_instance();
}

void rlc::init(pdcp_interface_rlc *pdcp_,
               rrc_interface_rlc  *rrc_,
               ue_interface       *ue_,
               srslte::log        *rlc_log_, 
               mac_interface_timers *mac_timers_)
{
  pdcp    = pdcp_;
  rrc     = rrc_;
  ue      = ue_;
  rlc_log = rlc_log_;
  mac_timers = mac_timers_;

  for(uint32_t i=0;i<SRSUE_N_RADIO_BEARERS;i++)
  {
    rlc_array[i] = NULL;
  }

  rlc_array[0] = new rlc_tm;
  rlc_array[0]->init(rlc_log, RB_ID_SRB0, pdcp, rrc, mac_timers); // SRB0
}

void rlc::stop()
{}

/*******************************************************************************
  PDCP interface
*******************************************************************************/
void rlc::write_sdu(uint32_t lcid, byte_buffer_t *sdu)
{
  if(valid_lcid(lcid)) {
    rlc_array[lcid]->write_sdu(sdu);
  }
}

/*******************************************************************************
  MAC interface
*******************************************************************************/
uint32_t rlc::get_buffer_state(uint32_t lcid)
{
  if(valid_lcid(lcid)) {
    return rlc_array[lcid]->get_buffer_state();
  } else {
    return 0;
  }
}

int rlc::read_pdu(uint32_t lcid, uint8_t *payload, uint32_t nof_bytes)
{
  if(valid_lcid(lcid)) {
    return rlc_array[lcid]->read_pdu(payload, nof_bytes);
  }
}

void rlc::write_pdu(uint32_t lcid, uint8_t *payload, uint32_t nof_bytes)
{
  if(valid_lcid(lcid)) {
    rlc_array[lcid]->write_pdu(payload, nof_bytes);
  }
}

void rlc::write_pdu_bcch_bch(uint8_t *payload, uint32_t nof_bytes)
{
  rlc_log->info_hex(payload, nof_bytes, "BCCH BCH message received.");
  byte_buffer_t *buf = pool->allocate();
  memcpy(buf->msg, payload, nof_bytes);
  buf->N_bytes = nof_bytes;
  pdcp->write_pdu_bcch_bch(buf);
}

void rlc::write_pdu_bcch_dlsch(uint8_t *payload, uint32_t nof_bytes)
{
  rlc_log->info_hex(payload, nof_bytes, "BCCH DLSCH message received.");
  byte_buffer_t *buf = pool->allocate();
  memcpy(buf->msg, payload, nof_bytes);
  buf->N_bytes = nof_bytes;
  pdcp->write_pdu_bcch_dlsch(buf);
}

/*******************************************************************************
  RRC interface
*******************************************************************************/
void rlc::add_bearer(uint32_t lcid)
{
  // No config provided - use defaults for lcid
  LIBLTE_RRC_RLC_CONFIG_STRUCT cnfg;
  if(RB_ID_SRB1 == lcid || RB_ID_SRB2 == lcid)
  {
    cnfg.rlc_mode                     = LIBLTE_RRC_RLC_MODE_AM;
    cnfg.ul_am_rlc.t_poll_retx        = LIBLTE_RRC_T_POLL_RETRANSMIT_MS45;
    cnfg.ul_am_rlc.poll_pdu           = LIBLTE_RRC_POLL_PDU_INFINITY;
    cnfg.ul_am_rlc.poll_byte          = LIBLTE_RRC_POLL_BYTE_INFINITY;
    cnfg.ul_am_rlc.max_retx_thresh    = LIBLTE_RRC_MAX_RETX_THRESHOLD_T4;
    cnfg.dl_am_rlc.t_reordering       = LIBLTE_RRC_T_REORDERING_MS35;
    cnfg.dl_am_rlc.t_status_prohibit  = LIBLTE_RRC_T_STATUS_PROHIBIT_MS0;
    add_bearer(lcid, &cnfg);
  }else{
    rlc_log->error("Radio bearer %s does not support default RLC configuration.",
                   rb_id_text[lcid]);
  }
}

void rlc::add_bearer(uint32_t lcid, LIBLTE_RRC_RLC_CONFIG_STRUCT *cnfg)
{
  if(lcid < 0 || lcid >= SRSUE_N_RADIO_BEARERS) {
    rlc_log->error("Radio bearer id must be in [0:%d] - %d\n", SRSUE_N_RADIO_BEARERS, lcid);
    return;
  }else{
    rlc_log->info("Adding radio bearer %s with mode %s\n",
                  rb_id_text[lcid], liblte_rrc_rlc_mode_text[cnfg->rlc_mode]);
  }

  switch(cnfg->rlc_mode)
  {
  case LIBLTE_RRC_RLC_MODE_AM:
    rlc_array[lcid] = new rlc_am;
    break;
  case LIBLTE_RRC_RLC_MODE_UM_BI:
    rlc_array[lcid] = new rlc_um;
    break;
  case LIBLTE_RRC_RLC_MODE_UM_UNI_UL:
    rlc_array[lcid] = new rlc_um;
    break;
  case LIBLTE_RRC_RLC_MODE_UM_UNI_DL:
    rlc_array[lcid] = new rlc_um;
    break;
  default:
    rlc_log->error("Cannot add RLC entity - invalid mode\n");
    return;
  }
  rlc_array[lcid]->init(rlc_log, lcid, pdcp, rrc, mac_timers);
  if(cnfg)
    rlc_array[lcid]->configure(cnfg);

}

/*******************************************************************************
  Helpers
*******************************************************************************/
bool rlc::valid_lcid(uint32_t lcid)
{
  if(lcid < 0 || lcid >= SRSUE_N_RADIO_BEARERS) {
    return false;
  }
  if(!rlc_array[lcid]) {
    return false;
  }
  return true;
}


} // namespace srsue
