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

/******************************************************************************
 *  File:         timeout.h
 *  Description:  Millisecond resolution timeouts. Uses a dedicated thread to
 *                call an optional callback function upon timeout expiry.
 *  Reference:
 *****************************************************************************/

#ifndef TIMEOUT_H
#define TIMEOUT_H

#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace srsue {
  
class timeout_callback
{
  public: 
    virtual void timeout_expired(uint32_t timeout_id) = 0;
}; 
  
class timeout
{
public:
  timeout():exp(false){}
  void start(uint32_t timeout_id_, uint32_t duration_msec_, timeout_callback *callback_=NULL)
  {
    exp = false;
    start_time = boost::posix_time::microsec_clock::local_time();
    timeout_id    = timeout_id_;
    duration_usec = duration_msec_*1000;
    callback      = callback_;

    pthread_create(&thread, NULL, &thread_start, this);
  }
  static void* thread_start(void *t_)
  {
    timeout *t = (timeout*)t_;
    t->thread_func();
  }
  void thread_func()
  {
    boost::posix_time::time_duration diff;
    boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
    diff = now - start_time;
    duration_usec -= diff.total_microseconds();
    if(duration_usec > 0)
      usleep(duration_usec);
    exp = true;
    if(callback)
        callback->timeout_expired(timeout_id);
  }
  bool expired()
  {
      return exp;
  }

private:
  boost::posix_time::ptime  start_time;
  pthread_t                 thread;
  uint32_t                  timeout_id;
  uint32_t                  duration_usec;
  timeout_callback         *callback;
  bool                      exp;
};

} // namespace srsue
  
#endif // TIMEOUT_H
