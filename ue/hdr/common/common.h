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

#ifndef COMMON_H
#define COMMON_H

/*******************************************************************************
                              INCLUDES
*******************************************************************************/

#include <stdint.h>
#include <string.h>

/*******************************************************************************
                              DEFINES
*******************************************************************************/

#define SRSUE_UE_CATEGORY     3

#define SRSUE_N_SRB           3
#define SRSUE_N_DRB           8
#define SRSUE_N_RADIO_BEARERS 11

// Cat 3 UE - Max number of DL-SCH transport block bits received within a TTI
// 3GPP 36.306 Table 4.1.1
#define SRSUE_MAX_BUFFER_SIZE 102048

/*******************************************************************************
                              TYPEDEFS
*******************************************************************************/

namespace srsue {

typedef enum{
    SRSUE_RB_ID_SRB0 = 0,
    SRSUE_RB_ID_SRB1,
    SRSUE_RB_ID_SRB2,
    SRSUE_RB_ID_DRB1,
    SRSUE_RB_ID_DRB2,
    SRSUE_RB_ID_DRB3,
    SRSUE_RB_ID_DRB4,
    SRSUE_RB_ID_DRB5,
    SRSUE_RB_ID_DRB6,
    SRSUE_RB_ID_DRB7,
    SRSUE_RB_ID_DRB8,
    SRSUE_RB_ID_N_ITEMS,
}srsue_rb_id_t;
static const char srsue_rb_id_text[SRSUE_RB_ID_N_ITEMS][20] = { "SRB0",
                                                                "SRB1",
                                                                "SRB2",
                                                                "DRB1",
                                                                "DRB2",
                                                                "DRB3",
                                                                "DRB4",
                                                                "DRB5",
                                                                "DRB6",
                                                                "DRB7",
                                                                "DRB8"};

struct srsue_byte_buffer_t{
    uint32_t N_bytes;
    uint8_t  msg[SRSUE_MAX_BUFFER_SIZE];

    srsue_byte_buffer_t():N_bytes(0){}
    srsue_byte_buffer_t(const srsue_byte_buffer_t& buf){
      N_bytes = buf.N_bytes;
      memcpy(msg, buf.msg, N_bytes);
    }
    srsue_byte_buffer_t & operator= (const srsue_byte_buffer_t & buf){
      N_bytes = buf.N_bytes;
      memcpy(msg, buf.msg, N_bytes);
    }
};

struct srsue_bit_buffer_t{
    uint32_t N_bits;
    uint8_t  msg[SRSUE_MAX_BUFFER_SIZE];

    srsue_bit_buffer_t():N_bits(0){}
    srsue_bit_buffer_t(const srsue_bit_buffer_t& buf){
      N_bits = buf.N_bits;
      memcpy(msg, buf.msg, N_bits);
    }
    srsue_bit_buffer_t & operator= (const srsue_bit_buffer_t & buf){
      N_bits = buf.N_bits;
      memcpy(msg, buf.msg, N_bits);
    }
};

} // namespace srsue

#endif // COMMON_H
