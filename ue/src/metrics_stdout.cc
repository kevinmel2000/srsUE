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

#include "metrics_stdout.h"

namespace srsue{

metrics_stdout::metrics_stdout()
    :started(false)
    ,metrics_report_period(1)
    ,n_reports(10)
    ,first_connect(true)
{
}

bool metrics_stdout::init(ue *u)
{
  ue_ = u;

  started = true;
  pthread_create(&metrics_thread, NULL, &metrics_thread_start, this);
  return true;
}

void metrics_stdout::stop()
{
  if(started)
  {
    started = false;
    pthread_join(metrics_thread, NULL);
  }
}

void* metrics_stdout::metrics_thread_start(void *m_)
{
  metrics_stdout *m = (metrics_stdout*)m_;
  m->metrics_thread_run();
}

void metrics_stdout::metrics_thread_run()
{
  while(started)
  {
    sleep(metrics_report_period);
    if(ue_->get_metrics(metrics)) {
      first_connect = false;
      print_metrics();
    } else {
      if(!first_connect)
        printf("--- disconnected ---\n");
    }
  }
}

void metrics_stdout::print_metrics()
{
  if(++n_reports > 10)
  {
    n_reports = 0;
    printf("-----n----sinr----rsrp---rsrq----rssi--itx---mcs--cfo-----sfo----mabr\n");
  }
  printf("%02.4f %02.4f %03.4f %02.4f %03.4f %01.2f %02.2f %04.0f %04.2f %03.2f\n",
         metrics.phy.phch_metrics.n,
         metrics.phy.phch_metrics.sinr,
         metrics.phy.phch_metrics.rsrp,
         metrics.phy.phch_metrics.rsrq,
         metrics.phy.phch_metrics.rssi,
         metrics.phy.phch_metrics.turbo_iters,
         metrics.phy.phch_metrics.dl_mcs,
         metrics.phy.sync_metrics.cfo,
         metrics.phy.sync_metrics.sfo,
         metrics.phy.mabr);
  if(metrics.uhd.uhd_error) {
    printf("UHD status: O=%d, U=%d, L=%d\n",
           metrics.uhd.uhd_o, metrics.uhd.uhd_u, metrics.uhd.uhd_l);
  }
}

} // namespace srsue
