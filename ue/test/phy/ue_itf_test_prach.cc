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

#include <unistd.h>

#include "srslte/utils/debug.h"
#include "phy/phy.h"
#include "common/log_stdout.h"
#include "radio/radio_uhd.h"

/**********************************************************************
 *  Program arguments processing
 ***********************************************************************/
typedef struct {
  float uhd_rx_freq;
  float uhd_tx_freq; 
  float uhd_rx_gain;
  float uhd_tx_gain;
  bool  continous; 
}prog_args_t;

prog_args_t prog_args; 
uint32_t srsapps_verbose = 0; 

void args_default(prog_args_t *args) {
  args->uhd_rx_freq = -1.0;
  args->uhd_tx_freq = -1.0;
  args->uhd_rx_gain = -1; // set to autogain
  args->uhd_tx_gain = -1; 
  args->continous = false; 
}

void usage(prog_args_t *args, char *prog) {
  printf("Usage: %s [gGcv] -f rx_frequency -F tx_frequency (in Hz)\n", prog);
  printf("\t-g UHD RX gain [Default AGC]\n");
  printf("\t-G UHD TX gain [Default same as RX gain (AGC)]\n");
  printf("\t-c Run continuously [Default only once]\n");
  printf("\t-v [increase verbosity, default none]\n");
}

void parse_args(prog_args_t *args, int argc, char **argv) {
  int opt;
  args_default(args);
  while ((opt = getopt(argc, argv, "gGfFcv")) != -1) {
    switch (opt) {
    case 'g':
      args->uhd_rx_gain = atof(argv[optind]);
      break;
    case 'G':
      args->uhd_tx_gain = atof(argv[optind]);
      break;
    case 'f':
      args->uhd_rx_freq = atof(argv[optind]);
      break;
    case 'F':
      args->uhd_tx_freq = atof(argv[optind]);
      break;
    case 'c':
      args->continous = true; 
      break;
    case 'v':
      srsapps_verbose++;
      break;
    default:
      usage(args, argv[0]);
      exit(-1);
    }
  }
  if (args->uhd_rx_freq < 0 || args->uhd_tx_freq < 0) {
    usage(args, argv[0]);
    exit(-1);
  }
}



typedef enum{
    rar_header_type_bi = 0,
    rar_header_type_rapid,
    rar_header_type_n_items,
}rar_header_t;
static const char rar_header_text[rar_header_type_n_items][8] = {"BI", "RAPID"};

typedef struct {
  rar_header_t      hdr_type;
  bool              hopping_flag;
  uint32_t          tpc_command;
  bool              ul_delay;
  bool              csi_req;
  uint16_t          rba; 
  uint16_t          timing_adv_cmd;
  uint16_t          temp_c_rnti;
  uint8_t           mcs; 
  uint8_t           RAPID;
  uint8_t           BI;
}rar_msg_t; 


int rar_unpack(uint8_t *buffer, rar_msg_t *msg)
{
    int ret = SRSLTE_ERROR;
    uint8_t *ptr = buffer; 
    
    if(buffer != NULL &&
          msg != NULL)
    {
      ptr++;
      msg->hdr_type = (rar_header_t) *ptr++;
      if(msg->hdr_type == rar_header_type_bi) {
        ptr += 2; 
        msg->BI = srslte_bit_pack(&ptr, 4);
        ret = SRSLTE_SUCCESS; 
      } else if (msg->hdr_type == rar_header_type_rapid) {
        msg->RAPID = srslte_bit_pack(&ptr, 6);
        ptr++;
        
        msg->timing_adv_cmd = srslte_bit_pack(&ptr, 11);
        msg->hopping_flag   = *ptr++;
        msg->rba            = srslte_bit_pack(&ptr, 10); 
        msg->mcs            = srslte_bit_pack(&ptr, 4);
        msg->tpc_command    = srslte_bit_pack(&ptr, 3);
        msg->ul_delay       = *ptr++;
        msg->csi_req        = *ptr++;
        msg->temp_c_rnti    = srslte_bit_pack(&ptr, 16);
        ret = SRSLTE_SUCCESS;
      } 
    }

    return(ret);
}



srsue::phy my_phy;
bool bch_decoded = false; 

uint8_t payload[10240]; 
uint8_t payload_bits[10240]; 
const uint8_t conn_request_msg[] = {0x20, 0x06, 0x1F, 0x5C, 0x2C, 0x04, 0xB2, 0xAC, 0xF6, 0x00, 0x00, 0x00};

enum mac_state {RA, RAR, CONNREQUEST, CONNSETUP} state = RA; 

