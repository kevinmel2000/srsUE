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

#include "upper/nas.h"
#include "liblte_mme.h"

using namespace srslte;

namespace srsue{

nas::nas()
  :state(EMM_STATE_DEREGISTERED)
{}

void nas::init(usim_interface_nas *usim_, rrc_interface_nas *rrc_, srslte::log *nas_log_)
{
  pool    = buffer_pool::get_instance();
  usim    = usim_;
  rrc     = rrc_;
  nas_log = nas_log_;
}

void nas::stop()
{}

void nas::notify_connection_setup()
{
  nas_log->debug("State = %s", emm_state_text[state]);
  if(EMM_STATE_DEREGISTERED == state)
  {
    send_attach_request();
  }
}

void nas::write_pdu(uint32_t lcid, srsue_byte_buffer_t *pdu)
{}

void nas::send_attach_request()
{
  LIBLTE_MME_ATTACH_REQUEST_MSG_STRUCT  attach_req;
  srsue_byte_buffer_t                  *msg = pool->allocate();
  u_int32_t                             i;

  attach_req.eps_attach_type = 1; // EPS Attach

  for(i=0; i<8; i++)
  {
      attach_req.ue_network_cap.eea[i] = false;
      attach_req.ue_network_cap.eia[i] = false;
  }
  attach_req.ue_network_cap.eea[0] = true; // EEA0 supported
  attach_req.ue_network_cap.eia[0] = true; // EIA0 supported
  attach_req.ue_network_cap.eia[2] = true; // EIA2 supported

  attach_req.ue_network_cap.uea_present = false; // UMTS encryption algos
  attach_req.ue_network_cap.uia_present = false; // UMTS integrity algos

  attach_req.ms_network_cap_present = false; // A/Gb mode (2G) or Iu mode (3G)

  attach_req.eps_mobile_id.type_of_id = LIBLTE_MME_EPS_MOBILE_ID_TYPE_IMSI;
  usim->get_imsi_vec(attach_req.eps_mobile_id.imsi, 15);

  // ESM message (PDN connectivity request)
  gen_pdn_connectivity_request(&attach_req.esm_msg);

  attach_req.old_p_tmsi_signature_present = false;
  attach_req.additional_guti_present = false;
  attach_req.last_visited_registered_tai_present = false;
  attach_req.drx_param_present = false;
  attach_req.ms_network_cap_present = false;
  attach_req.old_lai_present = false;
  attach_req.tmsi_status_present = false;
  attach_req.ms_cm2_present = false;
  attach_req.ms_cm3_present = false;
  attach_req.supported_codecs_present = false;
  attach_req.additional_update_type_present = false;
  attach_req.voice_domain_pref_and_ue_usage_setting_present = false;
  attach_req.device_properties_present = false;
  attach_req.old_guti_type_present = false;

  // Pack the message
  liblte_mme_pack_attach_request_msg(&attach_req, (LIBLTE_BYTE_MSG_STRUCT*)msg);

  nas_log->info("Sending attach request\n");
  rrc->write_sdu(SRSUE_RB_ID_SRB1, msg);
}

void nas::gen_pdn_connectivity_request(LIBLTE_BYTE_MSG_STRUCT *msg)
{
    LIBLTE_MME_PDN_CONNECTIVITY_REQUEST_MSG_STRUCT  pdn_con_req;

    nas_log->info("Generating PDN Connectivity Request\n");

    // Set the PDN con req parameters
    pdn_con_req.eps_bearer_id       = 0x05;     // First Bearer ID
    pdn_con_req.proc_transaction_id = 0x01;     // First transaction ID
    pdn_con_req.pdn_type            = 0x01;     // Indicate IPv4 capability
    pdn_con_req.request_type        = 0x01;     // Initial Request

    // Set the optional flags
    pdn_con_req.esm_info_transfer_flag_present  = false; //FIXME: Check if this is needed
    pdn_con_req.apn_present                     = false;
    pdn_con_req.protocol_cnfg_opts_present      = false;
    pdn_con_req.device_properties_present       = false;

    // Pack the message
    liblte_mme_pack_pdn_connectivity_request_msg(&pdn_con_req, msg);
}

} // namespace srsue
