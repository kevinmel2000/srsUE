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

/******************************************************************************
 *  File:         circ_buf.h
 *  Description:  Simple circular buffer of srsue_byte_buffer pointers.
 *  Reference:
 *****************************************************************************/

#ifndef CIRC_BUF_H
#define CIRC_BUF_H

#include "common/common.h"

namespace srsue {

class circ_buf
{
public:
  circ_buf(uint32_t capacity_ = 10)
    :head(0)
    ,tail(0)
    ,unread(0)
    ,capacity(capacity_)
  {
    buf = new srsue_byte_buffer_t*[capacity];
  }

  ~circ_buf()
  {
    delete [] buf;
  }

  void write(srsue_byte_buffer_t *msg)
  {
    buf[head] = msg;
    head = (head+1)%capacity;
    unread = (unread+1)%capacity;
  }

  bool try_read(srsue_byte_buffer_t **msg)
  {
    if(is_empty())
    {
      return false;
    }else{
      *msg = buf[tail];
      tail = (tail+1)%capacity;
      unread--;
      return true;
    }
  }

  uint32_t  size()      const { return unread; }
  bool      is_empty()  const { return unread == 0; }
  bool      is_full()   const { return unread == capacity; }

private:
  srsue_byte_buffer_t **buf;
  uint32_t              capacity;
  uint32_t              unread;
  uint32_t              head;
  uint32_t              tail;
};

} // namespace srsue


#endif // CIRC_BUF_H
