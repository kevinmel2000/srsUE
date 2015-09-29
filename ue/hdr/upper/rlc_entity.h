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
#include "common/interfaces.h"
#include "liblte/hdr/liblte_rrc.h"

namespace srsue {

/****************************************************************************
 * Structs and Defines
 * Ref: 3GPP TS 36.322 v10.0.0
 ***************************************************************************/

#define RLC_AM_WINDOW_SIZE  512
#define RLC_COUNTER_MOD     1024

typedef enum{
  RLC_MODE_TM = 0,
  RLC_MODE_UM,
  RLC_MODE_AM,
  RLC_MODE_N_ITEMS,
}rlc_mode_t;
static const char rlc_mode_text[RLC_MODE_N_ITEMS][20] = {"Transparent Mode",
                                                         "Unacknowledged Mode",
                                                         "Acknowledged Mode"};

typedef enum{
  RLC_FI_FIELD_FULL_SDU = 0,
  RLC_FI_FIELD_FIRST_SDU_SEGMENT,
  RLC_FI_FIELD_LAST_SDU_SEGMENT,
  RLC_FI_FIELD_MIDDLE_SDU_SEGMENT,
  RLC_FI_FIELD_N_ITEMS,
}rlc_fi_field_t;
static const char rlc_fi_field_text[RLC_FI_FIELD_N_ITEMS][20] = {"Full SDU",
                                                                 "First SDU Segment",
                                                                 "Last SDU Segment",
                                                                 "Middle SDU Segment"};

typedef enum{
  RLC_DC_FIELD_CONTROL_PDU = 0,
  RLC_DC_FIELD_DATA_PDU,
  RLC_DC_FIELD_N_ITEMS,
}rlc_dc_field_t;
static const char rlc_dc_field_text[RLC_DC_FIELD_N_ITEMS][20] = {"Control PDU",
                                                                 "Data PDU"};

typedef enum{
  RLC_UMD_SN_SIZE_5_BITS = 0,
  RLC_UMD_SN_SIZE_10_BITS,
  RLC_UMD_SN_SIZE_N_ITEMS,
}rlc_umd_sn_size_t;
static const char     rlc_umd_sn_size_text[RLC_UMD_SN_SIZE_N_ITEMS][20] = {"5 bits", "10 bits"};
static const uint16_t rlc_umd_sn_size_num[RLC_UMD_SN_SIZE_N_ITEMS][20]  = {5, 10};

// UMD PDU Header
typedef struct{
  rlc_fi_field_t    fi;       // Framing info
  rlc_umd_sn_size_t sn_size;  // Sequence number size (5 or 10 bits)
  uint16_t          sn;       // Sequence number
}rlc_umd_pdu_header_t;

// AMD PDU Header
typedef struct{
  rlc_dc_field_t dc;                      // Data or control
  uint8_t        rf;                      // Resegmentation flag
  uint8_t        p;                       // Polling bit
  rlc_fi_field_t fi;                      // Framing info
  uint16_t       sn;                      // Sequence number
  uint8_t        lsf;                     // Last segment flag
  uint16_t       so;                      // Segment offset
  uint32_t       N_li;                    // Number of length indicators
  uint16_t       li[RLC_AM_WINDOW_SIZE];  // Array of length indicators
}rlc_amd_pdu_header_t;

// STATUS PDU
typedef struct{
  uint32_t N_nack;
  uint16_t ack_sn;
  uint16_t nack_sn[RLC_AM_WINDOW_SIZE];
}rlc_status_pdu_t;

/****************************************************************************
 * RLC Entity interface
 * Common interface for all RLC entities
 ***************************************************************************/
class rlc_entity
{
public:
  virtual void init(srslte::log *rlc_entity_log_, uint32_t lcid_, pdcp_interface_rlc *pdcp_) = 0;
  virtual void configure(LIBLTE_RRC_RLC_CONFIG_STRUCT *cnfg) = 0;

  virtual rlc_mode_t    get_mode() = 0;
  virtual uint32_t      get_bearer() = 0;

  // PDCP interface
  virtual void write_sdu(srsue_byte_buffer_t *sdu) = 0;
  virtual bool read_sdu() = 0;

  // MAC interface
  virtual uint32_t get_buffer_state() = 0;
  virtual int      read_pdu(uint8_t *payload, uint32_t nof_bytes) = 0;
  virtual void     write_pdu(uint8_t *payload, uint32_t nof_bytes) = 0;
};

} // namespace srsue


#endif // RLC_ENTITY_H
