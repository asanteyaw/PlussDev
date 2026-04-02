#pragma once

#include "pluss/export.h"

#include <cstdint>
#include <iosfwd>
#include <string>
#include <string_view>

namespace pluss::codeh {

class Timeline;

struct TimelinePrintOptions {
  std::string_view format = "%Y-%m-%d %H:%M:%S";
  std::int64_t edge_items = 3;
  std::int64_t max_rows = 10;
  std::int64_t max_cols = 7;
  std::int64_t max_slices = 6;
  bool show_header = true;
  bool show_shape = true;
  bool show_tz = true;
};

std::string to_string(const Timeline& t, const TimelinePrintOptions& opt = {});
std::string print_timeline(const Timeline& t, const TimelinePrintOptions& opt = {});
std::ostream& print_timeline(std::ostream& os,
                             const Timeline& t,
                             const TimelinePrintOptions& opt = {});
std::ostream& operator<<(std::ostream& os, const Timeline& t);

} // namespace pluss::codeh
