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

#include "upper/usim.h"

using namespace srslte;

namespace srsue{

usim::usim()
{}

void usim::init(std::string imsi_, std::string imei_, std::string k_, srslte::log *usim_log_)
{
  usim_log = usim_log_;

  const char *imsi_str = imsi_.c_str();
  const char *imei_str = imei_.c_str();
  const char *k_str    = k_.c_str();
  uint32_t    i;

  if(15   == imsi_.length() &&
     15   == imei_.length() &&
     32   == k_.length())
  {
      imsi = 0;
      for(i=0; i<15; i++)
      {
          imsi *= 10;
          imsi += imsi_str[i] - '0';
      }

      imei = 0;
      for(i=0; i<15; i++)
      {
          imei *= 10;
          imei += imei_str[i] - '0';
      }

      for(i=0; i<16; i++)
      {
          if(k_str[i*2+0] >= '0' && k_str[i*2+0] <= '9')
          {
              k[i] = (k_str[i*2+0] - '0') << 4;
          }else if(k_str[i*2+0] >= 'A' && k_str[i*2+0] <= 'F'){
              k[i] = ((k_str[i*2+0] - 'A') + 0xA) << 4;
          }else{
              k[i] = ((k_str[i*2+0] - 'a') + 0xA) << 4;
          }

          if(k_str[i*2+1] >= '0' && k_str[i*2+1] <= '9')
          {
              k[i] |= k_str[i*2+1] - '0';
          }else if(k_str[i*2+1] >= 'A' && k_str[i*2+1] <= 'F'){
              k[i] |= (k_str[i*2+1] - 'A') + 0xA;
          }else{
              k[i] |= (k_str[i*2+1] - 'a') + 0xA;
          }
      }
  }
}

void usim::stop()
{}

// NAS interface
void usim::get_imsi_vec(uint8_t* imsi_, uint32_t n)
{
  if(NULL == imsi_ || n < 15)
  {
    usim_log->error("Invalid parameters to get_imsi_vec");
    return;
  }

  uint64_t temp = imsi;
  for(int i=14;i>=0;i--)
  {
      imsi_[i] = temp % 10;
      temp /= 10;
  }
}

} // namespace srsue
