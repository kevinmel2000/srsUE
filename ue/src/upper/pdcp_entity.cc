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

#include "upper/pdcp_entity.h"

using namespace srslte;

namespace srsue{

pdcp_entity::pdcp_entity()
  :active(false)
{}

void pdcp_entity::init(rlc_interface_pdcp *rlc_,
                       rrc_interface_pdcp *rrc_,
                       gw_interface_pdcp  *gw_,
                       srslte::log        *log_,
                       uint32_t            lcid_)
{
  rlc     = rlc_;
  rrc     = rrc_;
  gw      = gw_;
  log     = log_;
  lcid    = lcid_;
  active  = true;
}

bool pdcp_entity::is_active()
{
  return active;
}

void pdcp_entity::write_sdu(srsue_byte_buffer_t *sdu)
{
  // Handle SRB messages
  switch(lcid)
  {
  case SRSUE_RB_ID_SRB0:
    handle_srb0_sdu(sdu);
    break;
  case SRSUE_RB_ID_SRB1:
  case SRSUE_RB_ID_SRB2:
    break;
  }

  // Handle DRB messages
  if(lcid >= SRSUE_RB_ID_DRB1)
  {

  }
}
void pdcp_entity::write_pdu(srsue_byte_buffer_t *pdu)
{
  // Handle SRB messages
  switch(lcid)
  {
  case SRSUE_RB_ID_SRB0:
    handle_srb0_pdu(pdu);
    break;
  case SRSUE_RB_ID_SRB1:
  case SRSUE_RB_ID_SRB2:
    break;
  }

  // Handle DRB messages
  if(lcid >= SRSUE_RB_ID_DRB1)
  {

  }
}

void pdcp_entity::handle_srb0_sdu(srsue_byte_buffer_t *sdu)
{
  // Simply pass on to RLC
  log->info_hex(sdu->msg, sdu->N_bytes, "UL Bearer %s PDU", srsue_rb_id_text[lcid]);
  rlc->write_sdu(lcid, sdu);
}

void pdcp_entity::handle_srb0_pdu(srsue_byte_buffer_t *pdu)
{
  // Simply pass on to RRC
  rrc->write_pdu(pdu);
}

}
