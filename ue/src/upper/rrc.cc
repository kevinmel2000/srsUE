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

void rrc::stop()
{}

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
      // Handle SIB1
      memcpy(&sib1, &dlsch_msg.sibs[0].sib.sib1, sizeof(LIBLTE_RRC_SYS_INFO_BLOCK_TYPE_1_STRUCT));
      rrc_log->info("SIB1 received, CellID=%d, si_window=%d, sib2_period=%d\n",
                    sib1.cell_id&0xfff,
                    liblte_rrc_si_window_length_num[sib1.si_window_length],
                    liblte_rrc_si_periodicity_num[sib1.sched_info[0].si_periodicity]);
      
      state = RRC_STATE_SIB2_SEARCH;
      mac->set_param(srsue::mac_interface_params::BCCH_SI_WINDOW_ST, -1);
      //TODO: Use all SIB1 info

    } else if (LIBLTE_RRC_SYS_INFO_BLOCK_TYPE_2 == dlsch_msg.sibs[0].sib_type && RRC_STATE_SIB2_SEARCH == state) {
      // Handle SIB2
      memcpy(&sib2, &dlsch_msg.sibs[0].sib.sib2, sizeof(LIBLTE_RRC_SYS_INFO_BLOCK_TYPE_2_STRUCT));
      rrc_log->info("SIB2 received, \n", pdu->N_bytes);
      state = RRC_STATE_WAIT_FOR_CON_SETUP;
      mac->set_param(srsue::mac_interface_params::BCCH_SI_WINDOW_ST, -1);
      handle_sib2();
      send_con_request();
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
  uint32_t  si_win_start, si_win_len;
  uint16_t  period;

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
      rrc_log->debug("Instructed MAC to search for SIB1, win_start=%d, win_len=%d\n",
                     si_win_start, 1);

      break;
    case RRC_STATE_SIB2_SEARCH:
      // Instruct MAC to look for SIB2
      usleep(10000);
      tti          = mac->get_current_tti();
      period       = liblte_rrc_si_periodicity_num[sib1.sched_info[0].si_periodicity];
      si_win_start = sib_start_tti(tti, period, 0);
      si_win_len   = liblte_rrc_si_window_length_num[sib1.si_window_length];

      mac->set_param(srsue::mac_interface_params::BCCH_SI_WINDOW_ST,  si_win_start);
      mac->set_param(srsue::mac_interface_params::BCCH_SI_WINDOW_LEN, si_win_len);
      rrc_log->debug("Instructed MAC to search for SIB2, win_start=%d, win_len=%d\n",
                     si_win_start, si_win_len);

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
  LIBLTE_RRC_UL_CCCH_MSG_STRUCT ul_ccch_msg;
  // Prepare ConnectionRequest packet
  ul_ccch_msg.msg_type = LIBLTE_RRC_UL_CCCH_MSG_TYPE_RRC_CON_REQ;
  ul_ccch_msg.msg.rrc_con_req.ue_id_type = LIBLTE_RRC_CON_REQ_UE_ID_TYPE_RANDOM_VALUE;
  ul_ccch_msg.msg.rrc_con_req.ue_id.random = 1000;
  ul_ccch_msg.msg.rrc_con_req.cause = LIBLTE_RRC_CON_REQ_EST_CAUSE_MO_SIGNALLING;
  liblte_rrc_pack_ul_ccch_msg(&ul_ccch_msg, (LIBLTE_BIT_MSG_STRUCT*)&bit_buf);

  // Pack the message bits for PDCP
  srslte_bit_pack_vector(bit_buf.msg, pdcp_buf.msg, bit_buf.N_bits);
  pdcp_buf.N_bytes = bit_buf.N_bits/8;

  // Set UE contention resolution ID in MAC
  uint64_t uecri=0;
  uint8_t *ue_cri_ptr = (uint8_t*) &uecri;
  uint32_t nbytes = 6;
  for (int i=0;i<nbytes;i++) {
    ue_cri_ptr[nbytes-i-1] = pdcp_buf.msg[i];
  }
  rrc_log->debug("Setting UE contention resolution ID: %d\n", uecri);
  mac->set_param(srsue::mac_interface_params::CONTENTION_ID, uecri);

  rrc_log->info("Sending RRC Connection Request on SRB0\n");
  state = RRC_STATE_WAIT_FOR_CON_SETUP;
  pdcp->write_sdu(SRSUE_RB_ID_SRB0, &pdcp_buf);
}

// Determine SI messages scheduling as in 36.331 5.2.3 Acquisition of an SI message
uint32_t rrc::sib_start_tti(uint32_t tti, uint32_t period, uint32_t x) {
  return (period*10*(1+tti/(period*10))+x)%10240; // the 1 means next opportunity
}

