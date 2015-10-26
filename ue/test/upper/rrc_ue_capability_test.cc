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
#include <iomanip>
#include <sstream>
#include <string>
#include "common/common.h"
#include "liblte_rrc.h"

// Fixed header only
uint8_t pdu1[] = {0x18 ,0xE2};
uint32_t PDU1_LEN = 2;

// Fixed + 1 LI field (value 104)
uint8_t pdu2[] = {0x1C ,0xE1 ,0x06 ,0x80};
uint32_t PDU2_LEN = 4;

using namespace srsue;

uint32_t srslte_bit_pack(uint8_t **bits, int nof_bits)
{
    int i;
    uint32_t value=0;

    for(i=0; i<nof_bits; i++) {
      value |= (uint32_t) (*bits)[i] << (nof_bits-i-1);
    }
    *bits += nof_bits;
    return value;
}

void srslte_bit_pack_vector(uint8_t *unpacked, uint8_t *packed, int nof_bits)
{
  uint32_t i, nbytes;
  nbytes = nof_bits/8;
  for (i=0;i<nbytes;i++) {
    packed[i] = srslte_bit_pack(&unpacked, 8);
  }
  if (nof_bits%8) {
    packed[i] = srslte_bit_pack(&unpacked, nof_bits%8);
    packed[i] <<= 8-(nof_bits%8);
  }
}

std::string hex_string(uint8_t *hex, int size)
{
  std::stringstream ss;
  int c = 0;

  ss << std::hex << std::setfill('0');
  while(c < size) {
    ss << "             " << std::setw(4) << static_cast<unsigned>(c) << ": ";
    int tmp = (size-c < 16) ? size-c : 16;
    for(int i=0;i<tmp;i++) {
      ss << std::setw(2) << static_cast<unsigned>(hex[c++]) << " ";
    }
    ss << "\n";
  }
  return ss.str();
}

int main(int argc, char **argv) {
  LIBLTE_RRC_UL_DCCH_MSG_STRUCT ul_dcch_msg;

  ul_dcch_msg.msg_type = LIBLTE_RRC_UL_DCCH_MSG_TYPE_UE_CAPABILITY_INFO;
  ul_dcch_msg.msg.ue_capability_info.rrc_transaction_id = 0;

  LIBLTE_RRC_UE_CAPABILITY_INFORMATION_STRUCT *info = &ul_dcch_msg.msg.ue_capability_info;
  info->N_ue_caps = 1;
  info->ue_capability_rat[0].rat_type = LIBLTE_RRC_RAT_TYPE_EUTRA;

  LIBLTE_RRC_UE_EUTRA_CAPABILITY_STRUCT *cap = &info->ue_capability_rat[0].eutra_capability;
  cap->access_stratum_release = LIBLTE_RRC_ACCESS_STRATUM_RELEASE_REL9;
  cap->ue_category = 3;

  cap->pdcp_params.max_rohc_ctxts_present = false;
  cap->pdcp_params.supported_rohc_profiles[0] = false;
  cap->pdcp_params.supported_rohc_profiles[1] = false;
  cap->pdcp_params.supported_rohc_profiles[2] = false;
  cap->pdcp_params.supported_rohc_profiles[3] = false;
  cap->pdcp_params.supported_rohc_profiles[4] = false;
  cap->pdcp_params.supported_rohc_profiles[5] = false;
  cap->pdcp_params.supported_rohc_profiles[6] = false;
  cap->pdcp_params.supported_rohc_profiles[7] = false;
  cap->pdcp_params.supported_rohc_profiles[8] = false;

  cap->phy_params.specific_ref_sigs_supported = false;
  cap->phy_params.tx_antenna_selection_supported = false;

  //TODO: Generate this from user input?
  cap->rf_params.N_supported_band_eutras = 3;
  cap->rf_params.supported_band_eutra[0].band_eutra = 3;
  cap->rf_params.supported_band_eutra[0].half_duplex = false;
  cap->rf_params.supported_band_eutra[1].band_eutra = 7;
  cap->rf_params.supported_band_eutra[1].half_duplex = false;
  cap->rf_params.supported_band_eutra[2].band_eutra = 20;
  cap->rf_params.supported_band_eutra[2].half_duplex = false;

  cap->meas_params.N_band_list_eutra = 3;
  cap->meas_params.band_list_eutra[0].N_inter_freq_need_for_gaps = 3;
  cap->meas_params.band_list_eutra[0].inter_freq_need_for_gaps[0] = true;
  cap->meas_params.band_list_eutra[0].inter_freq_need_for_gaps[1] = true;
  cap->meas_params.band_list_eutra[0].inter_freq_need_for_gaps[2] = true;
  cap->meas_params.band_list_eutra[1].N_inter_freq_need_for_gaps = 3;
  cap->meas_params.band_list_eutra[1].inter_freq_need_for_gaps[0] = true;
  cap->meas_params.band_list_eutra[1].inter_freq_need_for_gaps[1] = true;
  cap->meas_params.band_list_eutra[1].inter_freq_need_for_gaps[2] = true;
  cap->meas_params.band_list_eutra[2].N_inter_freq_need_for_gaps = 3;
  cap->meas_params.band_list_eutra[2].inter_freq_need_for_gaps[0] = true;
  cap->meas_params.band_list_eutra[2].inter_freq_need_for_gaps[1] = true;
  cap->meas_params.band_list_eutra[2].inter_freq_need_for_gaps[2] = true;

  cap->feature_group_indicator_present         = true;
  cap->feature_group_indicator                 = 0x7f0dfcba;
  cap->inter_rat_params.utra_fdd_present       = false;
  cap->inter_rat_params.utra_tdd128_present    = false;
  cap->inter_rat_params.utra_tdd384_present    = false;
  cap->inter_rat_params.utra_tdd768_present    = false;
  cap->inter_rat_params.geran_present          = false;
  cap->inter_rat_params.cdma2000_hrpd_present  = false;
  cap->inter_rat_params.cdma2000_1xrtt_present = false;

  bit_buffer_t bit_buf;
  liblte_rrc_pack_ul_dcch_msg(&ul_dcch_msg, (LIBLTE_BIT_MSG_STRUCT*)&bit_buf);

  // Byte align and pack the message bits for PDCP
  byte_buffer_t pdu;
  if((bit_buf.N_bits % 8) != 0)
  {
    for(int i=0; i<8-(bit_buf.N_bits % 8); i++)
      bit_buf.msg[bit_buf.N_bits + i] = 0;
    bit_buf.N_bits += 8 - (bit_buf.N_bits % 8);
  }
  srslte_bit_pack_vector(bit_buf.msg, pdu.msg, bit_buf.N_bits);
  pdu.N_bytes = bit_buf.N_bits/8;

  std::cout << hex_string(pdu.msg, pdu.N_bytes);

  LIBLTE_RRC_UL_DCCH_MSG_STRUCT rx_ul_dcch_msg;
  liblte_rrc_unpack_ul_dcch_msg((LIBLTE_BIT_MSG_STRUCT*)&bit_buf, &rx_ul_dcch_msg);
}
