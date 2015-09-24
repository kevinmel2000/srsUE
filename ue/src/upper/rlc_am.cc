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

void rlc_am::init(srslte::log *log_, uint32_t lcid_)
{
  log  = log_;
  lcid = lcid_;
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

RLC_MODE_ENUM rlc_am::get_mode()
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
  ul_queue.write(sdu);
}

bool rlc_am::try_read_sdu(srsue_byte_buffer_t *sdu)
{
  return dl_queue.try_read(sdu);
}

// MAC interface
uint32_t rlc_am::get_buffer_state(){return 0;}
int      rlc_am::read_pdu(uint8_t *payload, uint32_t nof_bytes){}
void     rlc_am:: write_pdu(uint8_t *payload, uint32_t nof_bytes){}

}
