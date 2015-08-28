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

#define LOG_BUFFER_SIZE 1024*32

#include "ue_logger.h"

using namespace std;

namespace srsue{

ue_logger::ue_logger(std::string file)
  :buffer(LOG_BUFFER_SIZE)
  ,filename(file)
  ,not_done(true)
{
  logfile = fopen(filename.c_str(), "a");
  if(logfile==NULL) {
    printf("Error: could not create log file, no messages will be logged");
  }
  pthread_create(&thread, NULL, &start, this);
}

ue_logger::~ue_logger() {
  not_done = false;
  log("Closing log");
  pthread_join(thread, NULL);
  flush();
  fclose(logfile);
}

void ue_logger::log(const char *msg) {
  str_ptr s_ptr(new std::string(msg));
  log(s_ptr);
}

void ue_logger::log(str_ptr msg) {
    boost::mutex::scoped_lock lock(mutex);
    if(buffer.full()) {
      buffer.set_capacity(buffer.capacity()*2);
      if(logfile)
        fprintf(logfile, "Log queue full, doubling capacity\n");
    }
    buffer.push_back(msg);
    lock.unlock();
    not_empty.notify_one();
}

void* ue_logger::start(void *input) {
  ue_logger *l = (ue_logger*)input;
  l->reader_loop();
}

void ue_logger::reader_loop() {
  while(not_done) {
    boost::mutex::scoped_lock lock(mutex);
    while(buffer.empty()) not_empty.wait(lock);
    str_ptr s = buffer.front();
    buffer.pop_front();
    lock.unlock();
    not_full.notify_one();
    if(logfile)
      fprintf(logfile, "%s\n", s.get()->c_str());
  }
}

void ue_logger::flush() {
  boost::circular_buffer<str_ptr>::iterator it;
  for(it=buffer.begin();it!=buffer.end();it++)
  {
    if(logfile)
      fprintf(logfile, "%s\n", it->get()->c_str());
  }
}

} // namespace srsue
