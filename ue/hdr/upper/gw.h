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

#ifndef GW_H
#define GW_H

#include "common/buffer_pool.h"
#include "common/log.h"
#include "common/common.h"
#include "common/msg_queue.h"
#include "common/interfaces.h"
#include "common/threads.h"

#include <linux/if.h>

namespace srsue {

struct gw_metrics_t
{
  float dl_tput;
  float ul_tput;
};

class gw
    :public gw_interface_pdcp
    ,public gw_interface_nas
    ,public thread
{
public:
  gw();
  void init(pdcp_interface_gw *pdcp_, ue_interface *ue_, srslte::log *gw_log_);
  void stop();

  void get_metrics(gw_metrics_t *m){}

  // UE interface
  bool check_ul_buffers();

  // PDCP interface
  void write_pdu(uint32_t lcid, byte_buffer_t *pdu);

  // NAS interface
  error_t setup_if_addr(uint32_t ip_addr, char *err_str);
  
private:
  
  static const int GW_THREAD_PRIO = 7; 
  
  buffer_pool        *pool;
  srslte::log        *gw_log;
  pdcp_interface_gw  *pdcp;
  ue_interface       *ue;
  bool                running;
  int32               tun_fd;
  struct ifreq        ifr;
  int32               sock;
  bool                if_up;
  msg_queue           rx_sdu_queue;

  void                run_thread();
  error_t             init_if(char *err_str);
};

} // namespace srsue


#endif // GW_H
