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

#include <sstream>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <iomanip>

using namespace std;

namespace srsue{

char const * const prefixes[2][9] =
{
  {   "",   "m",   "u",   "n",    "p",    "f",    "a",    "z",    "y", },
  {   "",   "k",   "M",   "G",    "T",    "P",    "E",    "Z",    "Y", },
};

metrics_stdout::metrics_stdout(int report_period_secs)
    :started(false)
    ,do_print(false)
    ,metrics_report_period(report_period_secs)
    ,n_reports(10)
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

void metrics_stdout::toggle_print(bool b)
{
  do_print = b;
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
      print_metrics();
    } else {
      print_disconnect();
    }
  }
}

void metrics_stdout::print_metrics()
{
  if(!do_print)
    return;

  if(++n_reports > 10)
  {
    n_reports = 0;
    cout << endl;
    cout << "--DL-------------------------------------------------- UL------" << endl;
    cout << "mcs\tsnr\trsrq\tturbo\tcfo\tsfo\tmabr\tmcs" << endl;
  }
  cout << float_to_string(metrics.phy.dl.mcs, 2) << "\t";
  cout << float_to_string(metrics.phy.dl.sinr, 3) << "\t";
  cout << float_to_string(metrics.phy.dl.rsrq, 3) << "\t";
  cout << float_to_string(metrics.phy.dl.turbo_iters, 2) << "\t";
  cout << float_to_eng_string(metrics.phy.sync.cfo, 3) << "\t";
  cout << float_to_eng_string(metrics.phy.sync.sfo, 3) << "\t";
  cout << float_to_eng_string(metrics.phy.mabr*1000, 3) << "\t";
  cout << float_to_string(metrics.phy.ul.mcs, 2) << " ";
  cout << endl;


//  printf("%f %f %f %f %f %f %f %f %f %f\n",
//         metrics.phy.dl.n,
//         metrics.phy.dl.sinr,
//         metrics.phy.dl.rsrp,
//         metrics.phy.dl.rsrq,
//         metrics.phy.dl.rssi,
//         metrics.phy.dl.turbo_iters,
//         metrics.phy.dl.mcs,
//         metrics.phy.sync.cfo,
//         metrics.phy.sync.sfo,
//         metrics.phy.mabr);
  if(metrics.uhd.uhd_error) {
    cout << "UHD status:"
         << "  O=" << metrics.uhd.uhd_o
         << ", U=" << metrics.uhd.uhd_u
         << ", L=" << metrics.uhd.uhd_l << endl;
  }
}

void metrics_stdout::print_disconnect()
{
  if(do_print) {
    cout << "--- disconnected ---" << endl;
  }
}

std::string metrics_stdout::float_to_string(float f, int digits)
{
  std::ostringstream os;
  const int    precision = (f == 0.0) ? digits-1 : digits - log10(fabs(f))-2*DBL_EPSILON;
  os << std::fixed << std::setprecision(precision) << f;
  return os.str();
}

std::string metrics_stdout::float_to_eng_string(float f, int digits)
{
  const int degree = (f == 0.0) ? 0 : lrint( floor( log10( fabs( f ) ) / 3) );

  std::string factor;

  if ( abs( degree ) < 9 )
  {
    if(degree < 0)
      factor = prefixes[0][ abs( degree ) ];
    else
      factor = prefixes[1][ abs( degree ) ];
  } else {
    return "failed";
  }

  const double scaled = f * pow( 1000.0, -degree );
  return float_to_string(scaled, digits) + factor;
}

} // namespace srsue
