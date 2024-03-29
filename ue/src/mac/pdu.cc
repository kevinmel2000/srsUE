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

#include <strings.h>
#include <string.h>
#include <stdlib.h>

#include "mac/pdu.h"
#include "srslte/srslte.h"


namespace srsue {
   
    
void sch_pdu::fprint(FILE* stream)
{
  fprintf(stream, "MAC SDU for UL/DL-SCH. ");
  pdu::fprint(stream);
}

void sch_subh::fprint(FILE* stream)
{
  if (is_sdu()) {
    fprintf(stream, "SDU LCHID=%d, SDU nof_bytes=%d\n", lcid, nof_bytes);
  } else {
    if (parent->is_ul()) {
      switch(lcid) {
        case C_RNTI:
          fprintf(stream, "C-RNTI CE\n");
          break;
        case PHR_REPORT:
          fprintf(stream, "PHR\n");
          break;
        case TRUNC_BSR:
          fprintf(stream, "Truncated BSR CE\n");
          break;
        case SHORT_BSR:
          fprintf(stream, "Short BSR CE\n");
          break;
        case LONG_BSR:
          fprintf(stream, "Long BSR CE\n");
          break;
        case PADDING:
          fprintf(stream, "PADDING\n");
      }
    } else {
      switch(lcid) {
        case CON_RES_ID:
          fprintf(stream, "Contention Resolution ID CE: 0x%lx\n", get_con_res_id());
          break;
        case TA_CMD:
          fprintf(stream, "Time Advance Command CE: %d\n", get_ta_cmd());
          break;
        case DRX_CMD:
          fprintf(stream, "DRX Command CE: Not implemented\n");
          break;
        case PADDING:
          fprintf(stream, "PADDING\n");
      }      
    }
  }
}

void sch_pdu::parse_packet(uint8_t *ptr)
{

  pdu::parse_packet(ptr);

  // Correct size for last SDU 
  if (nof_subheaders > 0) {
    uint32_t read_len = 0; 
    for (int i=0;i<nof_subheaders-1;i++) {
      read_len += subheaders[i].size_plus_header();
    }

    if (pdu_len-read_len-1 >= 0) {
      subheaders[nof_subheaders-1].set_payload_size(pdu_len-read_len-1);
    } else {
      fprintf(stderr,"Reading MAC PDU: negative payload for last subheader\n");
    }
  }
}

uint8_t* sch_pdu::write_packet() {
  write_packet(NULL);
}

/* Writes the MAC PDU in the packet, including the MAC headers and CE payload. Section 6.1.2 */
uint8_t* sch_pdu::write_packet(srslte::log *log_h)
{
  int init_rem_len=rem_len; 
  sch_subh padding; 
  padding.set_padding(); 
    
  /* If last SDU has zero payload, remove it. FIXME: Why happens this?? */
  if (subheaders[nof_subheaders-1].get_payload_size() == 0) {
    del_subh();
  }
  
  /* Determine if we are transmitting CEs only. */
  bool ce_only = last_sdu_idx<0?true:false; 
  
  /* Determine if we will need multi-byte padding or 1/2 bytes padding */
  bool multibyte_padding  = false;
  uint32_t onetwo_padding = 0; 
  if (rem_len > 2) {
    multibyte_padding = true; 
    // Add 1 header for padding 
    rem_len--; 
    // Add the header for the last SDU
    if (!ce_only) {
      rem_len -= (subheaders[last_sdu_idx].get_header_size(false)-1); // Becuase we were assuming it was the one
    }
  } else if (rem_len > 0) {
    onetwo_padding = rem_len; 
    rem_len = 0;  
  }
  
  /* Determine the header size and CE payload size */
  uint32_t header_sz     = 0; 
  uint32_t ce_payload_sz = 0; 
  for (int i=0;i<nof_subheaders;i++) {
    header_sz += subheaders[i].get_header_size(!multibyte_padding && i==last_sdu_idx);
    if (!subheaders[i].is_sdu()) {
      ce_payload_sz += subheaders[i].get_payload_size();
    }
  }
  if (multibyte_padding) {
    header_sz += 1; 
  } else if (onetwo_padding) {
    header_sz += onetwo_padding;
  }
  if (ce_payload_sz + header_sz >= sdu_offset_start) {
    fprintf(stderr, "Writting PDU: header sz + ce_payload_sz >= sdu_offset_start (%d>=%d). pdu_len=%d, total_sdu_len=%d\n", 
            header_sz + ce_payload_sz, sdu_offset_start, pdu_len, total_sdu_len);
    return NULL; 
  }

  /* Start writting header and CE payload before the start of the SDU payload */
  uint8_t *ptr = &buffer_tx[sdu_offset_start-header_sz-ce_payload_sz];
  uint8_t *pdu_start_ptr = ptr; 
  
  // Add single/two byte padding first 
  for (int i=0;i<onetwo_padding;i++) {
    padding.write_subheader(&ptr, false);  
  }
  // Then write subheaders for MAC CE
  for (int i=0;i<nof_subheaders;i++) {
    if (!subheaders[i].is_sdu()) {
      subheaders[i].write_subheader(&ptr, ce_only && !multibyte_padding && i==(nof_subheaders-1));
    }
  }
  // Then for SDUs
  if (!ce_only) {
    for (int i=0;i<nof_subheaders;i++) {
      if (subheaders[i].is_sdu()) {
        subheaders[i].write_subheader(&ptr, !multibyte_padding && i==last_sdu_idx);
      }
    }
  }
  // and finally add multi-byte padding
  if (multibyte_padding) {
    sch_subh padding_multi; 
    padding_multi.set_padding(rem_len); 
    padding_multi.write_subheader(&ptr, true);
  }
  
  // Write CE payloads (SDU payloads already in the buffer)
  for (int i=0;i<nof_subheaders;i++) {
    if (!subheaders[i].is_sdu()) {      
      subheaders[i].write_payload(&ptr);
    }
  }

  // Set padding to zeros (if any) 
  if (rem_len > 0) {
    bzero(&pdu_start_ptr[pdu_len-rem_len], rem_len*sizeof(uint8_t));
  }

  if (log_h) {
    log_h->info("Wrote PDU: pdu_len=%d, header_and_ce=%d (%d+%d), nof_subh=%d, last_sdu=%d, sdu_len=%d, onepad=%d, multi=%d\n", 
         pdu_len, header_sz+ce_payload_sz, header_sz, ce_payload_sz, 
         nof_subheaders, last_sdu_idx, total_sdu_len, onetwo_padding, rem_len);
  } else {
    printf("Wrote PDU: pdu_len=%d, header_and_ce=%d (%d+%d), nof_subh=%d, last_sdu=%d, sdu_len=%d, onepad=%d, multi=%d, init_rem_len=%d\n", 
         pdu_len, header_sz+ce_payload_sz, header_sz, ce_payload_sz, 
         nof_subheaders, last_sdu_idx, total_sdu_len, onetwo_padding, rem_len, init_rem_len);
  }
  
  if (rem_len + header_sz + ce_payload_sz + total_sdu_len != pdu_len) {
    printf("\n------------------------------\n");
    for (int i=0;i<nof_subheaders;i++) {
      printf("SUBH %d is_sdu=%d, payload=%d\n", i, subheaders[i].is_sdu(), subheaders[i].get_payload_size());
    }
    printf("Wrote PDU: pdu_len=%d, header_and_ce=%d (%d+%d), nof_subh=%d, last_sdu=%d, sdu_len=%d, onepad=%d, multi=%d, init_rem_len=%d\n", 
         pdu_len, header_sz+ce_payload_sz, header_sz, ce_payload_sz, 
         nof_subheaders, last_sdu_idx, total_sdu_len, onetwo_padding, rem_len, init_rem_len);
    fprintf(stderr, "Expected PDU len %d bytes but wrote %d\n", pdu_len, rem_len + header_sz + ce_payload_sz + total_sdu_len);
    printf("------------------------------\n");
    return NULL; 
  }

  if (header_sz + ce_payload_sz != (int) (ptr - pdu_start_ptr)) {
    fprintf(stderr, "Expected a header and CE payload of %d bytes but wrote %d\n", 
            header_sz+ce_payload_sz,(int) (ptr - pdu_start_ptr));
    return NULL;
  }
  
  return pdu_start_ptr; 
}

int sch_pdu::rem_size() {
  return rem_len; 
}

int sch_pdu::get_pdu_len()
{
  return pdu_len; 
}

uint32_t sch_pdu::size_header_sdu(uint32_t nbytes)
{
  if (nbytes < 128) {
    return 2; 
  } else {
    return 3; 
  }
}
bool sch_pdu::has_space_ce(uint32_t nbytes)
{
  if (rem_len >= nbytes + 1) {
    return true; 
  } else {
    return false; 
  }
}

bool sch_pdu::update_space_ce(uint32_t nbytes)
{
  if (has_space_ce(nbytes)) {
    rem_len      -= nbytes + 1; 
    return true; 
  } else {
    return false; 
  }
}

bool sch_pdu::has_space_sdu(uint32_t nbytes)
{
  return get_sdu_space() >= nbytes;
}

bool sch_pdu::update_space_sdu(uint32_t nbytes)
{
  int init_rem = rem_len; 
  if (has_space_sdu(nbytes)) {
    if (last_sdu_idx < 0) {
      rem_len      -= (nbytes+1);
    } else {
      rem_len      -= (nbytes+1 + (size_header_sdu(subheaders[last_sdu_idx].get_payload_size())-1));
    }
    last_sdu_idx = cur_idx;
    return true;
  } else {
    return false;
  }
}

int sch_pdu::get_sdu_space()
{
  int ret; 
  if (last_sdu_idx < 0) {
    ret = rem_len - 1;
  } else {
    ret = rem_len - (size_header_sdu(subheaders[last_sdu_idx].get_payload_size())-1) - 1; 
  }
  return ret; 
}

void sch_subh::init()
{
  lcid            = 0; 
  nof_bytes       = 0; 
  payload         = NULL;
}

sch_subh::cetype sch_subh::ce_type()
{
  if (lcid >= PHR_REPORT) {
    return (cetype) lcid;
  } else {
    return SDU;
  }
}

void sch_subh::set_payload_size(uint32_t size) {
  nof_bytes = size; 
}

uint32_t sch_subh::size_plus_header() {
  if (is_sdu()) {
    return sch_pdu::size_header_sdu(nof_bytes) + nof_bytes; 
  } else {
    return nof_bytes + 1;
  }
}

uint32_t sch_subh::sizeof_ce(uint32_t lcid, bool is_ul)
{
  if (is_ul) {
    switch(lcid) {
      case PHR_REPORT: 
        return 1; 
      case C_RNTI: 
        return 2;
      case TRUNC_BSR: 
        return 1;
      case SHORT_BSR: 
        return 1; 
      case LONG_BSR: 
        return 3; 
      case PADDING: 
        return 0; 
    }      
  } else {
    switch(lcid) {
      case CON_RES_ID: 
        return 6;
      case TA_CMD: 
        return 1; 
      case DRX_CMD:
        return 0; 
      case PADDING: 
        return 0; 
    }  
  }
}
bool sch_subh::is_sdu()
{
  return ce_type() == SDU;
}
uint16_t sch_subh::get_c_rnti()
{
  if (payload) {
    return (uint16_t) payload[0] | payload[1]<<8;
  } else {
    return 0;
  }
}
uint64_t sch_subh::get_con_res_id()
{
  if (payload) {
    return ((uint64_t) payload[5]) | (((uint64_t) payload[4])<<8) | (((uint64_t) payload[3])<<16) | (((uint64_t) payload[2])<<24) |
           (((uint64_t) payload[1])<<32) | (((uint64_t) payload[0])<<40);                
  } else {
    return 0; 
  }
}
uint8_t sch_subh::get_phr()
{
  if (payload) {
    return (uint8_t) payload[0]&0x3f;
  } else {
    return 0;
  }
}
uint8_t sch_subh::get_ta_cmd()
{
  if (payload) {
    return (uint8_t) payload[0]&0x3f;
  } else {
    return 0;
  }
}
uint32_t sch_subh::get_sdu_lcid()
{
  return lcid;
}
uint32_t sch_subh::get_payload_size()
{
  return nof_bytes;
}
uint32_t sch_subh::get_header_size(bool is_last) {
  if (!is_last) {
    // For all subheaders, size can be 1, 2 or 3 bytes
    if (is_sdu()) {
      return sch_pdu::size_header_sdu(get_payload_size());
    } else {
      return 1; 
    }
  } else {
    // Last subheader (CE or SDU) has always 1 byte header      
    return 1; 
  }
}
uint8_t* sch_subh::get_sdu_ptr()
{
  return payload;
}
void sch_subh::set_padding(uint32_t padding_len)
{
  lcid = PADDING;
  nof_bytes = padding_len; 
}
void sch_subh::set_padding()
{
  set_padding(0);
}


bool sch_subh::set_bsr(uint32_t buff_size[4], sch_subh::cetype format)
{
  uint32_t nonzero_lcg=0;
  for (int i=0;i<4;i++) {
    if (buff_size[i]) {
      nonzero_lcg=i;
    }
  }
  uint32_t ce_size = format==LONG_BSR?3:1;
  if (((sch_pdu*)parent)->has_space_ce(ce_size)) {
    if (format==LONG_BSR) {
      w_payload_ce[0] = (buff_size_table(buff_size[0])&0x3f) << 2 | (buff_size_table(buff_size[1])&0xc0)>>6;
      w_payload_ce[1] = (buff_size_table(buff_size[1])&0xf)  << 4 | (buff_size_table(buff_size[2])&0xf0)>>4;
      w_payload_ce[2] = (buff_size_table(buff_size[2])&0x3)  << 6 | (buff_size_table(buff_size[3])&0x3f);
    } else {
      w_payload_ce[0] = (nonzero_lcg&0x3)<<6 | buff_size_table(buff_size[nonzero_lcg])&0x3f;
    }
    lcid = format;
    ((sch_pdu*)parent)->update_space_ce(ce_size);
    nof_bytes = ce_size;
    return true; 
  } else {
    return false; 
  }  
}

bool sch_subh::set_c_rnti(uint16_t crnti)
{
  if (((sch_pdu*)parent)->has_space_ce(2)) {
    w_payload_ce[0] = (uint8_t) (crnti&0xff00)>>8;
    w_payload_ce[1] = (uint8_t) (crnti&0x00ff);
    lcid = C_RNTI;
    ((sch_pdu*)parent)->update_space_ce(2);
    nof_bytes = 2; 
    return true; 
  } else {
    return false; 
  }
}
bool sch_subh::set_con_res_id(uint64_t con_res_id)
{
  if (((sch_pdu*)parent)->has_space_ce(6)) {
    w_payload_ce[0] = (uint8_t) ((con_res_id&0xff0000000000)>>48);
    w_payload_ce[1] = (uint8_t) ((con_res_id&0x00ff00000000)>>32);
    w_payload_ce[2] = (uint8_t) ((con_res_id&0x0000ff000000)>>24);
    w_payload_ce[3] = (uint8_t) ((con_res_id&0x000000ff0000)>>16);
    w_payload_ce[4] = (uint8_t) ((con_res_id&0x00000000ff00)>>8);
    w_payload_ce[5] = (uint8_t) ((con_res_id&0x0000000000ff));
    lcid = CON_RES_ID;
    ((sch_pdu*)parent)->update_space_ce(6);
    nof_bytes = 6; 
    return true; 
  } else {
    return false; 
  }
}
bool sch_subh::set_phr(float phr)
{
  if (((sch_pdu*)parent)->has_space_ce(1)) {
    w_payload_ce[0] = phr_report_table(phr)&0x3f; 
    lcid = PHR_REPORT;
    ((sch_pdu*)parent)->update_space_ce(1);
    nof_bytes = 1; 
    return true; 
  } else {
    return false; 
  }
}

bool sch_subh::set_ta_cmd(uint8_t ta_cmd)
{
  if (((sch_pdu*)parent)->has_space_ce(1)) {
    w_payload_ce[0] = ta_cmd&0x3f; 
    lcid = TA_CMD;
    ((sch_pdu*)parent)->update_space_ce(1);
    nof_bytes = 1; 
    return true; 
  } else {
    return false; 
  }
}

int sch_subh::set_sdu(uint32_t lcid_, uint32_t requested_bytes, rlc_interface_mac *rlc)
{
  if (((sch_pdu*)parent)->has_space_sdu(requested_bytes)) {
    lcid = lcid_;
    
    payload = ((sch_pdu*)parent)->get_current_sdu_ptr();
    
    // Copy data and get final number of bytes written to the MAC PDU 
    int sdu_sz = rlc->read_pdu(lcid, payload, requested_bytes);
    
    if (sdu_sz < 0 || sdu_sz > requested_bytes) {
      return -1;
    } 
    if (sdu_sz == 0) {
      return 0; 
    }

    // Save final number of written bytes
    nof_bytes = sdu_sz;

    ((sch_pdu*)parent)->add_sdu(nof_bytes);
    ((sch_pdu*)parent)->update_space_sdu(nof_bytes);
    return nof_bytes; 
  } else {
    return -1; 
  }
}

int sch_subh::set_sdu(uint32_t lcid_, uint32_t nof_bytes_, uint8_t *payload)
{
  if (((sch_pdu*)parent)->has_space_sdu(nof_bytes_)) {
    lcid = lcid_;
    
    memcpy(((sch_pdu*)parent)->get_current_sdu_ptr(), payload, nof_bytes_);
    
    ((sch_pdu*)parent)->add_sdu(nof_bytes_);
    ((sch_pdu*)parent)->update_space_sdu(nof_bytes_);
    nof_bytes = nof_bytes_;

    return (int) nof_bytes; 
  } else {
    return -1; 
  }
}


// Section 6.2.1
void sch_subh::write_subheader(uint8_t** ptr, bool is_last)
{
  *(*ptr)   = (uint8_t) (is_last?0:(1<<5)) | ((uint8_t) lcid & 0x1f);
  *ptr += 1;
  if (is_sdu()) {
    // MAC SDU: R/R/E/LCID/F/L subheader
    // 2nd and 3rd octet
    if (!is_last) {
      if (nof_bytes >= 128) {
        *(*ptr) = (uint8_t) 1<<7 | ((nof_bytes & 0x7f00) >> 8);
        *ptr += 1;
        *(*ptr) = (uint8_t) (nof_bytes & 0xff);
        *ptr += 1;
      } else {
       *(*ptr) = (uint8_t) (nof_bytes & 0x7f); 
       *ptr += 1;
     }      
    }
  } 
}

void sch_subh::write_payload(uint8_t** ptr)
{
  if (is_sdu()) {
    // SDU is written directly during subheader creation
  } else {
    nof_bytes = sizeof_ce(lcid, parent->is_ul()); 
    memcpy(*ptr, w_payload_ce, nof_bytes*sizeof(uint8_t));    
  }
  *ptr += nof_bytes;
}

bool sch_subh::read_subheader(uint8_t** ptr)
{
  // Skip R
  bool e_bit    = (bool)    (*(*ptr) & 0x20)?true:false;
  lcid          = (uint8_t) *(*ptr) & 0x1f;
  *ptr += 1;
  if (is_sdu()) {
    if (e_bit) {
      F_bit     = (bool)    (*(*ptr) & 0x80)?true:false;
      nof_bytes = (uint32_t)*(*ptr) & 0x7f;
      *ptr += 1;
      if (F_bit) {
        nof_bytes = nof_bytes<<8 | (uint32_t) *(*ptr) & 0xff;
        *ptr += 1;
      }
    } else {
      nof_bytes = 0; 
      F_bit = 0; 
    }
  } else {
    nof_bytes = sizeof_ce(lcid, parent->is_ul()); 
  }
  return e_bit;
}
void sch_subh::read_payload(uint8_t** ptr)
{
  payload = *ptr; 
  *ptr += nof_bytes;
}



// Table 6.1.3.1-1 Buffer size levels for BSR 
uint32_t btable[61] = {
  10, 12, 14, 17, 19, 22, 26, 31, 36, 42, 49, 57, 67, 78, 91, 107, 125, 146, 171, 200, 234, 274, 321, 376, 440, 515, 603, 706, 826, 967, 1132, 
  1326, 1552, 1817, 2127, 2490, 2915, 3413, 3995, 4667, 5476, 6411, 7505, 8787, 10287, 12043, 14099, 16507, 19325, 22624, 26487, 31009, 36304, 
  42502, 49759, 58255, 68201, 79846, 93479, 109439, 128125};

uint8_t sch_subh::buff_size_table(uint32_t buffer_size) {
  if (buffer_size == 0) {
    return 0; 
  } else if (buffer_size > 150000) {
    return 63;
  } else {
    for (int i=0;i<61;i++) {
      if (buffer_size < btable[i]) {
        return 1+i; 
      }      
    }
    return 62; 
  }
}
  
// Implements Table 9.1.8.4-1 Power headroom report mapping (36.133)
uint8_t sch_subh::phr_report_table(float phr_value)
{
  if (phr_value < -23) {
    phr_value = -23; 
  }
  if (phr_value > 40) {
    phr_value = 40;
  }
  return (uint8_t) floor(phr_value+23);
}







void rar_pdu::fprint(FILE* stream)
{
  fprintf(stream, "MAC PDU for RAR. ");
  if (has_backoff_indicator) {
    fprintf(stream, "Backoff Indicator %d. ", backoff_indicator);
  }
  pdu::fprint(stream);  
} 


void rar_subh::fprint(FILE* stream)
{
  fprintf(stream, "RAPID: %d, Temp C-RNTI: %d, TA: %d, UL Grant: ", preamble, temp_rnti, ta);
  srslte_vec_fprint_hex(stream, grant, 20);
}

rar_pdu::rar_pdu(uint32_t max_rars_) : pdu(max_rars_)
{
  backoff_indicator = 0; 
  has_backoff_indicator = false; 
}
uint8_t rar_pdu::get_backoff()
{
  return backoff_indicator;
}
bool rar_pdu::has_backoff()
{
  return has_backoff_indicator;
}
void rar_pdu::set_backoff(uint8_t bi)
{
  has_backoff_indicator = true; 
  backoff_indicator = bi; 
}

// Section 6.1.5
bool rar_pdu::write_packet(uint8_t* ptr)
{
  // Write Backoff Indicator, if any 
  if (has_backoff_indicator) {
    if (nof_subheaders > 0) {
      *(ptr) = 1<<7 | backoff_indicator&0xf;
    }
  }
  // Write RAR subheaders
  for (int i=0;i<nof_subheaders;i++) {
    subheaders[i].write_subheader(&ptr, i==nof_subheaders-1);
  }
  // Write payload 
  for (int i=0;i<nof_subheaders;i++) {
    subheaders[i].write_payload(&ptr);
  }
  // Set paddint to zeros (if any) 
  bzero(ptr, rem_len*sizeof(uint8_t));
  
  return true; 
}



void rar_subh::init()
{
  bzero(grant, sizeof(uint8_t) * RAR_GRANT_LEN);
  ta        = 0; 
  temp_rnti = 0; 
}
uint32_t rar_subh::get_rapid()
{
  return preamble; 
}
void rar_subh::get_sched_grant(uint8_t grant_[RAR_GRANT_LEN])
{
  memcpy(grant_, grant, sizeof(uint8_t)*RAR_GRANT_LEN);
}
uint32_t rar_subh::get_ta_cmd()
{
  return ta; 
}
uint16_t rar_subh::get_temp_crnti()
{
  return temp_rnti;
}
void rar_subh::set_rapid(uint32_t rapid)
{
  preamble = rapid; 
}
void rar_subh::set_sched_grant(uint8_t grant_[RAR_GRANT_LEN])
{
  memcpy(grant, grant_, sizeof(uint8_t)*RAR_GRANT_LEN);
}
void rar_subh::set_ta_cmd(uint32_t ta_)
{
  ta = ta_; 
}
void rar_subh::set_temp_crnti(uint16_t temp_rnti_)
{
  temp_rnti = temp_rnti_;
}
// Section 6.2.2
void rar_subh::write_subheader(uint8_t** ptr, bool is_last)
{
  *(*ptr + 0) = (uint8_t) (is_last<<7 | 1<<6 | preamble & 0x3f);
  *ptr += 1;
}
// Section 6.2.3
void rar_subh::write_payload(uint8_t** ptr)
{
  *(*ptr + 0) = (uint8_t)  (ta&0x7f0)>>4;
  *(*ptr + 1) = (uint8_t)  (ta&0xf)  <<4 | grant[0]<<3 | grant[1] << 2 | grant[2] << 1 | grant[3];
  uint8_t *x = &grant[4];
  *(*ptr + 2) = (uint8_t) srslte_bit_pack(&x, 8);
  *(*ptr + 3) = (uint8_t) srslte_bit_pack(&x, 8);
  *(*ptr + 4) = (uint8_t) ((temp_rnti&0xff00) >> 8);
  *(*ptr + 5) = (uint8_t)  (temp_rnti&0x00ff);
  *ptr += 6;
}

void rar_subh::read_payload(uint8_t** ptr)
{
  ta = ((uint32_t) *(*ptr + 0)&0x7f)<<4 | (*(*ptr + 1)&0xf0)>>4;
  grant[0] = *(*ptr + 1)&0x8?1:0;
  grant[1] = *(*ptr + 1)&0x4?1:0;
  grant[2] = *(*ptr + 1)&0x2?1:0;
  grant[3] = *(*ptr + 1)&0x1?1:0;
  uint8_t *x = &grant[4];
  srslte_bit_unpack(*(*ptr+2), &x, 8);
  srslte_bit_unpack(*(*ptr+3), &x, 8);
  temp_rnti = ((uint16_t) *(*ptr + 4))<<8 | *(*ptr + 5);    
  *ptr += 6;
}

bool rar_subh::read_subheader(uint8_t** ptr)
{
  bool e_bit = *(*ptr) & 0x80?true:false;
  bool type  = *(*ptr) & 0x40?true:false;
  if (type) {
    preamble = *(*ptr) & 0x3f;
  } else {
    ((rar_pdu*)parent)->set_backoff(*(*ptr) & 0xf);
  }
  *ptr += 1; 
  return e_bit;
}
    
}



