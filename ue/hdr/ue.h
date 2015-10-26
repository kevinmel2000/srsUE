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

/******************************************************************************
 * File:        ue.h
 * Description: Top-level UE class. Creates and links all
 *              layers and helpers.
 *****************************************************************************/

#ifndef UE_H
#define UE_H

#include <stdarg.h>
#include <string>

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
#include "common/threads.h"
#include "common/logger.h"
#include "common/log_filter.h"

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

typedef struct{
  std::string imsi;
  std::string imei;
  std::string k;
}usim_args_t;

typedef struct {
  std::string   usrp_args;
  rf_args_t     rf;
  pcap_args_t   pcap;
  trace_args_t  trace;
  log_args_t    log;
  usim_args_t   usim;
}all_args_t;

/*******************************************************************************
  Main UE class
*******************************************************************************/

class ue
    :public thread
    ,public ue_interface
{
public:
  ue(all_args_t *args_);
  ~ue();
  bool init();
  void stop();
  void notify();

protected:
  void run_thread();

private:
  srslte::radio_uhd *radio_uhd;
  srsue::phy        *phy;
  srsue::mac        *mac;
  srsue::mac_pcap   *mac_pcap;
  srsue::rlc        *rlc;
  srsue::pdcp       *pdcp;
  srsue::rrc        *rrc;
  srsue::nas        *nas;
  srsue::gw         *gw;
  srsue::usim       *usim;

  srsue::logger     *logger;
  srsue::log_filter *phy_log;
  srsue::log_filter *mac_log;
  srsue::log_filter *rlc_log;
  srsue::log_filter *pdcp_log;
  srsue::log_filter *rrc_log;
  srsue::log_filter *nas_log;
  srsue::log_filter *gw_log;
  srsue::log_filter *usim_log;

  srsue::buffer_pool *pool;

  all_args_t       *args;
  bool              started;

  bool              have_data;
  boost::condition  condition;
  boost::mutex      mutex;

  srslte::LOG_LEVEL_ENUM level(std::string l);
};

} // namespace srsue

#endif // UE_H
