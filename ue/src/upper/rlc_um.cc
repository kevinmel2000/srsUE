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

#include "upper/rlc_um.h"

using namespace srslte;

namespace srsue{

rlc_um::rlc_um()
{}

void rlc_um::init(srslte::log        *log_,
                  uint32_t            lcid_,
                  pdcp_interface_rlc *pdcp_,
                  rrc_interface_rlc  *rrc_)
{
  log  = log_;
  lcid = lcid_;
  pdcp = pdcp_;
  rrc  = rrc_;
}

void rlc_um::configure(LIBLTE_RRC_RLC_CONFIG_STRUCT *cnfg)
{
  //TODO
}

rlc_mode_t rlc_um::get_mode()
{
  return RLC_MODE_UM;
}

uint32_t rlc_um::get_bearer()
{
  return lcid;
}

// PDCP interface
void rlc_um::write_sdu(srsue_byte_buffer_t *sdu){}
bool rlc_um::read_sdu(){}

// MAC interface
uint32_t rlc_um::get_buffer_state(){return 0;}
int      rlc_um::read_pdu(uint8_t *payload, uint32_t nof_bytes){}
void     rlc_um:: write_pdu(uint8_t *payload, uint32_t nof_bytes){}

}
