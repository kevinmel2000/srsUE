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
  rlc_entity();
  void init(srslte::log *rlc_entity_log_, RLC_MODE_ENUM mode_, uint32_t lcid_);
  void configure(LIBLTE_RRC_RLC_CONFIG_STRUCT *cnfg);
  bool is_active();

  // PDCP interface
  void write_sdu(srsue_byte_buffer_t *sdu);

  // MAC interface
  uint32_t get_buffer_state();
  void     read_pdu(uint8_t *payload, uint32_t nof_bytes);
  void     write_pdu(uint8_t *payload, uint32_t nof_bytes);

private:
  void handle_tm_sdu(LIBLTE_BYTE_MSG_STRUCT *sdu);
  void handle_tm_pdu(LIBLTE_BYTE_MSG_STRUCT *pdu);

  void handle_um_sdu(LIBLTE_BYTE_MSG_STRUCT *sdu);
  void handle_um_pdu(LIBLTE_BYTE_MSG_STRUCT *pdu);

  void handle_am_sdu(LIBLTE_BYTE_MSG_STRUCT *sdu);
  void handle_am_pdu(LIBLTE_BYTE_MSG_STRUCT *pdu);

  srslte::log   *rlc_entity_log;
  bool           active;
  RLC_MODE_ENUM  mode;
  uint32_t       lcid;
};

} // namespace srsue


#endif // RLC_ENTITY_H