uint32_t preamble_idx = 0; 
rar_msg_t rar_msg;

uint32_t nof_rtx_connsetup = 0; 
uint32_t rv_value[4] = {0, 2, 3, 1}; 

void config_phy() {
  my_phy.set_param(srsue::phy_interface_params::PRACH_CONFIG_INDEX, 0);
  my_phy.set_param(srsue::phy_interface_params::PRACH_FREQ_OFFSET, 0);
  my_phy.set_param(srsue::phy_interface_params::PRACH_HIGH_SPEED_FLAG, 0);
  my_phy.set_param(srsue::phy_interface_params::PRACH_ROOT_SEQ_IDX, 0);
  my_phy.set_param(srsue::phy_interface_params::PRACH_ZC_CONFIG, 11);

  my_phy.set_param(srsue::phy_interface_params::DMRS_GROUP_HOPPING_EN, 0);
  my_phy.set_param(srsue::phy_interface_params::DMRS_SEQUENCE_HOPPING_EN, 0);
  my_phy.set_param(srsue::phy_interface_params::PUSCH_HOPPING_N_SB, 2);
  my_phy.set_param(srsue::phy_interface_params::PUSCH_RS_CYCLIC_SHIFT, 0);
  my_phy.set_param(srsue::phy_interface_params::PUSCH_RS_GROUP_ASSIGNMENT, 0);
  my_phy.set_param(srsue::phy_interface_params::PUSCH_HOPPING_OFFSET, 0);

  my_phy.set_param(srsue::phy_interface_params::PUCCH_DELTA_SHIFT, 2);
  my_phy.set_param(srsue::phy_interface_params::PUCCH_CYCLIC_SHIFT, 0);
  my_phy.set_param(srsue::phy_interface_params::PUCCH_N_PUCCH_1, 1);
  my_phy.set_param(srsue::phy_interface_params::PUCCH_N_RB_2, 2);

  my_phy.configure_ul_params();
  my_phy.configure_prach_params();
}

srslte_softbuffer_rx_t softbuffer_rx; 
srslte_softbuffer_tx_t softbuffer_tx; 

uint16_t temp_c_rnti; 

/******** MAC Interface implementation */
class testmac : public srsue::mac_interface_phy
{
public:
  
  testmac() { 
    rar_rnti_set = false; 
  }
  
  bool rar_rnti_set;
  
  void tti_clock(uint32_t tti) {
    if (!rar_rnti_set) {
      int prach_tti = my_phy.prach_tx_tti();
      if (prach_tti > 0) {
        my_phy.pdcch_dl_search(SRSLTE_RNTI_RAR, 1+prach_tti%10, prach_tti+3, prach_tti+13);
        rar_rnti_set = true; 
      }
    }
  }
  
  void new_grant_ul(mac_grant_t grant, tb_action_ul_t *action) {
    printf("New grant UL\n");
    memcpy(payload, conn_request_msg, grant.n_bytes*sizeof(uint8_t));
    action->current_tx_nb = nof_rtx_connsetup;
    action->rv = rv_value[nof_rtx_connsetup%4];
    action->softbuffer = &softbuffer_tx;     
    action->rnti = temp_c_rnti; 
    action->expect_ack = (nof_rtx_connsetup < 5)?true:false;
    action->payload_ptr = payload;
    memcpy(&action->phy_grant, &grant.phy_grant, sizeof(srslte_phy_grant_t));
    memcpy(&last_grant, &grant, sizeof(mac_grant_t));
    action->tx_enabled = true; 
    if (action->rv == 0) {
      srslte_softbuffer_tx_reset(&softbuffer_tx);      
    }
    my_phy.pdcch_dl_search(SRSLTE_RNTI_USER, temp_c_rnti);
  }
  
  void new_grant_ul_ack(mac_grant_t grant, bool ack, tb_action_ul_t *action) {
    printf("New grant UL ACK\n");    
  }

  void harq_recv(uint32_t tti, bool ack, tb_action_ul_t *action) {
    printf("harq recv hi=%d\n", ack?1:0);    
    if (!ack) {
      nof_rtx_connsetup++;
      action->current_tx_nb = nof_rtx_connsetup;
      action->rv = rv_value[nof_rtx_connsetup%4];
      action->softbuffer = &softbuffer_tx;     
      action->rnti = temp_c_rnti; 
      action->expect_ack = true; 
      memcpy(&action->phy_grant, &last_grant.phy_grant, sizeof(srslte_phy_grant_t));
      action->tx_enabled = true; 
      if (action->rv == 0) {
        srslte_softbuffer_tx_reset(&softbuffer_tx);      
      }
      printf("Retransmission %d, rv=%d\n", nof_rtx_connsetup, action->rv);
    } 
  }

