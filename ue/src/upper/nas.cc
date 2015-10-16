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
#include "liblte_security.h"

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


/*******************************************************************************
  RRC interface
*******************************************************************************/

void nas::notify_connection_setup()
{
  nas_log->debug("State = %s\n", emm_state_text[state]);
  if(EMM_STATE_DEREGISTERED == state)
  {
    send_attach_request();
  }
}

void nas::write_pdu(uint32_t lcid, byte_buffer_t *pdu)
{
  uint8 pd;
  uint8 msg_type;

  nas_log->info_hex(pdu->msg, pdu->N_bytes, "DL %s PDU", rb_id_text[lcid]);

  // Parse the message
  liblte_mme_parse_msg_header((LIBLTE_BYTE_MSG_STRUCT*)pdu, &pd, &msg_type);
  switch(msg_type)
  {
  case LIBLTE_MME_MSG_TYPE_ATTACH_ACCEPT:
      parse_attach_accept(lcid, pdu);
      break;
  case LIBLTE_MME_MSG_TYPE_ATTACH_REJECT:
      parse_attach_reject(lcid, pdu);
      break;
  case LIBLTE_MME_MSG_TYPE_AUTHENTICATION_REQUEST:
      parse_authentication_request(lcid, pdu);
      break;
  case LIBLTE_MME_MSG_TYPE_AUTHENTICATION_REJECT:
      parse_authentication_reject(lcid, pdu);
      break;
  case LIBLTE_MME_MSG_TYPE_IDENTITY_REQUEST:
      parse_identity_request(lcid, pdu);
      break;
  case LIBLTE_MME_MSG_TYPE_SECURITY_MODE_COMMAND:
      parse_security_mode_command(lcid, pdu);
      break;
  case LIBLTE_MME_MSG_TYPE_SERVICE_REJECT:
      parse_service_reject(lcid, pdu);
      break;
  case LIBLTE_MME_MSG_TYPE_ESM_INFORMATION_REQUEST:
      parse_esm_information_request(lcid, pdu);
      break;
  case LIBLTE_MME_MSG_TYPE_EMM_INFORMATION:
      parse_emm_information(lcid, pdu);
      break;
  default:
      nas_log->error("Not handling NAS message with MSG_TYPE=%02X\n",msg_type);
      break;
  }

  usim->increment_nas_count_dl();
}


/*******************************************************************************
  Parsers
*******************************************************************************/

void nas::parse_attach_accept(uint32_t lcid, byte_buffer_t *pdu){nas_log->error("TODO:parse_attach_accept\n");}
void nas::parse_attach_reject(uint32_t lcid, byte_buffer_t *pdu){nas_log->error("TODO:parse_attach_reject\n");}

void nas::parse_authentication_request(uint32_t lcid, byte_buffer_t *pdu)
{
  auth_vector_t *auth_vec;
  LIBLTE_MME_AUTHENTICATION_REQUEST_MSG_STRUCT  auth_req;
  LIBLTE_MME_AUTHENTICATION_RESPONSE_MSG_STRUCT auth_res;

  liblte_mme_unpack_authentication_request_msg((LIBLTE_BYTE_MSG_STRUCT*)pdu, &auth_req);
  nas_log->info("Received Authentication Request\n");

  // Reuse the pdu for the response message
  pdu->reset();

  // Generate authentication response using RAND, AUTN & KSI-ASME
  uint16 mcc, mnc;
  mcc = rrc->get_mcc();
  mnc = rrc->get_mnc();

  nas_log->info("MCC=%d, MNC=%d\n", mcc, mnc);

  bool net_valid;
  usim->generate_authentication_response(auth_req.rand, auth_req.autn, mcc, mnc, &net_valid);

  if(net_valid)
  {
    nas_log->info("Network authentication succesful\n");
    auth_vec = usim->get_auth_vector();
    if(NULL != auth_vec)
    {
      for(int i=0; i<8; i++)
      {
        auth_res.res[i] = auth_vec->res[i];
      }
      liblte_mme_pack_authentication_response_msg(&auth_res, (LIBLTE_BYTE_MSG_STRUCT*)pdu);

      nas_log->info("Sending Authentication Response\n");
      rrc->write_sdu(lcid, pdu);
    }
  }
  else
  {
    nas_log->warning("Network authentication failure\n");
  }
}

