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

#ifndef UE_METRICS_INTERFACE_H
#define UE_METRICS_INTERFACE_H

#include "mac/mac_metrics.h"
#include "phy/phy_metrics.h"

namespace srsue {

typedef struct {
  uint32_t uhd_o;
  uint32_t uhd_u;
  uint32_t uhd_l;
  bool     uhd_error;
}uhd_metrics_t;

typedef struct {
  uhd_metrics_t uhd;
  phy_metrics_t phy;
  mac_metrics_t mac;
}ue_metrics_t;

// UE interface
class ue_metrics_interface
{
public:
  virtual bool get_metrics(ue_metrics_t &m) = 0;
};

} // namespace srsue

#endif // UE_METRICS_INTERFACE_H
