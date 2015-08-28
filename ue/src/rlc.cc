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

#include "rlc.h"

using namespace srslte;

namespace srsue{

rlc::rlc()
  :active(false)
{}

void rlc::init(srslte::log *rlc_log_, RLC_MODE_ENUM mode_, uint32_t lcid_)
{
  rlc_log = rlc_log_;
  mode    = mode_;
  lcid    = lcid_;
  active  = true;
}

void rlc::configure(LIBLTE_RRC_RLC_CONFIG_STRUCT *cnfg)
{
  //TODO
}

bool rlc::is_active()
{
  return active;
}

}
