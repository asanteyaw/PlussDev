#include "CodeH/core/Printing.h"

#include <algorithm>
#include <cstdio>
#include <iomanip>
#include <span>
#include <sstream>
#include <string>
#include <vector>

#include "CodeH/core/Duration.h"
#include "CodeH/core/Timeline.h"
#include "CodeH/core/TimelineIndexAccessor.h"

namespace pluss::codeh {

namespace {

// Same floor-normalized "[D] days HH:MM:SS.nnnnnnnnn" representation as
// Duration::to_note(). The sub-day fields therefore stay non-negative even
// when the duration itself is negative.
static std::string format_duration_one_(std::int64_t ns) {
  if (ns == Duration::kNaT) return "NaT";
  const std::int64_t day = 86400LL * 1'000'000'000LL;
  const std::int64_t hour = 3600LL * 1'000'000'000LL;
  const std::int64_t min = 60LL * 1'000'000'000LL;
  const std::int64_t sec = 1'000'000'000LL;
  std::int64_t d = ns / day;
  std::int64_t remainder = ns % day;
  if (remainder < 0) {
    --d;
    remainder += day;
  }
  const auto h = remainder / hour; remainder %= hour;
  const auto m = remainder / min; remainder %= min;
  const auto s = remainder / sec; remainder %= sec;
  const auto n = remainder;
  char buf[128];
  std::snprintf(buf, sizeof(buf), "%lld days %02lld:%02lld:%02lld.%09lld",
                (long long)d, (long long)h, (long long)m, (long long)s,
                (long long)n);
  return std::string(buf);
}

static std::vector<std::int64_t> make_tokens(std::int64_t n,
                                             std::int64_t max_visible,
                                             std::int64_t edge_items) {
  std::vector<std::int64_t> out;
  if (n <= 0) {
    return out;
  }

  const std::int64_t edge = std::max<std::int64_t>(0, edge_items);
  if (max_visible <= 0 || n <= max_visible || n <= 2 * edge) {
    out.reserve(static_cast<std::size_t>(n));
    for (std::int64_t i = 0; i < n; ++i) {
      out.push_back(i);
    }
    return out;
  }

  const std::int64_t head = std::min<std::int64_t>(edge, n);
  const std::int64_t tail = std::min<std::int64_t>(edge, n - head);
  out.reserve(static_cast<std::size_t>(head + tail + 1));
  for (std::int64_t i = 0; i < head; ++i) {
    out.push_back(i);
  }
  out.push_back(-1);
  for (std::int64_t i = n - tail; i < n; ++i) {
    out.push_back(i);
  }
  return out;
}

static std::string format_shape(std::span<const std::int64_t> shape) {
  std::ostringstream oss;
  oss << "[";
  for (std::size_t i = 0; i < shape.size(); ++i) {
    if (i) oss << ", ";
    oss << shape[i];
  }
  oss << "]";
  return oss.str();
}

static std::vector<std::string> timeline_text_flat(const Timeline& t,
                                                   std::string_view fmt) {
  const auto note = detail::impl_strftime(t, fmt);
  std::vector<std::string> out;
  out.reserve(static_cast<std::size_t>(t.numel()));
  for (std::int64_t i = 0; i < t.numel(); ++i) {
    out.emplace_back(note.at(i));
  }
  return out;
}

static std::string render_matrix(const std::vector<std::string>& values,
                                 std::size_t rows,
                                 std::size_t cols,
                                 const TimelinePrintOptions& opt,
                                 std::size_t base_offset = 0,
                                 std::size_t plane_stride = 0) {
  const auto row_tokens = make_tokens(static_cast<std::int64_t>(rows), opt.max_rows, opt.edge_items);
  const auto col_tokens = make_tokens(static_cast<std::int64_t>(cols), opt.max_cols, opt.edge_items);

  std::vector<std::size_t> widths(col_tokens.size(), 3);
  for (std::size_t ri = 0; ri < row_tokens.size(); ++ri) {
    const auto r = row_tokens[ri];
    if (r < 0) continue;
    for (std::size_t ci = 0; ci < col_tokens.size(); ++ci) {
      const auto c = col_tokens[ci];
      if (c < 0) continue;
      const auto off = base_offset + static_cast<std::size_t>(r) * cols + static_cast<std::size_t>(c);
      const auto len = values[off + plane_stride].size();
      widths[ci] = std::max(widths[ci], len);
    }
  }

  std::ostringstream oss;
  for (std::size_t ri = 0; ri < row_tokens.size(); ++ri) {
    const auto r = row_tokens[ri];
    if (r < 0) {
      oss << "...";
    } else {
      const auto row_idx = static_cast<std::size_t>(r);
      oss << "[ ";
      for (std::size_t ci = 0; ci < col_tokens.size(); ++ci) {
        if (ci) oss << " ";
        const auto c = col_tokens[ci];
        const std::string cell =
            (c < 0) ? "..." : values[base_offset + row_idx * cols + static_cast<std::size_t>(c) + plane_stride];
        oss << std::setw(static_cast<int>(widths[ci])) << cell;
      }
      oss << " ]";
    }
    if (ri + 1 < row_tokens.size()) {
      oss << "\n";
    }
  }
  return oss.str();
}

static std::string render_values(const Timeline& t,
                                 const std::vector<std::string>& flat,
                                 const TimelinePrintOptions& opt) {
  const auto shape = t.sizes();
  const auto rank = shape.size();
  if (rank == 0) {
    return flat.empty() ? "NaT" : flat[0];
  }
  if (rank == 1) {
    const auto tokens = make_tokens(shape[0], opt.max_cols, opt.edge_items);
    std::ostringstream oss;
    oss << "[ ";
    for (std::size_t i = 0; i < tokens.size(); ++i) {
      if (i) oss << ", ";
      if (tokens[i] < 0) {
        oss << "...";
      } else {
        oss << flat[static_cast<std::size_t>(tokens[i])];
      }
    }
    oss << " ]";
    return oss.str();
  }
  if (rank == 2) {
    return render_matrix(flat, static_cast<std::size_t>(shape[0]), static_cast<std::size_t>(shape[1]), opt);
  }

  CODEM_CHECK(rank == 3, "print_timeline: Timeline supports rank up to 3");
  const std::size_t slices = static_cast<std::size_t>(shape[0]);
  const std::size_t rows = static_cast<std::size_t>(shape[1]);
  const std::size_t cols = static_cast<std::size_t>(shape[2]);
  const std::size_t plane = rows * cols;
  const auto slice_tokens =
      make_tokens(static_cast<std::int64_t>(slices), opt.max_slices, opt.edge_items);

  std::ostringstream oss;
  for (std::size_t si = 0; si < slice_tokens.size(); ++si) {
    const auto s = slice_tokens[si];
    if (s < 0) {
      oss << "...";
    } else {
      const auto slice = static_cast<std::size_t>(s);
      oss << "values[" << slice << ", :, :]\n";
      oss << render_matrix(flat, rows, cols, opt, 0, slice * plane);
    }
    if (si + 1 < slice_tokens.size()) {
      oss << "\n\n";
    }
  }
  return oss.str();
}

// Shape-aware rendering for Duration, mirroring render_values(const Timeline&, ...)
// above rank-for-rank (0D scalar, 1D flat list with edge truncation, 2D real
// row-per-line grid via render_matrix, 3D slice-of-matrices) -- reuses
// render_matrix/make_tokens directly since neither is Timeline-specific.
// TimelinePrintOptions is reused here too (it's just layout/truncation knobs,
// nothing timeline-semantic beyond the fields render_values(Timeline) itself
// doesn't touch), rather than introducing a near-duplicate options struct.
static std::string render_duration_values_(std::span<const std::int64_t> shape,
                                           const std::vector<std::string>& flat) {
  const TimelinePrintOptions opt{};
  const auto rank = shape.size();
  if (rank == 0) {
    return flat.empty() ? "NaT" : flat[0];
  }
  if (rank == 1) {
    const auto tokens = make_tokens(shape[0], opt.max_cols, opt.edge_items);
    std::ostringstream oss;
    oss << "[ ";
    for (std::size_t i = 0; i < tokens.size(); ++i) {
      if (i) oss << ", ";
      oss << (tokens[i] < 0 ? "..." : flat[static_cast<std::size_t>(tokens[i])]);
    }
    oss << " ]";
    return oss.str();
  }
  if (rank == 2) {
    return render_matrix(flat, static_cast<std::size_t>(shape[0]), static_cast<std::size_t>(shape[1]), opt);
  }

  CODEM_CHECK(rank == 3, "print_duration: Duration supports rank up to 3");
  const std::size_t slices = static_cast<std::size_t>(shape[0]);
  const std::size_t rows = static_cast<std::size_t>(shape[1]);
  const std::size_t cols = static_cast<std::size_t>(shape[2]);
  const std::size_t plane = rows * cols;
  const auto slice_tokens =
      make_tokens(static_cast<std::int64_t>(slices), opt.max_slices, opt.edge_items);

  std::ostringstream oss;
  for (std::size_t si = 0; si < slice_tokens.size(); ++si) {
    const auto s = slice_tokens[si];
    if (s < 0) {
      oss << "...";
    } else {
      const auto slice = static_cast<std::size_t>(s);
      oss << "values[" << slice << ", :, :]\n";
      oss << render_matrix(flat, rows, cols, opt, 0, slice * plane);
    }
    if (si + 1 < slice_tokens.size()) {
      oss << "\n\n";
    }
  }
  return oss.str();
}

} // namespace

std::string to_string(const Timeline& t, const TimelinePrintOptions& opt) {
  if (!t.defined()) {
    return "Timeline(undefined)";
  }

  std::ostringstream oss;
  if (opt.show_header) {
    oss << "Timeline(";
    bool wrote = false;
    if (opt.show_shape) {
      oss << "shape=" << format_shape(t.sizes());
      wrote = true;
    }
    if (opt.show_tz) {
      if (wrote) oss << ", ";
      oss << "tz=" << t.tz();
    }
    oss << ")";
    if (t.numel() > 0) {
      oss << "\n";
    }
  }

  const auto flat = timeline_text_flat(t, opt.format);
  oss << render_values(t, flat, opt);
  return oss.str();
}

std::string print_timeline(const Timeline& t, const TimelinePrintOptions& opt) {
  return to_string(t, opt);
}

std::ostream& print_timeline(std::ostream& os,
                             const Timeline& t,
                             const TimelinePrintOptions& opt) {
  os << print_timeline(t, opt);
  return os;
}

std::ostream& operator<<(std::ostream& os, const Timeline& t) {
  os << to_string(t);
  return os;
}

std::string to_string(const Duration& d) {
  if (!d.defined()) {
    return "Duration(undefined)";
  }

  std::ostringstream oss;
  oss << "Duration(shape=" << format_shape(d.sizes())
      << ", dim=" << d.dim() << ", numel=" << d.numel() << ")";
  if (d.numel() > 0) oss << "\n";

  const auto nanos_flat = detail::impl_to_nanos_flat(d);
  std::vector<std::string> flat;
  flat.reserve(nanos_flat.size());
  for (const auto ns : nanos_flat) flat.push_back(format_duration_one_(ns));

  oss << render_duration_values_(d.sizes(), flat);
  return oss.str();
}

std::string print_duration(const Duration& d) {
  return to_string(d);
}

std::ostream& print_duration(std::ostream& os, const Duration& d) {
  os << to_string(d);
  return os;
}

std::ostream& operator<<(std::ostream& os, const Duration& d) {
  os << to_string(d);
  return os;
}

std::string to_string(const TimelineIndexAccessor& idx, const TimelinePrintOptions& opt) {
  if (!idx.timeline().defined()) {
    return "TimelineIndexAccessor(undefined)";
  }
  std::string s = to_string(idx.timeline(), opt);
  // Reuse Timeline's rendering verbatim and expose only the public accessor
  // name; TimelineIndexAccessor is the private engine behind Timeline::index().
  constexpr std::string_view kFrom = "Timeline(";
  if (s.compare(0, kFrom.size(), kFrom) == 0) {
    s.replace(0, kFrom.size(), "TimelineIndexAccessor(");
  }
  return s;
}

std::string print_index(const TimelineIndexAccessor& idx,
                            const TimelinePrintOptions& opt) {
  return to_string(idx, opt);
}

std::ostream& print_index(std::ostream& os,
                             const TimelineIndexAccessor& idx,
                             const TimelinePrintOptions& opt) {
  os << to_string(idx, opt);
  return os;
}

std::ostream& operator<<(std::ostream& os, const TimelineIndexAccessor& idx) {
  os << to_string(idx, {});
  return os;
}

} // namespace pluss::codeh