//int main()
//{
//  /* Test 1st message: CCCH + Short BSR + PHR */
//  uint8_t buffer[10240];
//  uint8_t ccch_payload[6] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60};
//  uint32_t bsr_st[4] = {1, 2, 3, 4};
//  srsue::sch_pdu pdu(10);
//  uint8_t *ptr;
  
//  printf("------- CCCH + Short BSR + PHR no padding ----------\n");
//  bzero(buffer, 10240);
//  pdu.init_tx(buffer, 11, true);
//  printf("Available space: %d\n", pdu.rem_size());
//  pdu.new_subh();
//  pdu.get()->set_sdu(0,6,ccch_payload);
//  pdu.new_subh();
//  pdu.get()->set_phr(10);
//  pdu.new_subh();
//  pdu.get()->set_bsr(bsr_st, srsue::sch_subh::SHORT_BSR);
//  printf("Remaining space: %d\n", pdu.rem_size());
//  ptr = pdu.write_packet();
//  srslte_vec_fprint_byte(stdout, ptr, pdu.get_pdu_len());
//  printf("\n");
  
//  /* Test single SDU: SDU 15 + 1 byte header */
//  printf("------- Single SDU no padding ----------\n");
//  uint8_t dlsch_payload[15] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
//  bzero(buffer, 10240);
//  pdu.init_tx(buffer, 16, true);
//  printf("Available space: %d\n", pdu.rem_size());
//  pdu.new_subh();
//  pdu.get()->set_sdu(1, 15, dlsch_payload);
//  printf("Remaining space: %d\n", pdu.rem_size());
//  ptr = pdu.write_packet();
//  srslte_vec_fprint_byte(stdout, ptr, pdu.get_pdu_len());
//  printf("\n");
  
