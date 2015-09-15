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

#ifndef RLC_ENTITY_H
#define RLC_ENTITY_H

#include "common/log.h"
#include "common/common.h"
#include "liblte_rrc.h"
#include "liblte_rlc.h"

namespace srsue {

typedef enum{
    RLC_MODE_TM = 0,
    RLC_MODE_UM,
    RLC_MODE_AM,
    RLC_MODE_N_ITEMS,
}RLC_MODE_ENUM;
static const char rlc_mode_text[RLC_MODE_N_ITEMS][20] = {"Transparent Mode",
                                                         "Unacknowledged Mode",
                                                         "Acknowledged Mode"};

class rlc_entity
{
public:
  virtual void init(srslte::log *rlc_entity_log_, uint32_t lcid_) = 0;
  virtual void configure(LIBLTE_RRC_RLC_CONFIG_STRUCT *cnfg) = 0;

  virtual RLC_MODE_ENUM get_mode() = 0;
  virtual uint32_t      get_bearer() = 0;

  // PDCP interface
  virtual void write_sdu(srsue_byte_buffer_t *sdu) = 0;

  // MAC interface
  virtual uint32_t get_buffer_state() = 0;
  virtual void     read_pdu(uint8_t *payload, uint32_t nof_bytes) = 0;
  virtual void     write_pdu(uint8_t *payload, uint32_t nof_bytes) = 0;
};

} // namespace srsue


#endif // RLC_ENTITY_H
