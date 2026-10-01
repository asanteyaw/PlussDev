#pragma once

#include "pluss/export.h"

#include <cstdint>
#include <iosfwd>
#include <string>
#include <string_view>

namespace pluss::codeh {

class Timeline;
class Duration;
class TimeIndex;

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

// TimeIndex is a 1D ordered Timeline axis, so its values render exactly like
// Timeline's -- this reuses Timeline's own (shape-aware) rendering rather
// than duplicating it, relabeling only the leading type name in the header.
std::string to_string(const TimeIndex& idx,
                      const TimelinePrintOptions& opt = {});
std::string print_timeindex(const TimeIndex& idx,
                            const TimelinePrintOptions& opt = {});
std::ostream& print_timeindex(std::ostream& os,
                             const TimeIndex& idx,
                             const TimelinePrintOptions& opt = {});
std::ostream& operator<<(std::ostream& os, const TimeIndex& idx);

} // namespace pluss::codeh