//  /* Test multiple SDU + multiword padding: SDU 8 + SDU 2 byte*/
//  printf("------- Multiple SDU + multiword padding ----------\n");
//  uint8_t dlsch_payload1[8] = {1,2,3,4,5,6,7,8};
//  uint8_t dlsch_payload2[2] = {0xA, 0xB};
//  bzero(buffer, 10240);
//  pdu.init_tx(buffer, 18, true);
//  printf("Available space: %d\n", pdu.rem_size());
//  pdu.new_subh();
//  pdu.get()->set_sdu(2, 8, dlsch_payload1);
//  pdu.new_subh();
//  pdu.get()->set_sdu(3, 2, dlsch_payload2);
//  printf("Remaining space: %d\n", pdu.rem_size());
//  ptr = pdu.write_packet();
//  srslte_vec_fprint_byte(stdout, ptr, pdu.get_pdu_len());
//  printf("\n");
  
//  printf("------- Multiple SDU + 2word padding ----------\n");
//  bzero(buffer, 10240);
//  pdu.init_tx(buffer, 15, true);
//  printf("Available space: %d\n", pdu.rem_size());
//  pdu.new_subh();
//  pdu.get()->set_sdu(2, 8, dlsch_payload1);
//  pdu.new_subh();
//  pdu.get()->set_sdu(3, 2, dlsch_payload2);
//  printf("Remaining space: %d\n", pdu.rem_size());
//  ptr = pdu.write_packet();
//  srslte_vec_fprint_byte(stdout, ptr, pdu.get_pdu_len());
//  printf("\n");
  
