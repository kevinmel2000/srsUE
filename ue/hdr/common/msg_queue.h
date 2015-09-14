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

#ifndef MSG_QUEUE_H
#define MSG_QUEUE_H

#include "common/common.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/circular_buffer.hpp>

namespace srsue {

class msg_queue
{
public:
  msg_queue(uint32_t size = 10)
    :unread(0)
    ,circ_buf(size)
  {}

  void write(srsue_byte_buffer_t &msg)
  {
    boost::mutex::scoped_lock lock(mutex);
    while(is_full()) not_full.wait(lock);
    circ_buf.push_front(msg);
    ++unread;
    lock.unlock();
    not_empty.notify_one();
  }

  void write(uint8_t *payload, uint32_t nof_bytes)
  {
    boost::mutex::scoped_lock lock(mutex);
    while(is_full()) not_full.wait(lock);
    srsue_byte_buffer_t msg;
    memcpy(msg.msg, payload, nof_bytes);
    msg.N_bytes = nof_bytes;
    circ_buf.push_front(msg);
    ++unread;
    lock.unlock();
    not_empty.notify_one();
  }

  void read(srsue_byte_buffer_t &msg)
  {
    boost::mutex::scoped_lock lock(mutex);
    while(is_empty()) not_empty.wait(lock);
    msg = circ_buf[--unread];
    lock.unlock();
    not_full.notify_one();
  }

  bool try_read(srsue_byte_buffer_t &msg)
  {
    boost::mutex::scoped_lock lock(mutex);
    if(is_empty())
    {
      return false;
    }else{
      msg = circ_buf[--unread];
      lock.unlock();
      not_full.notify_one();
      return true;
    }
  }

private:
  bool is_empty() const { return unread == 0; }
  bool is_full() const { return unread == circ_buf.capacity(); }

  boost::condition                             not_empty;
  boost::condition                             not_full;
  boost::mutex                                 mutex;
  uint32_t                                     unread;
  boost::circular_buffer<srsue_byte_buffer_t>  circ_buf;
};

} // namespace srsue


#endif // MSG_QUEUE_H
