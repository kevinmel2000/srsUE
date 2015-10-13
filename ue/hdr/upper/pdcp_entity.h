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

#ifndef PDCP_ENTITY_H
#define PDCP_ENTITY_H

#include "common/buffer_pool.h"
#include "common/log.h"
#include "common/common.h"
#include "common/interfaces.h"

namespace srsue {

class pdcp_entity
{
public:
  pdcp_entity();
  void init(rlc_interface_pdcp *rlc_,
            rrc_interface_pdcp *rrc_,
            gw_interface_pdcp  *gw_,
            srslte::log        *log_,
            uint32_t            lcid_);
  bool is_active();

  // RRC interface
  void write_sdu(srsue_byte_buffer_t *sdu);

  // RLC interface
  void write_pdu(srsue_byte_buffer_t *pdu);

private:
  buffer_pool        *pool;
  srslte::log        *log;
  rlc_interface_pdcp *rlc;
  rrc_interface_pdcp *rrc;
  gw_interface_pdcp  *gw;

  bool                active;
  uint32_t            lcid;
  bool                do_security;

  // TODO: Support the following configurations
  // LIBLTE_SECURITY_CIPHERING_ALGORITHM_ID_ENUM cipher_alg;
  // LIBLTE_SECURITY_INTEGRITY_ALGORITHM_ID_ENUM integrity_alg;
  // bool do_rohc;
  // PDCP_SN_LENGTH sn_len;

  uint32              rx_sn;
  uint32              tx_sn;

};

} // namespace srsue


#endif // PDCP_ENTITY_H