//  printf("------- Multiple SDU + 1word padding ----------\n");
//  bzero(buffer, 10240);
//  pdu.init_tx(buffer, 14, true);
//  printf("Available space: %d\n", pdu.rem_size());
//  pdu.new_subh();
//  pdu.get()->set_sdu(2, 8, dlsch_payload1);
//  pdu.new_subh();
//  pdu.get()->set_sdu(3, 2, dlsch_payload2);
//  printf("Remaining space: %d\n", pdu.rem_size());
//  ptr = pdu.write_packet();
//  srslte_vec_fprint_byte(stdout, ptr, pdu.get_pdu_len());
//  printf("\n");

//  printf("------- Multiple SDU + 0word padding ----------\n");
//  bzero(buffer, 10240);
//  pdu.init_tx(buffer, 13, true);
//  printf("Available space: %d\n", pdu.rem_size());
//  pdu.new_subh();
//  pdu.get()->set_sdu(2, 8, dlsch_payload1);
//  pdu.new_subh();
//  pdu.get()->set_sdu(3, 2, dlsch_payload2);
//  printf("Remaining space: %d\n", pdu.rem_size());
//  ptr = pdu.write_packet();
//  srslte_vec_fprint_byte(stdout, ptr, pdu.get_pdu_len());
//  printf("\n");

