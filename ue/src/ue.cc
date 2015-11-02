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

#include <boost/algorithm/string.hpp>
#include "ue.h"

namespace srsue{

ue::ue(all_args_t *args_)
    :args(args_)
    ,started(false)
{
  pool      = buffer_pool::get_instance();

  logger    = new srsue::logger(args_->log.filename);
  phy_log   = new srsue::log_filter("PHY ", logger, true);
  mac_log   = new srsue::log_filter("MAC ", logger, true);
  rlc_log   = new srsue::log_filter("RLC ", logger);
  pdcp_log  = new srsue::log_filter("PDCP", logger);
  rrc_log   = new srsue::log_filter("RRC ", logger);
  nas_log   = new srsue::log_filter("NAS ", logger);
  gw_log    = new srsue::log_filter("GW  ", logger);
  usim_log  = new srsue::log_filter("USIM", logger);

  radio_uhd = new srslte::radio_uhd;
  phy       = new srsue::phy;
  mac       = new srsue::mac;
  mac_pcap  = new srsue::mac_pcap;
  rlc       = new srsue::rlc;
  pdcp      = new srsue::pdcp;
  rrc       = new srsue::rrc;
  nas       = new srsue::nas;
  gw        = new srsue::gw;
  usim      = new srsue::usim;
}

ue::~ue()
{
  delete radio_uhd;
  delete phy;
  delete mac;
  delete mac_pcap;
  delete rlc;
  delete pdcp;
  delete rrc;
  delete nas;
  delete gw;
  delete usim;

  delete logger;
  delete phy_log;
  delete mac_log;
  delete rlc_log;
  delete pdcp_log;
  delete rrc_log;
  delete nas_log;
  delete gw_log;
  delete usim_log;

  buffer_pool::cleanup();
}

bool ue::init()
{
  // Init logs
  logger->log("\n\n");

  phy_log->set_level(level(args->log.phy_level));
  mac_log->set_level(level(args->log.mac_level));
  rlc_log->set_level(level(args->log.rlc_level));
  pdcp_log->set_level(level(args->log.pdcp_level));
  rrc_log->set_level(level(args->log.rrc_level));
  nas_log->set_level(level(args->log.nas_level));
  gw_log->set_level(level(args->log.gw_level));
  usim_log->set_level(level(args->log.usim_level));

  phy_log->set_hex_limit(args->log.phy_hex_limit);
  mac_log->set_hex_limit(args->log.mac_hex_limit);
  rlc_log->set_hex_limit(args->log.rlc_hex_limit);
  pdcp_log->set_hex_limit(args->log.pdcp_hex_limit);
  rrc_log->set_hex_limit(args->log.rrc_hex_limit);
  nas_log->set_hex_limit(args->log.nas_hex_limit);
  gw_log->set_hex_limit(args->log.gw_hex_limit);
  usim_log->set_hex_limit(args->log.usim_hex_limit);

  // Set up pcap and trace
  if(args->pcap.enable)
  {
    mac_pcap->open(args->pcap.filename.c_str());
    mac->start_pcap(mac_pcap);
  }
  if(args->trace.enable)
  {
    phy->start_trace();
    radio_uhd->start_trace();
  }
  
  // Set up expert mode parameters
  set_expert_parameters();

  // Init layers
  char *c_str = new char[args->usrp_args.size() + 1];
  strcpy(c_str, args->usrp_args.c_str());
  
  /* Start Radio/PHY with AGC if rx_gain argument is negative */
  if (args->rf.rx_gain < 0) {
    if(!radio_uhd->init_agc(c_str))
    {
      printf("Failed to find usrp with args=%s\n",c_str);
      delete [] c_str;
      return false;
    }    
    phy->init_agc(radio_uhd, mac, phy_log);
  } else {
    if(!radio_uhd->init(c_str))
    {
      printf("Failed to find usrp with args=%s\n",c_str);
      delete [] c_str;
      return false;
    }    
    phy->init(radio_uhd, mac, phy_log);    
    radio_uhd->set_rx_gain(args->rf.rx_gain);
    if (args->rf.tx_gain < 0) {
      radio_uhd->set_tx_gain(args->rf.rx_gain);
    }
  }
  if (args->rf.tx_gain > 0) {
    radio_uhd->set_tx_gain(args->rf.tx_gain);
  }

  delete [] c_str;

  radio_uhd->set_rx_freq(args->rf.dl_freq);
  radio_uhd->set_tx_freq(args->rf.ul_freq);

  mac->init(phy, rlc, mac_log);
  rlc->init(pdcp, rrc, this, rlc_log, mac);
  pdcp->init(rlc, rrc, gw, pdcp_log);
  rrc->init(phy, mac, rlc, pdcp, nas, usim, rrc_log);
  nas->init(usim, rrc, gw, nas_log);
  gw->init(pdcp, this, gw_log);
  usim->init(args->usim.imsi, args->usim.imei, args->usim.k,
             args->usim.algo, args->usim.op, args->usim.amf, usim_log);

  started = true;
}

void ue::set_expert_parameters() {
  phy->set_param(phy_interface_params::CELLSEARCH_TIMEOUT_MIB_NFRAMES, args->expert.sync_find_max_frames);
  phy->set_param(phy_interface_params::CELLSEARCH_TIMEOUT_PSS_NFRAMES, args->expert.sync_find_max_frames);
  if (args->expert.sync_find_th > 1.0) {
    phy->set_param(phy_interface_params::CELLSEARCH_TIMEOUT_PSS_CORRELATION_THRESHOLD, args->expert.sync_find_th*10);
  } else {
    phy->set_param(phy_interface_params::CELLSEARCH_TIMEOUT_PSS_CORRELATION_THRESHOLD, 160);
  }
  
  phy->set_param(phy_interface_params::SYNC_TRACK_THRESHOLD, 10*args->expert.sync_track_th);
  phy->set_param(phy_interface_params::SYNC_TRACK_AVG_COEFF, 100*args->expert.sync_track_avg_coef);
    
  phy->set_param(phy_interface_params::PRACH_GAIN, args->expert.prach_gain);
  phy->set_param(phy_interface_params::UL_GAIN, args->expert.ul_gain);
  
  phy->set_param(phy_interface_params::UL_PWR_CTRL_OFFSET, args->expert.ul_pwr_ctrl_offset);
  
  phy->set_param(phy_interface_params::RX_GAIN_OFFSET, args->expert.rx_gain_offset);
  
  phy->set_param(phy_interface_params::CONTINUOUS_TX, args->expert.continuous_tx?1:0);
  phy->set_param(phy_interface_params::PDSCH_MAX_ITS, args->expert.pdsch_max_its);
    
}

void ue::stop()
{
  if(started)
  {
    phy->stop();
    mac->stop();
    rlc->stop();
    pdcp->stop();
    rrc->stop();
    nas->stop();
    gw->stop();
    usim->stop();

    sleep(1);
    if(args->pcap.enable)
    {
       mac_pcap->close();
    }
    if(args->trace.enable)
    {
      phy->write_trace(args->trace.phy_filename);
      radio_uhd->write_trace(args->trace.radio_filename);
    }
    started = false;
  }
}

srslte::LOG_LEVEL_ENUM ue::level(std::string l)
{
  boost::to_upper(l);
  if("NONE" == l){
    return srslte::LOG_LEVEL_NONE;
  }else if("ERROR" == l){
    return srslte::LOG_LEVEL_ERROR;
  }else if("WARNING" == l){
    return srslte::LOG_LEVEL_WARNING;
  }else if("INFO" == l){
    return srslte::LOG_LEVEL_INFO;
  }else if("DEBUG" == l){
    return srslte::LOG_LEVEL_DEBUG;
  }else{
    return srslte::LOG_LEVEL_NONE;
  }
}

} // namespace srsue
