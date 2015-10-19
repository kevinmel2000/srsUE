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

#ifndef USIM_DATA_H
#define USIM_DATA_H

#include "common/common.h"

namespace srsue {

typedef struct{
  uint32_t nas_count_ul;
  uint32_t nas_count_dl;
  uint8_t  rand[16];
  uint8_t  res[8];
  uint8_t  ck[16];
  uint8_t  ik[16];
  uint8_t  autn[16];
  uint8_t  k_nas_enc[32];
  uint8_t  k_nas_int[32];
  uint8_t  k_rrc_enc[32];
  uint8_t  k_rrc_int[32];
}auth_vector_t;

typedef struct{
  uint64_t sqn_he;
  uint64_t seq_he;
  uint8_t  ak[6];
  uint8_t  mac[8];
  uint8_t  k_asme[32];
  uint8_t  k_enb[32];
  uint8_t  k_up_enc[32];
  uint8_t  k_up_int[32];
  uint8_t  ind_he;
}generated_data_t;

} // namespace srsue


#endif // USIM_DATA_H