void nas::parse_authentication_reject(uint32_t lcid, byte_buffer_t *pdu){nas_log->error("TODO:parse_authentication_reject\n");}
void nas::parse_identity_request(uint32_t lcid, byte_buffer_t *pdu){nas_log->error("TODO:parse_identity_request\n");}

void nas::parse_security_mode_command(uint32_t lcid, byte_buffer_t *pdu)
{
  LIBLTE_MME_SECURITY_MODE_COMMAND_MSG_STRUCT  sec_mode_cmd;
  LIBLTE_MME_SECURITY_MODE_COMPLETE_MSG_STRUCT sec_mode_comp;
  LIBLTE_MME_SECURITY_MODE_REJECT_MSG_STRUCT   sec_mode_rej;

  liblte_mme_unpack_security_mode_command_msg((LIBLTE_BYTE_MSG_STRUCT*)pdu, &sec_mode_cmd);
  nas_log->info("Received Security Mode Command\n");

  // FIXME: Handle nonce_ue, nonce_mme
  // FIXME: Currently only handling ciphering EEA0 (null) and integrity EIA2
  // FIXME: Use selected_nas_sec_algs to choose correct algos

  // Reuse pdu for response
  pdu->reset();

  if(LIBLTE_MME_TYPE_OF_CIPHERING_ALGORITHM_EEA0 != sec_mode_cmd.selected_nas_sec_algs.type_of_eea ||
     LIBLTE_MME_TYPE_OF_INTEGRITY_ALGORITHM_128_EIA2 != sec_mode_cmd.selected_nas_sec_algs.type_of_eia)
  {
    // Send security mode reject
    sec_mode_rej.emm_cause = LIBLTE_MME_EMM_CAUSE_UE_SECURITY_CAPABILITIES_MISMATCH;
    liblte_mme_pack_security_mode_reject_msg(&sec_mode_rej, (LIBLTE_BYTE_MSG_STRUCT*)pdu);
    nas_log->warning("Sending Security Mode Reject due to security capabilities mismatch\n");
  }
  else
  {
    // Send security mode complete

    // Generate NAS encryption key and integrity protection key
    usim->generate_nas_keys();
    usim->generate_rrc_keys();

    if(sec_mode_cmd.imeisv_req_present && LIBLTE_MME_IMEISV_REQUESTED == sec_mode_cmd.imeisv_req)
    {
        sec_mode_comp.imeisv_present = true;
        sec_mode_comp.imeisv.type_of_id = LIBLTE_MME_MOBILE_ID_TYPE_IMEISV;
        usim->get_imei_vec(sec_mode_comp.imeisv.imeisv, 15);
        sec_mode_comp.imeisv.imeisv[14] = 5;
        sec_mode_comp.imeisv.imeisv[15] = 3;
    }
    else
    {
        sec_mode_comp.imeisv_present = false;
    }

    liblte_mme_pack_security_mode_complete_msg(&sec_mode_comp,
                                               LIBLTE_MME_SECURITY_HDR_TYPE_INTEGRITY_AND_CIPHERED,
                                               usim->get_auth_vector()->k_nas_int,
                                               usim->get_auth_vector()->nas_count_ul,
                                               LIBLTE_SECURITY_DIRECTION_UPLINK,
                                               lcid-1,
                                               (LIBLTE_BYTE_MSG_STRUCT*)pdu);
    nas_log->info("Sending Security Mode Complete nas_count_ul=%d, RB=%s\n",
                 usim->get_auth_vector()->nas_count_ul,
                 rb_id_text[lcid]);

  }

  usim->increment_nas_count_ul();
  rrc->write_sdu(lcid, pdu);
}

void nas::parse_service_reject(uint32_t lcid, byte_buffer_t *pdu){nas_log->error("TODO:parse_service_reject\n");}
void nas::parse_esm_information_request(uint32_t lcid, byte_buffer_t *pdu){nas_log->error("TODO:parse_esm_information_request\n");}
void nas::parse_emm_information(uint32_t lcid, byte_buffer_t *pdu){nas_log->error("TODO:parse_emm_information\n");}

/*******************************************************************************
  Senders
*******************************************************************************/

void nas::send_attach_request()
{
  LIBLTE_MME_ATTACH_REQUEST_MSG_STRUCT  attach_req;
  byte_buffer_t                  *msg = pool->allocate();
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
  rrc->write_sdu(RB_ID_SRB1, msg);
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

void nas::send_identity_response(){}
void nas::send_service_request(){}
void nas::send_esm_information_response(){}

} // namespace srsue
