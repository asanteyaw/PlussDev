#pragma once

#include <iomanip>
#include <sstream>
#include <string>

namespace pluss::subsp::stats::regression::detail {

// Column headings of the confidence-interval pair in a printed summary: "[0.025" and "0.975]" for alpha = 0.05, "[0.05" and "0.95]" for
// alpha = 0.10, so the heading always names the limits the intervals were computed at.
inline std::string ci_lower_label(double alpha) {
  std::ostringstream oss;
  oss << '[' << std::setprecision(4) << 0.5 * alpha;
  return oss.str();
}

inline std::string ci_upper_label(double alpha) {
  std::ostringstream oss;
  oss << std::setprecision(4) << 1.0 - 0.5 * alpha << ']';
  return oss.str();
}

} // namespace pluss::subsp::stats::regression::detail
