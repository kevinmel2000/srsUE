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

#include "upper/gw.h"

using namespace srslte;

namespace srsue{

gw::gw()
{}

void gw::init(pdcp_interface_gw *pdcp_, ue_interface *ue_, srslte::log *gw_log_)
{
  pdcp    = pdcp_;
  ue      = ue_;
  gw_log  = gw_log_;
}

void gw::stop()
{}

/*******************************************************************************
  UE interface
*******************************************************************************/
bool gw::check_ul_buffers()
{
  return false;
}

} // namespace srsue
