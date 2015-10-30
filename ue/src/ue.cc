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
#include <boost/thread/mutex.hpp>
#include "ue.h"

namespace srsue{

ue*           ue::instance = NULL;
boost::mutex  ue_instance_mutex;

ue* ue::get_instance(void)
{
    boost::mutex::scoped_lock lock(ue_instance_mutex);
    if(NULL == instance) {
        instance = new ue();
    }
    return(instance);
}
void ue::cleanup(void)
{
    boost::mutex::scoped_lock lock(ue_instance_mutex);
    if(NULL != instance) {
        delete instance;
        instance = NULL;
    }
}

ue::ue()
    :started(false)
{
  pool = buffer_pool::get_instance();
}

ue::~ue()
{
  buffer_pool::cleanup();
}

bool ue::init(all_args_t *args_)
{
  args     = args_;

  logger.init(args->log.filename);
  phy_log.init("PHY ", &logger, true);
  mac_log.init("MAC ", &logger, true);
  rlc_log.init("RLC ", &logger);
  pdcp_log.init("PDCP", &logger);
  rrc_log.init("RRC ", &logger);
  nas_log.init("NAS ", &logger);
  gw_log.init("GW  ", &logger);
  usim_log.init("USIM", &logger);

  // Init logs
  logger.log("\n\n");
  phy_log.set_level(level(args->log.phy_level));
  mac_log.set_level(level(args->log.mac_level));
  rlc_log.set_level(level(args->log.rlc_level));
  pdcp_log.set_level(level(args->log.pdcp_level));
  rrc_log.set_level(level(args->log.rrc_level));
  nas_log.set_level(level(args->log.nas_level));
  gw_log.set_level(level(args->log.gw_level));
  usim_log.set_level(level(args->log.usim_level));

  phy_log.set_hex_limit(args->log.phy_hex_limit);
  mac_log.set_hex_limit(args->log.mac_hex_limit);
  rlc_log.set_hex_limit(args->log.rlc_hex_limit);
  pdcp_log.set_hex_limit(args->log.pdcp_hex_limit);
  rrc_log.set_hex_limit(args->log.rrc_hex_limit);
  nas_log.set_hex_limit(args->log.nas_hex_limit);
  gw_log.set_hex_limit(args->log.gw_hex_limit);
  usim_log.set_hex_limit(args->log.usim_hex_limit);

  // Set up pcap and trace
  if(args->pcap.enable)
  {
    mac_pcap.open(args->pcap.filename.c_str());
    mac.start_pcap(&mac_pcap);
  }
  if(args->trace.enable)
  {
    phy.start_trace();
    radio_uhd.start_trace();
  }

  // Init layers
  radio_uhd.register_msg_handler(uhd_msg);
  char *c_str = new char[args->usrp_args.size() + 1];
  strcpy(c_str, args->usrp_args.c_str());
  if(!radio_uhd.init_agc(c_str))
  {
    printf("Failed to find usrp with args=%s\n",c_str);
    delete [] c_str;
    return false;
  }
  delete [] c_str;

  radio_uhd.set_rx_freq(args->rf.dl_freq);
  radio_uhd.set_tx_freq(args->rf.ul_freq);

  phy.init_agc(&radio_uhd, &mac, &phy_log);
  mac.init(&phy, &rlc, &mac_log);
  rlc.init(&pdcp, &rrc, this, &rlc_log, &mac);
  pdcp.init(&rlc, &rrc, &gw, &pdcp_log);
  rrc.init(&phy, &mac, &rlc, &pdcp, &nas, &usim, &rrc_log);
  nas.init(&usim, &rrc, &gw, &nas_log);
  gw.init(&pdcp, this, &gw_log);
  usim.init(args->usim.imsi, args->usim.imei, args->usim.k, &usim_log);

  started = true;
  return true;
}

void ue::stop()
{
  if(started)
  {
    phy.stop();
    mac.stop();
    rlc.stop();
    pdcp.stop();
    rrc.stop();
    nas.stop();
    gw.stop();
    usim.stop();

    sleep(1);
    if(args->pcap.enable)
    {
       mac_pcap.close();
    }
    if(args->trace.enable)
    {
      phy.write_trace(args->trace.phy_filename);
      radio_uhd.write_trace(args->trace.radio_filename);
    }
    started = false;
  }
}

bool ue::get_metrics(ue_metrics_t &m)
{
  m.uhd = uhd_metrics;
  uhd_metrics.uhd_error = false; // Reset error flag

  if(EMM_STATE_REGISTERED == nas.get_state()) {
    if(RRC_STATE_RRC_CONNECTED == rrc.get_state()) {
      phy.get_metrics(m.phy);
      return true;
    }
  }
  return false;
}

void ue::uhd_msg(const char *msg)
{
  ue *u = ue::get_instance();
  u->handle_uhd_msg(msg);
}

void ue::handle_uhd_msg(const char* msg)
{
  if(0 == strcmp(msg, "O")) {
    uhd_metrics.uhd_o++;
    uhd_metrics.uhd_error = true;
  } else if(0 == strcmp(msg, "D")) {
    uhd_metrics.uhd_o++;
    uhd_metrics.uhd_error = true;
  }else if(0 == strcmp(msg, "U")) {
    uhd_metrics.uhd_u++;
    uhd_metrics.uhd_error = true;
  } else if(0 == strcmp(msg, "L")) {
    uhd_metrics.uhd_l++;
    uhd_metrics.uhd_error = true;
  } else {
    printf("%s", msg);
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
