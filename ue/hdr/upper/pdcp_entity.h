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

#ifndef PDCP_ENTITY_H
#define PDCP_ENTITY_H

#include "common/buffer_pool.h"
#include "common/log.h"
#include "common/common.h"
#include "common/interfaces.h"

namespace srsue {

/****************************************************************************
 * Structs and Defines
 * Ref: 3GPP TS 36.323 v10.1.0
 ***************************************************************************/

#define PDCP_CONTROL_MAC_I 0x00000000

#define PDCP_PDU_TYPE_PDCP_STATUS_REPORT                0x0
#define PDCP_PDU_TYPE_INTERSPERSED_ROHC_FEEDBACK_PACKET 0x1

typedef enum{
    PDCP_D_C_CONTROL_PDU = 0,
    PDCP_D_C_DATA_PDU,
    PDCP_D_C_N_ITEMS,
}pdcp_d_c_t;
static const char pdcp_d_c_text[PDCP_D_C_N_ITEMS][20] = {"Control PDU",
                                                         "Data PDU"};

/****************************************************************************
 * PDCP Entity interface
 * Common interface for all PDCP entities
 ***************************************************************************/
class pdcp_entity
{
public:
  pdcp_entity();
  void init(rlc_interface_pdcp            *rlc_,
            rrc_interface_pdcp            *rrc_,
            gw_interface_pdcp             *gw_,
            srslte::log                   *log_,
            uint32_t                       lcid_,
            LIBLTE_RRC_PDCP_CONFIG_STRUCT *cnfg = NULL);
  bool is_active();

  // RRC interface
  void write_sdu(byte_buffer_t *sdu);
  void config_security(uint8_t *k_rrc_enc_, uint8_t *k_rrc_int_);

  // RLC interface
  void write_pdu(byte_buffer_t *pdu);

private:
  buffer_pool        *pool;
  srslte::log        *log;
  rlc_interface_pdcp *rlc;
  rrc_interface_pdcp *rrc;
  gw_interface_pdcp  *gw;

  bool                active;
  uint32_t            lcid;
  bool                do_security;

  uint8_t             sn_len;
  // TODO: Support the following configurations
  // LIBLTE_SECURITY_CIPHERING_ALGORITHM_ID_ENUM cipher_alg;
  // LIBLTE_SECURITY_INTEGRITY_ALGORITHM_ID_ENUM integrity_alg;
  // bool do_rohc;

  uint32_t            rx_count;
  uint32_t            tx_count;
  uint8_t             k_rrc_enc[32];
  uint8_t             k_rrc_int[32];

};

/****************************************************************************
 * Pack/Unpack helper functions
 * Ref: 3GPP TS 36.323 v10.1.0
 ***************************************************************************/

void pdcp_pack_control_pdu(uint32_t sn, byte_buffer_t *sdu);
void pdcp_pack_control_pdu(uint32_t sn, byte_buffer_t *sdu, uint8_t *key_256, uint8_t direction, uint8_t lcid);
void pdcp_unpack_control_pdu(byte_buffer_t *sdu, uint32_t *sn);

void pdcp_pack_data_pdu_short_sn(uint32_t sn, byte_buffer_t *sdu);
void pdcp_unpack_data_pdu_short_sn(byte_buffer_t *sdu, uint32_t *sn);
void pdcp_pack_data_pdu_long_sn(uint32_t sn, byte_buffer_t *sdu);
void pdcp_unpack_data_pdu_long_sn(byte_buffer_t *sdu, uint32_t *sn);

} // namespace srsue


#endif // PDCP_ENTITY_H