void rrc::handle_sib2()
{
  if(RRC_STATE_WAIT_FOR_CON_SETUP != state){
    rrc_log->error("State must be RRC_STATE_WAIT_FOR_CON_SETUP to handle SIB2. Actual state: %s\n",
                   rrc_state_text[state]);
    return;
  }

  // RACH-CONFIGCOMMON
  if (sib2.rr_config_common_sib.rach_cnfg.preambles_group_a_cnfg.present) {
    mac->set_param(srsue::mac_interface_params::RA_NOFGROUPAPREAMBLES,
                   liblte_rrc_message_size_group_a_num[sib2.rr_config_common_sib.rach_cnfg.preambles_group_a_cnfg.size_of_ra]);
    mac->set_param(srsue::mac_interface_params::RA_MESSAGESIZEA,
                   liblte_rrc_message_size_group_a_num[sib2.rr_config_common_sib.rach_cnfg.preambles_group_a_cnfg.msg_size]);
    mac->set_param(srsue::mac_interface_params::RA_MESSAGEPOWEROFFSETB,
                   liblte_rrc_message_power_offset_group_b_num[sib2.rr_config_common_sib.rach_cnfg.preambles_group_a_cnfg.msg_pwr_offset_group_b]);
  }
  mac->set_param(srsue::mac_interface_params::RA_NOFPREAMBLES,
                 liblte_rrc_number_of_ra_preambles_num[sib2.rr_config_common_sib.rach_cnfg.num_ra_preambles]);
  mac->set_param(srsue::mac_interface_params::RA_POWERRAMPINGSTEP,
                 liblte_rrc_power_ramping_step_num[sib2.rr_config_common_sib.rach_cnfg.pwr_ramping_step]);
  mac->set_param(srsue::mac_interface_params::RA_INITRECEIVEDPOWER,
                 liblte_rrc_preamble_initial_received_target_power_num[sib2.rr_config_common_sib.rach_cnfg.preamble_init_rx_target_pwr]);
  mac->set_param(srsue::mac_interface_params::RA_PREAMBLETRANSMAX,
                 liblte_rrc_preamble_trans_max_num[sib2.rr_config_common_sib.rach_cnfg.preamble_trans_max]);
  mac->set_param(srsue::mac_interface_params::RA_RESPONSEWINDOW,
                 liblte_rrc_ra_response_window_size_num[sib2.rr_config_common_sib.rach_cnfg.ra_resp_win_size]);
  mac->set_param(srsue::mac_interface_params::RA_CONTENTIONTIMER,
                 liblte_rrc_mac_contention_resolution_timer_num[sib2.rr_config_common_sib.rach_cnfg.mac_con_res_timer]);
  mac->set_param(srsue::mac_interface_params::HARQ_MAXMSG3TX,
                 sib2.rr_config_common_sib.rach_cnfg.max_harq_msg3_tx);

  rrc_log->info("Set RACH ConfigCommon: NofPreambles=%d, ResponseWindow=%d, ContentionResolutionTimer=%d ms\n",
         liblte_rrc_number_of_ra_preambles_num[sib2.rr_config_common_sib.rach_cnfg.num_ra_preambles],
         liblte_rrc_ra_response_window_size_num[sib2.rr_config_common_sib.rach_cnfg.ra_resp_win_size],
         liblte_rrc_mac_contention_resolution_timer_num[sib2.rr_config_common_sib.rach_cnfg.mac_con_res_timer]);

  // PDSCH ConfigCommon
  mac->set_param(srsue::mac_interface_params::PDSCH_RSPOWER,
                 sib2.rr_config_common_sib.pdsch_cnfg.rs_power);
  mac->set_param(srsue::mac_interface_params::PDSCH_PB,
                 sib2.rr_config_common_sib.pdsch_cnfg.p_b);

  // PUSCH ConfigCommon
  phy->set_param(srsue::phy_interface_params::PUSCH_EN_64QAM,
                 sib2.rr_config_common_sib.pusch_cnfg.enable_64_qam);
  phy->set_param(srsue::phy_interface_params::PUSCH_HOPPING_OFFSET,
                 sib2.rr_config_common_sib.pusch_cnfg.pusch_hopping_offset);
  phy->set_param(srsue::phy_interface_params::PUSCH_HOPPING_N_SB,
                 sib2.rr_config_common_sib.pusch_cnfg.n_sb);
  phy->set_param(srsue::phy_interface_params::PUSCH_HOPPING_INTRA_SF,
                 sib2.rr_config_common_sib.pusch_cnfg.hopping_mode == LIBLTE_RRC_HOPPING_MODE_INTRA_AND_INTER_SUBFRAME?1:0);
  phy->set_param(srsue::phy_interface_params::DMRS_GROUP_HOPPING_EN,
                 sib2.rr_config_common_sib.pusch_cnfg.ul_rs.group_hopping_enabled?1:0);
  phy->set_param(srsue::phy_interface_params::DMRS_SEQUENCE_HOPPING_EN,
                 sib2.rr_config_common_sib.pusch_cnfg.ul_rs.sequence_hopping_enabled?1:0);
  phy->set_param(srsue::phy_interface_params::PUSCH_RS_CYCLIC_SHIFT,
                 sib2.rr_config_common_sib.pusch_cnfg.ul_rs.cyclic_shift);
  phy->set_param(srsue::phy_interface_params::PUSCH_RS_GROUP_ASSIGNMENT,
                 sib2.rr_config_common_sib.pusch_cnfg.ul_rs.group_assignment_pusch);

  rrc_log->info("Set PUSCH ConfigCommon: HopOffset=%d, RSGroup=%d, RSNcs=%d, N_sb=%d\n",
    sib2.rr_config_common_sib.pusch_cnfg.pusch_hopping_offset,
    sib2.rr_config_common_sib.pusch_cnfg.ul_rs.group_assignment_pusch,
    sib2.rr_config_common_sib.pusch_cnfg.ul_rs.cyclic_shift,
    sib2.rr_config_common_sib.pusch_cnfg.n_sb);

  // PUCCH ConfigCommon
  phy->set_param(srsue::phy_interface_params::PUCCH_DELTA_SHIFT,
                 liblte_rrc_delta_pucch_shift_num[sib2.rr_config_common_sib.pucch_cnfg.delta_pucch_shift]);
  phy->set_param(srsue::phy_interface_params::PUCCH_CYCLIC_SHIFT,
                 sib2.rr_config_common_sib.pucch_cnfg.n_cs_an);
  phy->set_param(srsue::phy_interface_params::PUCCH_N_PUCCH_1,
                 sib2.rr_config_common_sib.pucch_cnfg.n1_pucch_an);
  phy->set_param(srsue::phy_interface_params::PUCCH_N_RB_2,
                 sib2.rr_config_common_sib.pucch_cnfg.n_rb_cqi);

  rrc_log->info("Set PUCCH ConfigCommon: DeltaShift=%d, CyclicShift=%d, N1=%d, NRB=%d\n",
         liblte_rrc_delta_pucch_shift_num[sib2.rr_config_common_sib.pucch_cnfg.delta_pucch_shift],
         sib2.rr_config_common_sib.pucch_cnfg.n_cs_an,
         sib2.rr_config_common_sib.pucch_cnfg.n1_pucch_an,
         sib2.rr_config_common_sib.pucch_cnfg.n_rb_cqi);

  // PRACH Configcommon
  phy->set_param(srsue::phy_interface_params::PRACH_ROOT_SEQ_IDX,
                 sib2.rr_config_common_sib.prach_cnfg.root_sequence_index);
  phy->set_param(srsue::phy_interface_params::PRACH_HIGH_SPEED_FLAG,
                 sib2.rr_config_common_sib.prach_cnfg.prach_cnfg_info.high_speed_flag?1:0);
  phy->set_param(srsue::phy_interface_params::PRACH_FREQ_OFFSET,
                 sib2.rr_config_common_sib.prach_cnfg.prach_cnfg_info.prach_freq_offset);
  phy->set_param(srsue::phy_interface_params::PRACH_ZC_CONFIG,
                 sib2.rr_config_common_sib.prach_cnfg.prach_cnfg_info.zero_correlation_zone_config);
  phy->set_param(srsue::phy_interface_params::PRACH_CONFIG_INDEX,
                 sib2.rr_config_common_sib.prach_cnfg.prach_cnfg_info.prach_config_index);

  rrc_log->info("Set PRACH ConfigCommon: SeqIdx=%d, HS=%d, FreqOffset=%d, ZC=%d, ConfigIndex=%d\n",
     sib2.rr_config_common_sib.prach_cnfg.root_sequence_index,
     sib2.rr_config_common_sib.prach_cnfg.prach_cnfg_info.high_speed_flag?1:0,
     sib2.rr_config_common_sib.prach_cnfg.prach_cnfg_info.prach_freq_offset,
     sib2.rr_config_common_sib.prach_cnfg.prach_cnfg_info.zero_correlation_zone_config,
     sib2.rr_config_common_sib.prach_cnfg.prach_cnfg_info.prach_config_index);

  // SRS ConfigCommon
  if (sib2.rr_config_common_sib.srs_ul_cnfg.present) {
    phy->set_param(srsue::phy_interface_params::SRS_CS_BWCFG, sib2.rr_config_common_sib.srs_ul_cnfg.bw_cnfg);
    phy->set_param(srsue::phy_interface_params::SRS_CS_SFCFG, sib2.rr_config_common_sib.srs_ul_cnfg.subfr_cnfg);
    phy->set_param(srsue::phy_interface_params::SRS_CS_ACKNACKSIMUL, sib2.rr_config_common_sib.srs_ul_cnfg.ack_nack_simul_tx);
  }

  rrc_log->info("Set SRS ConfigCommon: BW-Configuration=%d, SF-Configuration=%d, ACKNACK=%d\n",
    sib2.rr_config_common_sib.srs_ul_cnfg.bw_cnfg,
    sib2.rr_config_common_sib.srs_ul_cnfg.subfr_cnfg,
    sib2.rr_config_common_sib.srs_ul_cnfg.ack_nack_simul_tx);

  phy->configure_ul_params();
}

} // namespace srsue
