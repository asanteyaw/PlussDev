#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pluss::plotism {

enum class LegendLocation : std::uint8_t {
  Best = 0,
  TopLeft = 1,
  TopRight = 2,
  BottomLeft = 3,
  BottomRight = 4,
};

struct Style {
  std::string color{};
  std::string line_style{"solid"};
  std::string marker{"none"};
  double line_width{1.5};
  double alpha{1.0};
};

struct Axis {
  std::string label{};
  std::optional<double> min{std::nullopt};
  std::optional<double> max{std::nullopt};
  bool log_scale{false};
};

struct Legend {
  bool visible{true};
  LegendLocation location{LegendLocation::Best};
  std::string title{};
};

namespace detail {

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
  std::ostringstream os;
  os << std::setprecision(precision) << v;
  return os.str();
}

inline std::string vector_to_json(const std::vector<double>& values) {
  std::ostringstream os;
  os << "[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i > 0) {
      os << ",";
    }
    if (!std::isfinite(values[i])) {
      os << "null";
    } else {
      os << std::setprecision(17) << values[i];
    }
  }
  os << "]";
  return os.str();
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
      "#1f77b4", "#ff7f0e", "#2ca02c", "#d62728", "#9467bd", "#8c564b", "#e377c2", "#7f7f7f", "#bcbd22", "#17becf"};
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

} // namespace detail
} // namespace pluss::plotism

