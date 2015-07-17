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

#ifndef LOG_FILTER_H
#define LOG_FILTER_H

#include <string>
#include "srsapps/common/log.h"

namespace srsue {

class log_filter : public srslte::log
{
public:

  log_filter(std::string layer)
    :log(layer)
  {}

  void error(std::string message, ...);
  void warning(std::string message, ...);
  void info(std::string message, ...);
  void debug(std::string message, ...);

  void error_hex(uint8_t *hex, int size, std::string message, ...);
  void warning_hex(uint8_t *hex, int size, std::string message, ...);
  void info_hex(uint8_t *hex, int size, std::string message, ...);
  void debug_hex(uint8_t *hex, int size, std::string message, ...);

private:

};

} // namespace srsue

#endif // LOG_FILTER_H
