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

/**
 * File:        logger.h
 * Description: Common log object. Maintains a queue of log messages
 *              and runs a thread to read messages and write to file.
 *              Multiple producers, single consumer. If full, producers
 *              increase queue size. If empty, consumer blocks.
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <string>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/circular_buffer.hpp>

namespace srsue {

class logger
{
public:
  logger(std::string filename);
  ~logger();
  void log(const char *msg);
  void log(std::string &msg);

private:
  static void* start(void *input);
  void reader_loop();
  void flush();

  FILE*                               logfile;
  bool                                not_done;
  std::string                         filename;
  boost::condition                    not_empty;
  boost::condition                    not_full;
  boost::mutex                        mutex;
  pthread_t                           thread;
  boost::circular_buffer<std::string> buffer;
};

} // namespace srsue

#endif // LOGGER_H
