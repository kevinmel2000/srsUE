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

#include "upper/pdcp_entity.h"
#include "liblte/hdr/liblte_security.h"

using namespace srslte;

namespace srsue{

pdcp_entity::pdcp_entity()
  :active(false)
  ,tx_sn(0)
  ,rx_sn(0)
  ,do_security(false)
{
  pool = buffer_pool::get_instance();
}

void pdcp_entity::init(rlc_interface_pdcp *rlc_,
                       rrc_interface_pdcp *rrc_,
                       gw_interface_pdcp  *gw_,
                       srslte::log        *log_,
                       uint32_t            lcid_)
{
  rlc     = rlc_;
  rrc     = rrc_;
  gw      = gw_;
  log     = log_;
  lcid    = lcid_;
  active  = true;
}

bool pdcp_entity::is_active()
{
  return active;
}

// RRC interface
void pdcp_entity::write_sdu(byte_buffer_t *sdu)
{
  log->info_hex(sdu->msg, sdu->N_bytes, "UL %s SDU", rb_id_text[lcid]);

  // Handle SRB messages
  switch(lcid)
  {
  case RB_ID_SRB0:
    // Simply pass on to RLC
    rlc->write_sdu(lcid, sdu);
    break;
  case RB_ID_SRB1:  // Intentional fall-through
  case RB_ID_SRB2:
    // Pack SDU into a control PDU
    if(do_security)
    {
      /*pdcp_pack_control_pdu(&control,
                             (LIBLTE_BYTE_MSG_STRUCT*)sdu,
                             user->get_k_rrc_int(),
                             LIBLTE_SECURITY_DIRECTION_UPLINK,
                             lcid-1,
                             (LIBLTE_BYTE_MSG_STRUCT*)&pdu);*/
    }else{
      pdcp_pack_control_pdu(tx_sn, sdu);
    }
    tx_sn++;
    rlc->write_sdu(lcid, sdu);

    break;
  }

  // Handle DRB messages
  if(lcid >= RB_ID_DRB1)
  {

  }
}

// RLC interface
void pdcp_entity::write_pdu(byte_buffer_t *pdu)
{
  // Handle SRB messages
  switch(lcid)
  {
  case RB_ID_SRB0:
    // Simply pass on to RRC
    log->info_hex(pdu->msg, pdu->N_bytes, "DL %s PDU", rb_id_text[lcid]);
    rrc->write_pdu(RB_ID_SRB0, pdu);
    break;
  case RB_ID_SRB1: // Intentional fall-through
  case RB_ID_SRB2:
    uint32_t sn;
    pdcp_unpack_control_pdu(pdu, &sn);
    rrc->write_pdu(lcid, pdu);
    break;
  }

  // Handle DRB messages
  if(lcid >= RB_ID_DRB1)
  {

  }
}

/****************************************************************************
 * Pack/Unpack helper functions
 * Ref: 3GPP TS 36.323 v10.1.0
 ***************************************************************************/

void pdcp_pack_control_pdu(uint32_t sn, byte_buffer_t *sdu)
{
  // Make room and add header
  sdu->msg--;
  sdu->N_bytes++;
  *sdu->msg = sn & 0x1F;

  // Add MAC
  sdu->msg[sdu->N_bytes++] = (PDCP_CONTROL_MAC_I >> 24) & 0xFF;
  sdu->msg[sdu->N_bytes++] = (PDCP_CONTROL_MAC_I >> 16) & 0xFF;
  sdu->msg[sdu->N_bytes++] = (PDCP_CONTROL_MAC_I >>  8) & 0xFF;
  sdu->msg[sdu->N_bytes++] =  PDCP_CONTROL_MAC_I        & 0xFF;

}

void pdcp_pack_control_pdu(uint32_t sn, byte_buffer_t *sdu, uint8_t *key_256, uint8_t direction, uint8_t lcid)
{
  // Make room and add header
  sdu->msg--;
  sdu->N_bytes++;
  *sdu->msg = sn & 0x1F;

  // Add MAC
  liblte_security_128_eia2(&key_256[16],
                           sn,
                           lcid,
                           direction,
                           sdu->msg,
                           sdu->N_bytes,
                           &sdu->msg[sdu->N_bytes]);
  sdu->N_bytes += 4;
}

void pdcp_unpack_control_pdu(byte_buffer_t *pdu, uint32_t *sn)
{
  // Strip header
  *sn = *pdu->msg & 0x1F;
  pdu->msg++;
  pdu->N_bytes--;

  // Strip MAC
  pdu->N_bytes -= 4;

  // TODO: integrity check MAC
}

void pdcp_pack_data_pdu_long_sn(uint32_t sn, byte_buffer_t *sdu)
{
  // Make room and add header
  sdu->msg     -= 2;
  sdu->N_bytes += 2;
  sdu->msg[0] = (PDCP_D_C_DATA_PDU << 7) | ((sn >> 8) & 0x0F);
  sdu->msg[1] = sn & 0xFF;
}

void pdcp_unpack_data_pdu_long_sn(byte_buffer_t *sdu, uint32_t *sn)
{
  // Strip header
  *sn  = (sdu->msg[0] & 0x0F) << 8;
  *sn |= sdu->msg[1];
  sdu->msg     += 2;
  sdu->N_bytes -= 2;
}

}
