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

#ifndef MUX_H
#define MUX_H

#include <pthread.h>

#include "common/qbuff.h"
#include "common/log.h"
#include "common/mac_interface.h"
#include "mac/mac_params.h"
#include "mac/pdu.h"
#include "mac/proc_bsr.h"
#include "mac/proc_phr.h"

/* Logical Channel Multiplexing and Prioritization + Msg3 Buffer */   


namespace srsue {

class mux
{
public:
  mux();
  void     reset();
  void     init(rlc_interface_mac *rlc, srslte::log *log_h, bsr_proc *bsr_procedure, phr_proc *phr_procedure_);

  bool     is_pending_ccch_sdu();
  bool     is_pending_any_sdu();
  bool     is_pending_sdu(uint32_t lcid); 
  
  uint8_t* pdu_get(uint8_t *payload, uint32_t pdu_sz);
  uint8_t* msg3_get(uint8_t* payload, uint32_t pdu_sz);
  
  void     msg3_flush();
  bool     msg3_is_transmitted();
  
  void     append_crnti_ce_next_tx(uint16_t crnti); 
  
  void     set_priority(uint32_t lcid, uint32_t priority, int PBR_x_tti, uint32_t BSD);
      
private:  
  bool     pdu_move_to_msg3(uint32_t pdu_sz);
  bool     allocate_sdu(uint32_t lcid, sch_pdu *pdu, int max_sdu_sz, uint32_t *sdu_sz);
  
  const static int NOF_UL_LCH = 10; 
  const static int MIN_RLC_SDU_LEN = 0; 
  const static int MAX_NOF_SUBHEADERS = 20; 
  
  int64_t       Bj[NOF_UL_LCH];
  int           PBR[NOF_UL_LCH]; // -1 sets to infinity
  uint32_t      BSD[NOF_UL_LCH];
  uint32_t      priority[NOF_UL_LCH];
  uint32_t      priority_sorted[NOF_UL_LCH];
  uint32_t      lchid_sorted[NOF_UL_LCH];
  
  // Mutex for exclusive access
  pthread_mutex_t mutex; 

  srslte::log       *log_h;
  rlc_interface_mac *rlc; 
  bsr_proc          *bsr_procedure;
  phr_proc          *phr_procedure;
  uint16_t           pending_crnti_ce;
  
  /* Msg3 Buffer */
  static const uint32_t MSG3_BUFF_SZ = 128; 
  srslte::qbuff         msg3_buff;
  
  /* PDU Buffer */
  sch_pdu               pdu_msg; 
  bool msg3_has_been_transmitted;
  
  
  
};

} // namespace srsue

#endif // MUX_H

