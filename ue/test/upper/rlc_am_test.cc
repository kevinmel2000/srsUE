/**
 *
 * \section COPYRIGHT
 *
 * Copyright 2013-2015 The srsUE Developers. See the
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

#include <iostream>
#include "upper/rlc_am.h"

uint8_t pdu1[] = {0x88, 0x06};
uint32_t PDU1_LEN = 2;

uint8_t pdu2[] = {0x8C, 0x00, 0xDD, 0xCD, 0xDC, 0x5D, 0xC0};
uint32_t PDU2_LEN = 7;

int main(int argc, char **argv) {
  srsue::rlc_am rlc;
  srsue::rlc_amd_pdu_header_t h;
  srsue::srsue_byte_buffer_t b1,b2,b3,b4;

  memcpy(b1.msg, &pdu1[0], PDU1_LEN);
  b1.N_bytes = PDU1_LEN;
  rlc.read_data_pdu_header(&b1, &h);
  rlc.write_data_pdu_header(&h, &b2);
  assert(b2.N_bytes == PDU1_LEN);
  for(int i=0;i<b2.N_bytes;i++)
    assert(b2.msg[i] == b1.msg[i]);

  b1.reset();
  b2.reset();
  memset(&h, 0, sizeof(srsue::rlc_amd_pdu_header_t));

  memcpy(b1.msg, &pdu2[0], PDU2_LEN);
  b1.N_bytes = PDU2_LEN;
  rlc.read_data_pdu_header(&b1, &h);
  rlc.write_data_pdu_header(&h, &b2);
  assert(b2.N_bytes == PDU2_LEN);
  for(int i=0;i<b2.N_bytes;i++)
    assert(b2.msg[i] == b1.msg[i]);
}
