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

#include "upper/pdcp_entity.h"

using namespace srslte;

namespace srsue{

pdcp_entity::pdcp_entity()
  :active(false)
{}

void pdcp_entity::init(srslte::log *pdcp_entity_log_, uint32_t lcid_)
{
  pdcp_entity_log = pdcp_entity_log_;
  lcid    = lcid_;
  active  = true;
}

bool pdcp_entity::is_active()
{
  return active;
}

void pdcp_entity::write_sdu(srsue_byte_buffer_t *sdu){}
void pdcp_entity::write_pdu(srsue_byte_buffer_t *pdu){}

}
