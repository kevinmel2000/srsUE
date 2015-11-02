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

#ifndef RADIO_H
#define RADIO_H

#include <stdint.h>
#include "srslte/srslte.h"
#include "srslte/common/timestamp.h"

namespace srslte {
  
/* Interface to the RF frontend. */
class SRSLTE_API radio
{
  public:
    virtual void get_time(srslte_timestamp_t *now) = 0;
    virtual bool tx(void *buffer, uint32_t nof_samples, srslte_timestamp_t tx_time) = 0;
    virtual bool tx_end() = 0;
    virtual bool rx_now(void *buffer, uint32_t nof_samples, srslte_timestamp_t *rxd_time) = 0;
    virtual bool rx_at(void *buffer, uint32_t nof_samples, srslte_timestamp_t rx_time) = 0;

    virtual void set_tx_gain(float gain) = 0;
    virtual void set_rx_gain(float gain) = 0;
    virtual double set_rx_gain_th(float gain) = 0;

    virtual void set_tx_freq(float freq) = 0;
    virtual void set_rx_freq(float freq) = 0;

    virtual void set_master_clock_rate(float rate) = 0;
    virtual void set_tx_srate(float srate) = 0;
    virtual void set_rx_srate(float srate) = 0;

    virtual void start_rx() = 0;
    virtual void stop_rx() = 0;

    virtual float get_tx_gain() = 0;
    virtual float get_rx_gain() = 0;
    
    virtual float get_max_tx_power() = 0; 
    virtual float set_tx_power(float power_dbm) = 0;
    virtual float get_rssi() = 0; 
    virtual bool  has_rssi() = 0; 
    
    // This is used for debugging/trace purposes
    virtual void set_tti(uint32_t tti) = 0;
    virtual void tx_offset(int offset) = 0;
    virtual void set_tti_len(uint32_t sf_len) = 0;
    virtual uint32_t get_tti_len() = 0;
};

} // namespace srslte

#endif // RADIO_H