//  printf("------- Multiple SDU + no space ----------\n");
//  bzero(buffer, 10240);
//  pdu.init_tx(buffer, 12, true);
//  printf("Available space: %d\n", pdu.rem_size());
//  pdu.new_subh();
//  pdu.get()->set_sdu(2, 8, dlsch_payload1);
//  pdu.new_subh();
//  if (pdu.get()->set_sdu(3, 2, dlsch_payload2) < 0) {
//    pdu.del_subh();
//  }
//  printf("Remaining space: %d\n", pdu.rem_size());
//  ptr = pdu.write_packet();
//  srslte_vec_fprint_byte(stdout, ptr, pdu.get_pdu_len());
//  printf("\n");

//  /* CE only */
//  printf("------- CE only ----------\n");
//  bzero(buffer, 10240);
//  pdu.init_tx(buffer, 125, true);
//  printf("Available space: %d\n", pdu.rem_size());
//  pdu.new_subh();
//  pdu.get()->set_phr(15);
//  pdu.new_subh();
//  pdu.get()->set_bsr(bsr_st, srsue::sch_subh::SHORT_BSR);
//  printf("Remaining space: %d\n", pdu.rem_size());
//  ptr = pdu.write_packet();
//  srslte_vec_fprint_byte(stdout, ptr, pdu.get_pdu_len());
//  printf("\n");
  
//  /* Another test */
//  printf("------- Another test ----------\n");
//  uint8_t dlsch_payload3[602];
//  bzero(buffer, 10240);
//  pdu.init_tx(buffer, 75, true);
//  printf("Available space: %d\n", pdu.rem_size());
//  pdu.new_subh();
//  pdu.get()->set_bsr(bsr_st, srsue::sch_subh::SHORT_BSR);
//  pdu.new_subh();
//  pdu.get()->set_sdu(3, 2, dlsch_payload3);
//  pdu.new_subh();
//  pdu.get()->set_sdu(3, 66, dlsch_payload3);
//  pdu.new_subh();
//  printf("Remaining space: %d\n", pdu.rem_size());
//  ptr = pdu.write_packet();
//  //srslte_vec_fprint_byte(stdout, ptr, pdu.get_pdu_len());
//  printf("\n");
  
//  return 0;
//}



