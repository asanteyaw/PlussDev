#pragma once

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <fmt/format.h>

namespace pluss::plotism::detail {

inline std::string escape_xml(std::string_view s) {
  std::string out;
  out.reserve(s.size() + 8);
  for (char c : s) {
    switch (c) {
      case '&': out += "&amp;"; break;
      case '<': out += "&lt;"; break;
      case '>': out += "&gt;"; break;
      case '"': out += "&quot;"; break;
      case '\'': out += "&apos;"; break;
      default: out.push_back(c); break;
    }
  }
  return out;
}

inline std::string escape_json(std::string_view s) {
  std::string out;
  out.reserve(s.size() + 8);
  for (char c : s) {
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"': out += "\\\""; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default: out.push_back(c); break;
    }
  }
  return out;
}

inline std::string format_double(double v, int precision = 8) {
  if (!std::isfinite(v)) {
    return "NaN";
  }
  return fmt::format("{:.{}g}", v, precision);
}

inline std::string vector_to_json(const std::vector<double>& values) {
  fmt::memory_buffer buf;
  fmt::format_to(std::back_inserter(buf), "[");
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i > 0) {
      fmt::format_to(std::back_inserter(buf), ",");
    }
    if (!std::isfinite(values[i])) {
      fmt::format_to(std::back_inserter(buf), "null");
    } else {
      fmt::format_to(std::back_inserter(buf), "{:.17g}", values[i]);
    }
  }
  fmt::format_to(std::back_inserter(buf), "]");
  return fmt::to_string(buf);
}

inline void ensure_same_size(const std::vector<double>& a,
                             const std::vector<double>& b,
                             std::string_view where) {
  if (a.size() != b.size()) {
    throw std::invalid_argument(std::string("plotism::") + std::string(where) + ": vector size mismatch");
  }
}

inline void write_text_file(std::string_view path, const std::string& contents) {
  std::ofstream out(std::string(path), std::ios::binary);
  if (!out) {
    throw std::runtime_error("plotism: unable to open output file: " + std::string(path));
  }
  out << contents;
  out.flush();
  if (!out.good()) {
    throw std::runtime_error("plotism: failed writing output file: " + std::string(path));
  }
}

inline const std::vector<std::string>& default_palette() {
  static const std::vector<std::string> palette{
      "#1f77b4", "#ff7f0e", "#2ca02c", "#d62728", "#9467bd",
      "#8c564b", "#e377c2", "#7f7f7f", "#bcbd22", "#17becf"};
  return palette;
}

inline std::pair<double, double> finite_minmax(const std::vector<double>& values) {
  double lo = std::numeric_limits<double>::infinity();
  double hi = -std::numeric_limits<double>::infinity();
  for (const auto v : values) {
    if (!std::isfinite(v)) {
      continue;
    }
    lo = std::min(lo, v);
    hi = std::max(hi, v);
  }
  if (!std::isfinite(lo) || !std::isfinite(hi)) {
    return {0.0, 1.0};
  }
  if (lo == hi) {
    return {lo - 0.5, hi + 0.5};
  }
  return {lo, hi};
}

} // namespace pluss::plotism::detail
