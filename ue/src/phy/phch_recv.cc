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

#include <unistd.h>
#include "srslte/srslte.h"
#include "common/log.h"
#include "phy/phch_worker.h"
#include "phy/phch_common.h"
#include "phy/phch_recv.h"

#define Error(fmt, ...)   if (SRSLTE_DEBUG_ENABLED) log_h->error_line(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define Warning(fmt, ...) if (SRSLTE_DEBUG_ENABLED) log_h->warning_line(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define Info(fmt, ...)    if (SRSLTE_DEBUG_ENABLED) log_h->info_line(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define Debug(fmt, ...)   if (SRSLTE_DEBUG_ENABLED) log_h->debug_line(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

namespace srsue {
 

phch_recv::phch_recv() { 
  running = false; 
}

bool phch_recv::init(srslte::radio* _radio_handler, mac_interface_phy *_mac, prach* _prach_buffer, srslte::thread_pool* _workers_pool,
                     phch_common* _worker_com, srslte::log* _log_h, bool do_agc_, uint32_t prio)
{
  radio_h      = _radio_handler;
  log_h        = _log_h;     
  mac          = _mac; 
  workers_pool = _workers_pool;
  worker_com   = _worker_com;
  prach_buffer = _prach_buffer; 
  tx_mutex_cnt = 0; 
  running      = true; 
  phy_state    = IDLE; 
  time_adv_sec = 0; 
  cell_is_set  = false; 
  do_agc       = do_agc_;

  nof_tx_mutex = MUTEX_X_WORKER*workers_pool->get_nof_workers();
  worker_com->set_nof_mutex(nof_tx_mutex);
    
  start(prio);
}

void phch_recv::stop() {
  running = false; 
  wait_thread_finish();
}

int radio_recv_wrapper_cs(void *h, void *data, uint32_t nsamples, srslte_timestamp_t *rx_time)
{
  srslte::radio *radio_h = (srslte::radio*) h;
  if (radio_h->rx_now(data, nsamples, rx_time)) {
    int offset = nsamples-radio_h->get_tti_len();
    if (abs(offset)<10 && offset != 0) {
      radio_h->tx_offset(offset);
    }
    return nsamples;
  } else {
    return -1;
  }
}

double callback_set_rx_gain(void *h, double gain) {
  srslte::radio *radio_handler = (srslte::radio*) h;
  return radio_handler->set_rx_gain_th(gain);
}

void phch_recv::set_time_adv_sec(float _time_adv_sec) {
 time_adv_sec = _time_adv_sec;
}

bool phch_recv::init_cell() {
  cell_is_set = false;
  if (!srslte_ue_mib_init(&ue_mib, cell)) 
  {
    if (!srslte_ue_sync_init(&ue_sync, cell, radio_recv_wrapper_cs, radio_h)) 
    {
      
      for (int i=0;i<workers_pool->get_nof_workers();i++) {
        if (!((phch_worker*) workers_pool->get_worker(i))->init_cell(cell)) {
          Error("Error setting cell: initiating PHCH worker\n");
          return false; 
        }
      }
      radio_h->set_tti_len(SRSLTE_SF_LEN_PRB(cell.nof_prb));
      if (do_agc) {
        srslte_ue_sync_start_agc(&ue_sync, callback_set_rx_gain, last_gain);    
      }
      srslte_ue_sync_set_cfo(&ue_sync, cellsearch_cfo);
      cell_is_set = true;                             
    } else {
      Error("Error setting cell: initiating ue_sync");      
    }
  } else {
    Error("Error setting cell: initiating ue_mib\n"); 
  }      
  return cell_is_set; 
}

void phch_recv::free_cell()
{
  if (cell_is_set) {
    for (int i=0;i<workers_pool->get_nof_workers();i++) {
      ((phch_worker*) workers_pool->get_worker(i))->free_cell();
    }
    prach_buffer->free_cell();
  }
}


bool phch_recv::cell_search(int force_N_id_2) 
{
  uint8_t bch_payload[SRSLTE_BCH_PAYLOAD_LEN];
  uint8_t bch_payload_bits[SRSLTE_BCH_PAYLOAD_LEN/8];
  
  srslte_ue_cellsearch_result_t found_cells[3];
  srslte_ue_cellsearch_t        cs; 

  bzero(found_cells, 3*sizeof(srslte_ue_cellsearch_result_t));

  log_h->console("Searching for cell...\n");
  if (srslte_ue_cellsearch_init(&cs, radio_recv_wrapper_cs, radio_h)) {
    Error("Initiating UE cell search\n");
    return false; 
  }
  
  if (do_agc) {
    srslte_ue_sync_start_agc(&cs.ue_sync, callback_set_rx_gain, last_gain);
  }
  
  srslte_ue_cellsearch_set_nof_frames_to_scan(&cs, 
    worker_com->params_db->get_param(phy_interface_params::CELLSEARCH_TIMEOUT_PSS_NFRAMES));
  srslte_ue_cellsearch_set_threshold(&cs, (float) 
    worker_com->params_db->get_param(phy_interface_params::CELLSEARCH_TIMEOUT_PSS_CORRELATION_THRESHOLD)/10);

  radio_h->set_rx_srate(1.92e6);
  radio_h->start_rx();
  
  /* Find a cell in the given N_id_2 or go through the 3 of them to find the strongest */
  uint32_t max_peak_cell = 0;
  int ret = SRSLTE_ERROR; 
  
  if (force_N_id_2 >= 0 && force_N_id_2 < 3) {
    ret = srslte_ue_cellsearch_scan_N_id_2(&cs, force_N_id_2, &found_cells[force_N_id_2]);
    max_peak_cell = force_N_id_2;
  } else {
    ret = srslte_ue_cellsearch_scan(&cs, found_cells, &max_peak_cell); 
  }

  last_gain = srslte_agc_get_gain(&cs.ue_sync.agc);

  radio_h->stop_rx();
  srslte_ue_cellsearch_free(&cs);
  
  if (ret < 0) {
    Error("Error decoding MIB: Error searching PSS\n");
    return false;
  } else if (ret == 0) {
    Error("Error decoding MIB: Could not find any PSS in this frequency\n");
    return false;
  }
    
  // Save result 
  cell.id   = found_cells[max_peak_cell].cell_id;
  cell.cp   = found_cells[max_peak_cell].cp; 
  cellsearch_cfo = found_cells[max_peak_cell].cfo;
  
  log_h->console("Found CELL ID: %d CP: %s, CFO: %.1f KHz.\nTrying to decode MIB...\n", cell.id, srslte_cp_string(cell.cp), cellsearch_cfo/1000);
  
  srslte_ue_mib_sync_t ue_mib_sync; 

  if (srslte_ue_mib_sync_init(&ue_mib_sync, cell.id, cell.cp, radio_recv_wrapper_cs, radio_h)) {
    Error("Initiating UE MIB synchronization\n");
    return false; 
  }
  
  if (do_agc) {
    srslte_ue_sync_start_agc(&ue_mib_sync.ue_sync, callback_set_rx_gain, last_gain);    
  }

  /* Find and decode MIB */
  uint32_t sfn, sfn_offset; 
  radio_h->start_rx();
  ret = srslte_ue_mib_sync_decode(&ue_mib_sync, 
                                  worker_com->params_db->get_param(phy_interface_params::CELLSEARCH_TIMEOUT_MIB_NFRAMES), 
                                  bch_payload, &cell.nof_ports, &sfn_offset); 
  radio_h->stop_rx();
  last_gain = srslte_agc_get_gain(&ue_mib_sync.ue_sync.agc);
  srslte_ue_mib_sync_free(&ue_mib_sync);

  if (ret == 1) {
    srslte_pbch_mib_unpack(bch_payload, &cell, NULL);
    worker_com->set_cell(cell);
    srslte_cell_fprint(stdout, &cell, 0);
    //FIXME: this is temporal
    srslte_bit_pack_vector(bch_payload, bch_payload_bits, SRSLTE_BCH_PAYLOAD_LEN);
    mac->bch_decoded_ok(bch_payload_bits, SRSLTE_BCH_PAYLOAD_LEN/8);
    return true;     
  } else {
    Warning("Error decoding MIB: Error decoding PBCH\n");      
    return false;
  }
}


int phch_recv::sync_sfn(void) {
  
  cf_t *sf_buffer = NULL; 
  int ret = SRSLTE_ERROR; 
  uint8_t bch_payload[SRSLTE_BCH_PAYLOAD_LEN];

  srslte_ue_sync_decode_sss_on_track(&ue_sync, true);
  ret = srslte_ue_sync_get_buffer(&ue_sync, &sf_buffer);
  if (ret < 0) {
    Error("Error calling ue_sync_get_buffer");      
    return -1;
  }
    
  if (ret == 1) {
    if (srslte_ue_sync_get_sfidx(&ue_sync) == 0) {
      uint32_t sfn_offset=0;
      srslte_pbch_decode_reset(&ue_mib.pbch);
      int n = srslte_ue_mib_decode(&ue_mib, sf_buffer, bch_payload, NULL, &sfn_offset);
      if (n < 0) {
        Error("Error decoding MIB while synchronising SFN");      
        return -1; 
      } else if (n == SRSLTE_UE_MIB_FOUND) {  
        uint32_t sfn; 
        srslte_pbch_mib_unpack(bch_payload, &cell, &sfn);

        sfn = (sfn + sfn_offset)%1024;         
        tti = sfn*10 + srslte_ue_sync_get_sfidx(&ue_sync);
        
        srslte_ue_sync_decode_sss_on_track(&ue_sync, false);
        return 1;
      }
    }    
  }
  return 0;
}

void phch_recv::run_thread()
{
  phch_worker *worker = NULL;
  cf_t *buffer = NULL;
  while(running) {
    switch(phy_state) {
      case CELL_SEARCH:
        if (cell_search()) {
          init_cell();
          float srate = (float) srslte_sampling_freq_hz(cell.nof_prb); 
          if (srate < 10e6) {
            radio_h->set_master_clock_rate(4*srate);        
          } else {
            radio_h->set_master_clock_rate(srate);        
          }
          printf("Setting Sampling frequency %.2f MHz\n", (float) srate/1000000);

          radio_h->set_rx_srate(srate);
          radio_h->set_tx_srate(srate);
          Info("Cell found. Synchronizing...\n");
          phy_state = SYNCING;
        }
        break;
      case SYNCING:
        if (!radio_is_streaming) {
          // Start streaming
          radio_h->start_rx();
          radio_is_streaming = true; 
        }
          
        switch(sync_sfn()) {
          default:
            log_h->console("Going IDLE\n");
            phy_state = IDLE; 
            break; 
          case 1:
            srslte_ue_sync_set_agc_period(&ue_sync, 20);
            phy_state = SYNC_DONE;  
            break;        
          case 0:
            break;        
        } 
       break;
      case SYNC_DONE:
        /* Set synchronization track phase threshold and averaging factor */
        if (worker_com->params_db->get_param(phy_interface_params::SYNC_TRACK_THRESHOLD) > 0) {
          srslte_sync_set_threshold(&ue_sync.strack, (float) worker_com->params_db->get_param(phy_interface_params::SYNC_TRACK_THRESHOLD)/10);          
        }
        if (worker_com->params_db->get_param(phy_interface_params::SYNC_TRACK_AVG_COEFF) > 0) {
          srslte_sync_set_em_alpha(&ue_sync.strack, (float) worker_com->params_db->get_param(phy_interface_params::SYNC_TRACK_THRESHOLD)/10);
        }
        
        tti = (tti+1)%10240;        
        worker = (phch_worker*) workers_pool->wait_worker(tti);
        if (worker) {          
          buffer = worker->get_buffer();
          if (srslte_ue_sync_zerocopy(&ue_sync, buffer) == 1) {
            log_h->step(tti);

            Debug("Worker %d synchronized\n", worker->get_id());
            
            metrics.sfo = srslte_ue_sync_get_sfo(&ue_sync);
            metrics.cfo = srslte_ue_sync_get_cfo(&ue_sync);
            worker->set_cfo(metrics.cfo/15000);
            worker_com->set_sync_metrics(metrics);
    
            /* Compute TX time: Any transmission happens in TTI+4 thus advance 4 ms the reception time */
            srslte_timestamp_t rx_time, tx_time, tx_time_prach; 
            srslte_ue_sync_get_last_timestamp(&ue_sync, &rx_time); 
            srslte_timestamp_copy(&tx_time, &rx_time);
            srslte_timestamp_copy(&tx_time_prach, &rx_time);
            srslte_timestamp_add(&tx_time, 0, 4e-3 - time_adv_sec);
            srslte_timestamp_add(&tx_time_prach, 0, 4e-3);
            worker->set_tx_time(tx_time);
            
            Debug("Settting TTI=%d, tx_mutex=%d to worker %d\n", tti, tx_mutex_cnt, worker->get_id());
            worker->set_tti(tti, tx_mutex_cnt);
            tx_mutex_cnt = (tx_mutex_cnt+1)%nof_tx_mutex;

            // Check if we need to TX a PRACH 
            if (prach_buffer->is_ready_to_send(tti)) {
              srslte_timestamp_t cur_time; 
              radio_h->get_time(&cur_time);
              Info("TX PRACH now. RX time: %d:%f, Now: %d:%f\n", rx_time.full_secs, rx_time.frac_secs, 
                   cur_time.full_secs, cur_time.frac_secs);
              // send prach if we have to 
              prach_buffer->send(radio_h, metrics.cfo/15000, worker_com->pathloss, tx_time_prach);
              radio_h->tx_end();            
              worker_com->p0_preamble = prach_buffer->get_p0_preamble();
              worker_com->cur_radio_power = SRSLTE_MIN(SRSLTE_PC_MAX, worker_com->pathloss + worker_com->p0_preamble);
            }            
            workers_pool->start_worker(worker);             
            mac->tti_clock(tti);
          } else {
            log_h->console("Sync Error!\n");
            worker->release();
            phy_state = SYNCING;
            worker_com->reset_ul();
          }
        } else {
          // wait_worker() only returns NULL if it's being closed. Quit now to avoid unnecessary loops here
          running = false; 
        }
        break;
      case IDLE:
        usleep(1000);
        break;
    }
  }
}

uint32_t phch_recv::get_current_tti()
{
  return tti; 
}

bool phch_recv::status_is_sync()
{
  return phy_state == SYNC_DONE;
}

void phch_recv::get_current_cell(srslte_cell_t* cell_)
{
  if (cell_) {
    memcpy(cell_, &cell, sizeof(srslte_cell_t));
  }
}

void phch_recv::sync_start()
{
  radio_h->set_master_clock_rate(30.72e6);        
  phy_state = CELL_SEARCH;
}

void phch_recv::sync_stop()
{
  free_cell();
  radio_h->stop_rx();
  radio_is_streaming = false; 
  phy_state = IDLE; 
}

}
