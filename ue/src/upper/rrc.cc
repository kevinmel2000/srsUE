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

#include <unistd.h>

#include "upper/rrc.h"
#include <srslte/utils/bit.h>

using namespace srslte;

namespace srsue{

rrc::rrc()
  :state(RRC_STATE_IDLE)
{}

void rrc::init(phy_interface_rrc     *phy_,
               mac_interface_rrc     *mac_,
               rlc_interface_rrc     *rlc_,
               pdcp_interface_rrc    *pdcp_,
               nas_interface_rrc     *nas_,
               srslte::log           *rrc_log_)
{
  phy     = phy_;
  mac     = mac_;
  rlc     = rlc_;
  pdcp    = pdcp_;
  nas     = nas_;
  rrc_log = rrc_log_;
}

void rrc::write_pdu(srsue_byte_buffer_t *pdu)
{}

void rrc::write_pdu_bcch_bch(srsue_byte_buffer_t *pdu)
{
  // Unpack the MIB
  rrc_log->info_hex(pdu->msg, pdu->N_bytes, "BCCH BCH message received.");
  srslte_bit_unpack_vector(pdu->msg, bit_buf.msg, pdu->N_bytes*8);
  bit_buf.N_bits = pdu->N_bytes*8;
  liblte_rrc_unpack_bcch_bch_msg((LIBLTE_BIT_MSG_STRUCT*)&bit_buf, &mib);
  rrc_log->info("MIB received BW=%s MHz\n", liblte_rrc_dl_bandwidth_text[mib.dl_bw]);

  // Start the SIB search state machine
  state = RRC_STATE_SIB1_SEARCH;
  pthread_create(&sib_search_thread, NULL, &rrc::start_sib_thread, this);
}

void rrc::write_pdu_bcch_dlsch(srsue_byte_buffer_t *pdu)
{
  rrc_log->info_hex(pdu->msg, pdu->N_bytes, "BCCH DLSCH message received.");
  LIBLTE_RRC_BCCH_DLSCH_MSG_STRUCT dlsch_msg;
  srslte_bit_unpack_vector(pdu->msg, bit_buf.msg, pdu->N_bytes*8);
  bit_buf.N_bits = pdu->N_bytes*8;
  liblte_rrc_unpack_bcch_dlsch_msg((LIBLTE_BIT_MSG_STRUCT*)&bit_buf, &dlsch_msg);

  if (dlsch_msg.N_sibs > 0) {
    if (LIBLTE_RRC_SYS_INFO_BLOCK_TYPE_1 == dlsch_msg.sibs[0].sib_type && RRC_STATE_SIB1_SEARCH == state) {
      memcpy(&sib1, &dlsch_msg.sibs[0].sib.sib1, sizeof(LIBLTE_RRC_SYS_INFO_BLOCK_TYPE_1_STRUCT));
      rrc_log->info("SIB1 received, CellID=%d, si_window=%d, sib2_period=%d\n",
                    dlsch_msg.sibs[0].sib.sib1.cell_id&0xfff,
                    sib1.si_window_length,
                    sib1.sched_info[0].si_periodicity);
      state = RRC_STATE_SIB2_SEARCH;
      mac->set_param(srsue::mac_interface_params::BCCH_SI_WINDOW_ST, -1);
      //TODO: use SIB1 info or store it

    } else if (LIBLTE_RRC_SYS_INFO_BLOCK_TYPE_2 == dlsch_msg.sibs[0].sib_type && RRC_STATE_SIB2_SEARCH == state) {
      rrc_log->info("SIB2 received %d bytes\n", pdu->N_bytes);
      state = RRC_STATE_WAIT_FOR_CON_SETUP;
      mac->set_param(srsue::mac_interface_params::BCCH_SI_WINDOW_ST, -1);
      //TODO: Use SIB2 info or store it

    }
  }
}

void* rrc::start_sib_thread(void *rrc_)
{
  rrc *r = (rrc*)rrc_;
  r->sib_search();
}

void rrc::sib_search()
{
  bool      searching = true;
  uint32_t  tti ;
  uint32_t  si_win_start;

  while(searching)
  {
    switch(state)
    {
    case RRC_STATE_SIB1_SEARCH:
      // Instruct MAC to look for SIB1
      while(!phy->status_is_sync()){
        usleep(50000);
      }
      usleep(10000);
      tti          = mac->get_current_tti();
      si_win_start = sib_start_tti(tti, 2, 5);
      mac->set_param(srsue::mac_interface_params::BCCH_SI_WINDOW_ST, si_win_start);
      mac->set_param(srsue::mac_interface_params::BCCH_SI_WINDOW_LEN, 1);
      rrc_log->debug("Instructed MAC to search for SIB1, win_start=%d, win_len=%d\n", si_win_start, 1);

      break;
    case RRC_STATE_SIB2_SEARCH:
      // Instruct MAC to look for SIB2
      usleep(10000);
      tti          = mac->get_current_tti();
      si_win_start = sib_start_tti(tti, sib1.sched_info[0].si_periodicity, 0);
      mac->set_param(srsue::mac_interface_params::BCCH_SI_WINDOW_ST, si_win_start);
      mac->set_param(srsue::mac_interface_params::BCCH_SI_WINDOW_LEN, sib1.si_window_length);
      rrc_log->debug("Instructed MAC to search for SIB2, win_start=%d, win_len=%d\n", si_win_start, sib1.si_window_length);

      break;
    default:
      searching = false;
      break;
    }
    usleep(100000);
  }
}

void rrc::send_con_request()
{
  LIBLTE_RRC_CONNECTION_REQUEST_STRUCT msg;

  msg.cause         = LIBLTE_RRC_CON_REQ_EST_CAUSE_MO_SIGNALLING;
  msg.ue_id_type    = LIBLTE_RRC_CON_REQ_UE_ID_TYPE_RANDOM_VALUE;
  msg.ue_id.random  = rand();
  liblte_rrc_pack_rrc_connection_request_msg(&msg, (LIBLTE_BIT_MSG_STRUCT*)&bit_buf);

  rrc_log->info("Sending RRC Connection Request on SRB0");
  state = RRC_STATE_WAIT_FOR_CON_SETUP;

  // Pack the message bits for PDCP and pass on
  srslte_bit_pack_vector(bit_buf.msg, pdcp_buf.msg, bit_buf.N_bits);
  pdcp_buf.N_bytes = bit_buf.N_bits/8;
  pdcp->write_sdu(SRSUE_RB_ID_SRB0, &pdcp_buf);
}

// Determine SI messages scheduling as in 36.331 5.2.3 Acquisition of an SI message
uint32_t rrc::sib_start_tti(uint32_t tti, uint32_t period, uint32_t x) {
  return (period*10*(1+tti/(period*10))+x)%10240; // the 1 means next opportunity
}

} // namespace srsue
