/**
 *
 * \section COPYRIGHT
 *
 * Copyright 2013-2015 The srsUE Developers. See the
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

#ifndef MAC_H
#define MAC_H

#include "common/log.h"
#include "phy/phy.h"
#include "mac/mac_params.h"
#include "mac/dl_harq.h"
#include "mac/ul_harq.h"
#include "common/timers.h"
#include "mac/proc_ra.h"
#include "mac/proc_sr.h"
#include "mac/proc_bsr.h"
#include "mac/proc_phr.h"
#include "mac/mux.h"
#include "mac/demux.h"
#include "mac/mac_pcap.h"
#include "common/mac_interface.h"
#include "common/tti_sync_cv.h"
#include "common/threads.h"

namespace srsue {
  
class mac
    :public mac_interface_phy
    ,public mac_interface_rrc
    ,public mac_interface_timers
    ,public thread
    ,public srslte::timer_callback
{
public:
  mac();
  bool init(phy_interface *phy, rlc_interface_mac *rlc, srslte::log *log_h);
  void stop();

  /******** Interface from PHY (PHY -> MAC) ****************/ 
  /* see mac_interface.h for comments */
  void new_grant_ul(mac_grant_t grant, tb_action_ul_t *action);
  void new_grant_ul_ack(mac_grant_t grant, bool ack, tb_action_ul_t *action);
  void harq_recv(uint32_t tti, bool ack, tb_action_ul_t *action);
  void new_grant_dl(mac_grant_t grant, tb_action_dl_t *action);
  void tb_decoded(bool ack, srslte_rnti_type_t rnti_type, uint32_t harq_pid);
  void bch_decoded_ok(uint8_t *payload, uint32_t len);  
  void tti_clock(uint32_t tti);

  
  /******** Interface from RLC (RLC -> MAC) ****************/ 
  void setup_lcid(uint32_t lcid, uint32_t lcg, uint32_t priority, int PBR_x_tti, uint32_t BSD);
  void reconfiguration(); 
  void reset(); 

  /******** MAC parameters  ****************/ 
  void    set_param(mac_interface_params::mac_param_t param, int64_t value); 
  int64_t get_param(mac_interface_params::mac_param_t param);
  
  void timer_expired(uint32_t timer_id); 
  void start_pcap(mac_pcap* pcap);
  
  srslte::timers::timer*   get(uint32_t timer_id);
  u_int32_t                get_unique_id();
  
  uint32_t get_current_tti();
      
  enum {
    HARQ_RTT, 
    TIME_ALIGNMENT,
    CONTENTION_TIMER,
    BSR_TIMER_PERIODIC,
    BSR_TIMER_RETX,
    PHR_TIMER_PERIODIC,
    PHR_TIMER_PROHIBIT,
    NOF_MAC_TIMERS
  } mac_timers_t; 
  
  static const int MAC_NOF_UPPER_TIMERS = 20; 
  
private:  
  void run_thread(); 
  void search_si_rnti();
  

  static const int MAC_THREAD_PRIO = 5; 

  // Interaction with PHY 
  tti_sync_cv        ttisync; 
  phy_interface     *phy_h; 
  rlc_interface_mac *rlc_h; 
  srslte::log       *log_h;
  
  mac_params    params_db; 
  
  uint32_t      tti; 
  bool          started; 
  bool          is_synchronized; 
  uint16_t      last_temporal_crnti;
  uint16_t      phy_rnti;
  
  /* Multiplexing/Demultiplexing Units */
  mux           mux_unit; 
  demux         demux_unit; 
  
  /* DL/UL HARQ */  
  dl_harq_entity dl_harq; 
  ul_harq_entity ul_harq; 
  
  /* MAC Uplink-related Procedures */
  ra_proc       ra_procedure;
  sr_proc       sr_procedure; 
  bsr_proc      bsr_procedure; 
  phr_proc      phr_procedure; 
  
  /* Functions for MAC Timers */
  srslte::timers  timers_db;
  void            setup_timers();
  void            timeAlignmentTimerExpire();

  /* Class to run upper-layer timers with normal priority */
  class upper_timers : public thread {
  public: 
    upper_timers() : timers_db(MAC_NOF_UPPER_TIMERS),ttisync(10240) {start();}
    void tti_clock();
    void stop();
    void reset();
    srslte::timers::timer* get(uint32_t timer_id);
    uint32_t get_unique_id();
  private:
    void run_thread();
    srslte::timers  timers_db;
    tti_sync_cv     ttisync;
    bool running; 
  };
  upper_timers   upper_timers_thread; 
  
  
  // pointer to MAC PCAP object
  mac_pcap* pcap;
  bool si_search_in_progress;
  int si_window_length;
  int si_window_start;
  bool signals_pregenerated;
  bool is_first_ul_grant;
  
};

} // namespace srsue

#endif // MAC_H
