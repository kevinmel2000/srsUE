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

#include "upper/pdcp.h"

using namespace srslte;

namespace srsue{

pdcp::pdcp()
{}

void pdcp::init(rlc_interface_pdcp *rlc_, rrc_interface_pdcp *rrc_, gw_interface_pdcp *gw_, srslte::log *pdcp_log_)
{
  rlc       = rlc_;
  rrc       = rrc_;
  gw        = gw_;
  pdcp_log  = pdcp_log_;

  pdcp_array[0].init(rlc, rrc, gw, pdcp_log, SRSUE_RB_ID_SRB0); // SRB0
}

void pdcp::stop()
{}

/*******************************************************************************
  RRC interface
*******************************************************************************/
void pdcp::write_sdu(uint32_t lcid, srsue_byte_buffer_t *sdu)
{
  if(valid_lcid(lcid))
    pdcp_array[lcid].write_sdu(sdu);
}

void pdcp::add_bearer(uint32_t lcid)
{
  if(lcid < 0 || lcid >= SRSUE_N_RADIO_BEARERS) {
    pdcp_log->error("Radio bearer id must be in [0:%d] - %d\n", SRSUE_N_RADIO_BEARERS, lcid);
    return;
  }
  pdcp_array[lcid].init(rlc, rrc, gw, pdcp_log, lcid);
  pdcp_log->info("Added bearer %s\n", srsue_rb_id_text[lcid]);
}

/*******************************************************************************
  RLC interface
*******************************************************************************/
void pdcp::write_pdu(uint32_t lcid, srsue_byte_buffer_t *pdu)
{
  if(valid_lcid(lcid))
    pdcp_array[lcid].write_pdu(pdu);
}

void pdcp::write_pdu_bcch_bch(srsue_byte_buffer_t *sdu)
{
  rrc->write_pdu_bcch_bch(sdu);
}
void pdcp::write_pdu_bcch_dlsch(srsue_byte_buffer_t *sdu)
{
  rrc->write_pdu_bcch_dlsch(sdu);
}

/*******************************************************************************
  Helpers
*******************************************************************************/
bool pdcp::valid_lcid(uint32_t lcid)
{
  if(lcid < 0 || lcid >= SRSUE_N_RADIO_BEARERS) {
    pdcp_log->error("Radio bearer id must be in [0:%d] - %d", SRSUE_N_RADIO_BEARERS, lcid);
    return false;
  }
  if(!pdcp_array[lcid].is_active()) {
    pdcp_log->error("RLC entity for logical channel %d has not been activated", lcid);
    return false;
  }
  return true;
}

} // namespace srsue