  void new_grant_dl(mac_grant_t grant, tb_action_dl_t *action) {
    action->decode_enabled = true; 
    action->default_ack = false; 
    if (grant.rnti == 2) {
      action->generate_ack = false; 
    } else {
      action->generate_ack = true; 
    }
    action->payload_ptr = payload; 
    action->rnti = grant.rnti; 
    memcpy(&action->phy_grant, &grant.phy_grant, sizeof(srslte_phy_grant_t));
    memcpy(&last_grant, &grant, sizeof(mac_grant_t));
    action->rv = grant.rv;
    action->softbuffer = &softbuffer_rx;
    
    if (action->rv == 0) {
      srslte_softbuffer_rx_reset(&softbuffer_rx);
    }
  }
  
  void tb_decoded(bool ack, srslte_rnti_type_t rnti_type, uint32_t harq_pid) {
    if (ack) {
      if (rnti_type == SRSLTE_RNTI_RAR) {
        my_phy.pdcch_dl_search_reset();
        srslte_bit_unpack_vector(payload, payload_bits, last_grant.n_bytes*8);
        rar_unpack(payload_bits, &rar_msg);
        if (rar_msg.RAPID == preamble_idx) {

          printf("Received RAR at TTI: %d\n", last_grant.tti);
          my_phy.set_timeadv_rar(rar_msg.timing_adv_cmd);
          
          temp_c_rnti = rar_msg.temp_c_rnti; 
          
          if (last_grant.n_bytes*8 > 20 + SRSLTE_RAR_GRANT_LEN) {
            uint8_t rar_grant[SRSLTE_RAR_GRANT_LEN];
            memcpy(rar_grant, &payload_bits[20], sizeof(uint8_t)*SRSLTE_RAR_GRANT_LEN);
            my_phy.set_rar_grant(last_grant.tti, rar_grant);          
          }
        } else {
          printf("Received RAR RAPID=%d\n", rar_msg.RAPID);        
        }
      } else {
        printf("Received Connection Setup\n");
        my_phy.pdcch_dl_search_reset();
      }
    }
  }

  void bch_decoded_ok(uint8_t *payload, uint32_t len) {
    printf("BCH decoded\n");
    bch_decoded = true; 
    srslte_cell_t cell; 
    my_phy.get_current_cell(&cell); 
    srslte_softbuffer_rx_init(&softbuffer_rx, cell.nof_prb);
    srslte_softbuffer_tx_init(&softbuffer_tx, cell.nof_prb);
  }
   
private: 
  mac_grant_t last_grant; 
};


testmac         my_mac;
srslte::radio_uhd radio_uhd; 
  
int main(int argc, char *argv[])
{
  srslte::log_stdout log("PHY");
  
  parse_args(&prog_args, argc, argv);

  // Init Radio and PHY
  if (prog_args.uhd_rx_gain > 0 && prog_args.uhd_tx_gain > 0) {
    radio_uhd.init();
    radio_uhd.set_rx_gain(prog_args.uhd_rx_gain);
    radio_uhd.set_tx_gain(prog_args.uhd_tx_gain);
    my_phy.init(&radio_uhd, &my_mac, &log);
  } else {
    radio_uhd.init_agc();
    radio_uhd.set_tx_rx_gain_offset(10);
    my_phy.init_agc(&radio_uhd, &my_mac, &log);
  }
  
  if (srsapps_verbose == 1) {
    log.set_level(srslte::LOG_LEVEL_INFO);
    printf("Log level info\n");
  }
  if (srsapps_verbose == 2) {
    log.set_level(srslte::LOG_LEVEL_DEBUG);
    printf("Log level debug\n");
  }

  // Give it time to create thread 
  sleep(1);
  
  // Set RX freq
  radio_uhd.set_rx_freq(prog_args.uhd_rx_freq);
  radio_uhd.set_tx_freq(prog_args.uhd_tx_freq);
  
  // Instruct the PHY to configure PRACH parameters and sync to current cell 
  my_phy.sync_start();
  
  while(!my_phy.status_is_sync()) {
    usleep(20000);
  }

  // Setup PHY parameters
  config_phy();
    
  /* Instruct PHY to send PRACH and prepare it for receiving RAR */
  my_phy.prach_send(preamble_idx);
  
  /* go to idle and process each tti */
  bool running = true; 
  while(running) {
    sleep(1);
  }
  my_phy.stop();
  radio_uhd.stop_rx();
}



