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

#ifndef RRC_H
#define RRC_H

#include "pthread.h"

#include "common/buffer_pool.h"
#include "common/log.h"
#include "common/common.h"
#include "common/interfaces.h"

namespace srsue {

// RRC states (3GPP 36.331 v10.0.0)
typedef enum{
    RRC_STATE_IDLE = 0,
    RRC_STATE_SIB1_SEARCH,
    RRC_STATE_SIB2_SEARCH,
    RRC_STATE_WAIT_FOR_CON_SETUP,
    RRC_STATE_COMPLETING_SETUP,
    RRC_STATE_RRC_CONNECTED,
    RRC_STATE_N_ITEMS,
}rrc_state_t;
static const char rrc_state_text[RRC_STATE_N_ITEMS][100] = {"IDLE",
                                                            "SIB1_SEARCH",
                                                            "SIB2_SEARCH",
                                                            "WAIT FOR CON SETUP",
                                                            "COMPLETING SETUP",
                                                            "RRC CONNECTED"};


class rrc
    :public rrc_interface_nas
    ,public rrc_interface_pdcp
    ,public rrc_interface_rlc
{
public:
  rrc();
  void init(phy_interface_rrc     *phy_,
            mac_interface_rrc     *mac_,
            rlc_interface_rrc     *rlc_,
            pdcp_interface_rrc    *pdcp_,
            nas_interface_rrc     *nas_,
            srslte::log           *rrc_log_);
  void stop();

private:
  buffer_pool           *pool;
  srslte::log           *rrc_log;
  phy_interface_rrc     *phy;
  mac_interface_rrc     *mac;
  rlc_interface_rrc     *rlc;
  pdcp_interface_rrc    *pdcp;
  nas_interface_rrc     *nas;

  bit_buffer_t    bit_buf;

  rrc_state_t           state;
  uint8_t               transaction_id;

  LIBLTE_RRC_MIB_STRUCT mib;
  LIBLTE_RRC_SYS_INFO_BLOCK_TYPE_1_STRUCT sib1;
  LIBLTE_RRC_SYS_INFO_BLOCK_TYPE_2_STRUCT sib2;


  pthread_t             sib_search_thread;

  // NAS interface
  void write_sdu(uint32_t lcid, byte_buffer_t *sdu);

  // PDCP interface
  void write_pdu(uint32_t lcid, byte_buffer_t *pdu);
  void write_pdu_bcch_bch(byte_buffer_t *pdu);
  void write_pdu_bcch_dlsch(byte_buffer_t *pdu);

  // RLC interface
  void max_retx_attempted();

  // Senders
  void send_con_request();
  void send_con_setup_complete(byte_buffer_t *nas_msg);
  void send_ul_info_transfer(uint32_t lcid, byte_buffer_t *sdu);

  // Parsers
  void parse_dl_ccch(byte_buffer_t *pdu);
  void parse_dl_dcch(byte_buffer_t *pdu);

  // Helpers
  static void*  start_sib_thread(void *rrc_);
  void          sib_search();
  uint32_t      sib_start_tti(uint32_t tti, uint32_t period, uint32_t x);
  void          apply_sib2_configs();
  void          handle_con_setup(LIBLTE_RRC_CONNECTION_SETUP_STRUCT *setup);
  void          add_srb(LIBLTE_RRC_SRB_TO_ADD_MOD_STRUCT *srb_cnfg);
  void          add_drb(LIBLTE_RRC_DRB_TO_ADD_MOD_STRUCT *drb_cnfg);

};

} // namespace srsue


#endif // RRC_H
