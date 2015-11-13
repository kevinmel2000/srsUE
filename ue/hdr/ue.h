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

/******************************************************************************
 * File:        ue.h
 * Description: Top-level UE class. Creates and links all
 *              layers and helpers.
 *****************************************************************************/

#ifndef UE_H
#define UE_H

#include <stdarg.h>
#include <string>
#include <pthread.h>

#include "radio/radio_uhd.h"
#include "phy/phy.h"
#include "mac/mac.h"
#include "upper/rlc.h"
#include "upper/pdcp.h"
#include "upper/rrc.h"
#include "upper/nas.h"
#include "upper/gw.h"
#include "upper/usim.h"

#include "common/buffer_pool.h"
#include "common/interfaces.h"
#include "common/logger.h"
#include "common/log_filter.h"


#define REQUIRED_SRSLTE_VERSION_MAJOR  1
#define REQUIRED_SRSLTE_VERSION_MINOR  0
#define REQUIRED_SRSLTE_VERSION_BUGFIX 0

namespace srsue {

/*******************************************************************************
  UE Parameters
*******************************************************************************/

typedef struct {
  float         dl_freq;
  float         ul_freq;
  float         rx_gain;
  float         tx_gain;
}rf_args_t;

typedef struct {
  bool          enable;
  std::string   filename;
}pcap_args_t;

typedef struct {
  bool          enable;
  std::string   phy_filename;
  std::string   radio_filename;
}trace_args_t;

typedef struct {
  std::string   phy_level;
  std::string   mac_level;
  std::string   rlc_level;
  std::string   pdcp_level;
  std::string   rrc_level;
  std::string   gw_level;
  std::string   nas_level;
  std::string   usim_level;
  std::string   all_level;
  int           phy_hex_limit;
  int           mac_hex_limit;
  int           rlc_hex_limit;
  int           pdcp_hex_limit;
  int           rrc_hex_limit;
  int           gw_hex_limit;
  int           nas_hex_limit;
  int           usim_hex_limit;
  int           all_hex_limit;
  std::string   filename;
}log_args_t;

typedef struct {
  float prach_gain;
  float ul_gain;
  float ul_pwr_ctrl_offset;
  float rx_gain_offset;
  int pdsch_max_its;
  float sync_track_th;
  float sync_track_avg_coef;
  float sync_find_th;
  float sync_find_max_frames;
  bool enable_64qam_attach; 
  bool continuous_tx;
  int nof_phy_threads;  
}expert_args_t;

typedef struct {
  std::string   usrp_args;
  rf_args_t     rf;
  pcap_args_t   pcap;
  trace_args_t  trace;
  log_args_t    log;
  usim_args_t   usim;
  expert_args_t expert;
}all_args_t;

typedef struct {
  uint32_t uhd_o;
  uint32_t uhd_u;
  uint32_t uhd_l;
  bool     uhd_error;
}uhd_metrics_t;

typedef struct {
  uhd_metrics_t uhd;
  phy_metrics_t phy;
}ue_metrics_t;

/*******************************************************************************
  Main UE class
*******************************************************************************/

class ue
    :public ue_interface
{
public:
  static ue* get_instance(void);
  static void cleanup(void);

  bool init(all_args_t *args_);
  void stop();
  bool get_metrics(ue_metrics_t &m);
  static void uhd_msg(const char* msg);
  void handle_uhd_msg(const char* msg);

private:
  static ue *instance;
  ue();
  ~ue();

  srslte::radio_uhd radio_uhd;
  srsue::phy        phy;
  srsue::mac        mac;
  srsue::mac_pcap   mac_pcap;
  srsue::rlc        rlc;
  srsue::pdcp       pdcp;
  srsue::rrc        rrc;
  srsue::nas        nas;
  srsue::gw         gw;
  srsue::usim       usim;

  srsue::logger     logger;
  srsue::log_filter uhd_log;
  srsue::log_filter phy_log;
  srsue::log_filter mac_log;
  srsue::log_filter rlc_log;
  srsue::log_filter pdcp_log;
  srsue::log_filter rrc_log;
  srsue::log_filter nas_log;
  srsue::log_filter gw_log;
  srsue::log_filter usim_log;

  srsue::buffer_pool *pool;

  all_args_t       *args;
  bool              started;
  uhd_metrics_t     uhd_metrics;

  srslte::LOG_LEVEL_ENUM level(std::string l);
  
  bool check_srslte_version();
  void set_expert_parameters();
};

} // namespace srsue

#endif // UE_H
