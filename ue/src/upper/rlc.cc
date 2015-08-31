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

rlc::rlc(srslte::log *rlc_log_)
  :rlc_log(rlc_log_)
{
  rlc_array[0].init(rlc_log, RLC_MODE_TM, 0); // SRB0
}

void rlc::init(pdcp_interface_rlc *pdcp_)
{
  pdcp = pdcp_;
}

/*******************************************************************************
  PDCP interface
*******************************************************************************/
void rlc::write_sdu(uint32_t lcid, uint8_t *payload, uint32_t nof_bytes)
{
  if(valid_lcid(lcid)) {
    rlc_array[lcid].write_sdu(payload, nof_bytes);
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
  Helpers
*******************************************************************************/
bool rlc::valid_lcid(uint32_t lcid)
{
  if(lcid < 0 || lcid >= SRSUE_N_RADIO_BEARERS) {
    rlc_log->error("Logical channel index must be in [0:%d] - %d", SRSUE_N_RADIO_BEARERS, lcid);
    return false;
  }
  if(!rlc_array[lcid].is_active()) {
    rlc_log->error("RLC entity for logical channel %d has not been activated", lcid);
    return false;
  }
  return true;
}


} // namespace srsue
