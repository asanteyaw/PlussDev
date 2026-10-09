#pragma once

#include "pluss/export.h"

#include <cstdint>
#include <iosfwd>
#include <string>
#include <string_view>

namespace pluss::codeh {

class Timeline;
class Duration;
class TimelineIndexAccessor;

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

// Duration has no public layout options. N-D arrays use the normalized
// "[D] days HH:MM:SS.nnnnnnnnn" representation returned by to_note(); the
// separate to_note_iso() method produces ISO duration text.
std::string to_string(const Duration& d);
std::string print_duration(const Duration& d);
std::ostream& print_duration(std::ostream& os, const Duration& d);
std::ostream& operator<<(std::ostream& os, const Duration& d);

// The index accessor is a 1D ordered Timeline axis, so its values reuse
// Timeline's shape-aware rendering with the public accessor name in the header.
std::string to_string(const TimelineIndexAccessor& idx,
                      const TimelinePrintOptions& opt = {});
std::string print_index(const TimelineIndexAccessor& idx,
                            const TimelinePrintOptions& opt = {});
std::ostream& print_index(std::ostream& os,
                             const TimelineIndexAccessor& idx,
                             const TimelinePrintOptions& opt = {});
std::ostream& operator<<(std::ostream& os, const TimelineIndexAccessor& idx);

} // namespace pluss::codeh
