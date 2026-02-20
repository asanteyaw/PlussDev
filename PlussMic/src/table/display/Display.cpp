#include "table/display/Display.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <numeric>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <fmt/format.h>

#if !defined(_WIN32)
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#include "CodeM/core/Complex.h"
#include "CodeM/core/Half.h"
#include "CodeM/core/ScalarType.h"
#include "table/Atlas.h"
#include "table/DataFrame.h"
#include "table/Thread.h"

namespace pluss::table::display {

namespace {

std::string clip_cell(std::string s, std::int64_t maxw) {
  if (maxw <= 0) {
    return s;
  }
  const auto m = static_cast<std::size_t>(maxw);
  if (s.size() <= m) {
    return s;
  }
  if (m <= 1) {
    return s.substr(0, m);
  }
  return s.substr(0, m - 1) + "~";
}

std::string left_pad(const std::string& s, std::int64_t width) {
  if (static_cast<std::int64_t>(s.size()) >= width) {
    return s;
  }
  return std::string(static_cast<std::size_t>(width - static_cast<std::int64_t>(s.size())), ' ') + s;
}

std::string right_pad(const std::string& s, std::int64_t width) {
  if (static_cast<std::int64_t>(s.size()) >= width) {
    return s;
  }
  return s + std::string(static_cast<std::size_t>(width - static_cast<std::int64_t>(s.size())), ' ');
}

std::string fit_line(std::string line, std::int64_t width) {
  if (width <= 0) {
    return line;
  }
  if (static_cast<std::int64_t>(line.size()) <= width) {
    return line;
  }
  if (width <= 1) {
    return line.substr(0, static_cast<std::size_t>(width));
  }
  line.resize(static_cast<std::size_t>(width - 1));
  line.push_back('~');
  return line;
}

std::vector<std::int64_t> visible_indices(std::int64_t total, std::int64_t max_count, std::int64_t edge) {
  std::vector<std::int64_t> out;
  if (total <= 0) {
    return out;
  }
  if (max_count <= 0) {
    return out;
  }
  if (total <= max_count) {
    out.reserve(static_cast<std::size_t>(total));
    for (std::int64_t i = 0; i < total; ++i) out.push_back(i);
    return out;
  }

  const auto e = std::max<std::int64_t>(1, std::min<std::int64_t>(edge, max_count / 2));
  out.reserve(static_cast<std::size_t>(2 * e));
  for (std::int64_t i = 0; i < e; ++i) out.push_back(i);
  for (std::int64_t i = total - e; i < total; ++i) out.push_back(i);
  return out;
}

std::string scalar_at(const pluss::Series& s, std::int64_t row) {
  const auto st = s.scalar_type();
  const auto* base = s.raw_data();
  switch (st) {
    case pluss::codem::ScalarType::Bool:
      return reinterpret_cast<const bool*>(base)[row] ? "true" : "false";
    case pluss::codem::ScalarType::Int8:
      return fmt::format("{}", static_cast<int>(reinterpret_cast<const std::int8_t*>(base)[row]));
    case pluss::codem::ScalarType::Int16:
      return fmt::format("{}", reinterpret_cast<const std::int16_t*>(base)[row]);
    case pluss::codem::ScalarType::Int32:
      return fmt::format("{}", reinterpret_cast<const std::int32_t*>(base)[row]);
    case pluss::codem::ScalarType::Int64:
      return fmt::format("{}", reinterpret_cast<const std::int64_t*>(base)[row]);
    case pluss::codem::ScalarType::UInt8:
      return fmt::format("{}", static_cast<unsigned>(reinterpret_cast<const std::uint8_t*>(base)[row]));
    case pluss::codem::ScalarType::UInt16:
      return fmt::format("{}", reinterpret_cast<const std::uint16_t*>(base)[row]);
    case pluss::codem::ScalarType::UInt32:
      return fmt::format("{}", reinterpret_cast<const std::uint32_t*>(base)[row]);
    case pluss::codem::ScalarType::UInt64:
      return fmt::format("{}", reinterpret_cast<const std::uint64_t*>(base)[row]);
    case pluss::codem::ScalarType::Float16:
      return fmt::format("{:.6g}", pluss::codem::float16_to_float(reinterpret_cast<const pluss::codem::Float16*>(base)[row]));
    case pluss::codem::ScalarType::BFloat16:
      return fmt::format("{:.6g}", pluss::codem::bfloat16_to_float(reinterpret_cast<const pluss::codem::BFloat16*>(base)[row]));
    case pluss::codem::ScalarType::Float32:
      return fmt::format("{:.8g}", reinterpret_cast<const float*>(base)[row]);
    case pluss::codem::ScalarType::Float64:
      return fmt::format("{:.10g}", reinterpret_cast<const double*>(base)[row]);
    case pluss::codem::ScalarType::Complex64: {
      const auto v = reinterpret_cast<const pluss::codem::Complex64*>(base)[row];
      return fmt::format("({:.6g}{:+.6g}j)", v.real(), v.imag());
    }
    case pluss::codem::ScalarType::Complex128: {
      const auto v = reinterpret_cast<const pluss::codem::Complex128*>(base)[row];
      return fmt::format("({:.6g}{:+.6g}j)", v.real(), v.imag());
    }
    default:
      return "<scalar?>";
  }
}

bool is_numeric_thread(const Thread& t) {
  if (t.kind() != ThreadKind::Series) {
    return false;
  }
  const auto dt = t.as_series().dtype();
  return dt.is_floating() || dt.is_integral() || dt.scalar == pluss::codem::ScalarType::Bool;
}

std::string thread_value_at(const Thread& t, std::int64_t row) {
  if (t.has_validity_mask()) {
    const auto valid = t.validity_mask()->to_vec<bool>();
    if (!valid[static_cast<std::size_t>(row)]) {
      return "NA";
    }
  }

  switch (t.kind()) {
    case ThreadKind::Series:
      return scalar_at(t.as_series(), row);
    case ThreadKind::Note:
      return std::string(t.as_note().at(row));
    case ThreadKind::Timeline:
      return fmt::format("{}", t.as_timeline().to_epoch_nanos_vector()[static_cast<std::size_t>(row)]);
    case ThreadKind::TimeIndex:
      return fmt::format("{}", t.as_timeindex().timeline().to_epoch_nanos_vector()[static_cast<std::size_t>(row)]);
    default:
      return "<?>";
  }
}

std::int64_t non_null_count(const Thread& t) {
  if (!t.has_validity_mask()) {
    return t.size();
  }
  const auto valid = t.validity_mask()->to_vec<bool>();
  return static_cast<std::int64_t>(std::count(valid.begin(), valid.end(), true));
}

std::uint64_t estimate_bytes(const Thread& t) {
  std::uint64_t bytes = 0;
  if (t.kind() == ThreadKind::Series) {
    const auto item = t.as_series().dtype().itemsize;
    bytes += static_cast<std::uint64_t>(item) * static_cast<std::uint64_t>(t.size());
  } else if (t.kind() == ThreadKind::Timeline || t.kind() == ThreadKind::TimeIndex) {
    bytes += static_cast<std::uint64_t>(8) * static_cast<std::uint64_t>(t.size());
  } else if (t.kind() == ThreadKind::Note) {
    const auto& n = t.as_note();
    for (std::int64_t i = 0; i < t.size(); ++i) {
      bytes += static_cast<std::uint64_t>(n.at(i).size());
    }
  }
  if (t.has_validity_mask()) {
    bytes += static_cast<std::uint64_t>(t.size());
  }
  return bytes;
}

std::string human_bytes(std::uint64_t b) {
  static constexpr std::array<const char*, 5> units{"B", "KB", "MB", "GB", "TB"};
  double v = static_cast<double>(b);
  std::size_t u = 0;
  while (v >= 1024.0 && u + 1 < units.size()) {
    v /= 1024.0;
    ++u;
  }
  return fmt::format("{:.2f} {}", v, units[u]);
}

struct VisibleLayout {
  std::vector<std::int64_t> rows;
  std::vector<std::int64_t> cols;
  bool row_ellipsis{false};
  bool col_ellipsis{false};
};

VisibleLayout compute_layout(const DataFrame& frame, const DisplayOptions& options) {
  VisibleLayout l;
  l.rows = visible_indices(frame.nrows(), options.max_rows, options.edge_rows);
  l.cols = visible_indices(frame.ncols(), options.max_columns, options.edge_columns);
  l.row_ellipsis = static_cast<std::int64_t>(l.rows.size()) < frame.nrows();
  l.col_ellipsis = static_cast<std::int64_t>(l.cols.size()) < frame.ncols();
  return l;
}

} // namespace

TerminalInfo detect_terminal() {
  TerminalInfo out;
#if !defined(_WIN32)
  if (::isatty(STDOUT_FILENO)) {
    out.is_tty = true;
    winsize ws{};
    if (::ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
      if (ws.ws_col > 0) out.columns = ws.ws_col;
      if (ws.ws_row > 0) out.rows = ws.ws_row;
    }
  }
#endif
  if (const char* c = std::getenv("COLUMNS")) {
    const auto n = std::atoll(c);
    if (n > 0) out.columns = n;
  }
  if (const char* r = std::getenv("LINES")) {
    const auto n = std::atoll(r);
    if (n > 0) out.rows = n;
  }
  return out;
}

std::string render(const DataFrame& frame, const DisplayOptions& options) {
  const auto term = detect_terminal();
  const auto terminal_width = options.terminal_width > 0 ? options.terminal_width : term.columns;

  auto layout = compute_layout(frame, options);
  const auto names = frame.column_names();
  const auto cols = frame.columns();

  std::vector<std::string> headers;
  std::vector<std::int64_t> widths;
  std::vector<bool> right_align_col;

  if (options.show_case_index) {
    headers.push_back("case");
    widths.push_back(std::max<std::int64_t>(options.min_column_width, 4));
    right_align_col.push_back(true);
  }

  for (const auto cidx : layout.cols) {
    const auto h = clip_cell(names[static_cast<std::size_t>(cidx)], options.max_cell_width);
    headers.push_back(h);
    widths.push_back(std::max<std::int64_t>(options.min_column_width, static_cast<std::int64_t>(h.size())));
    right_align_col.push_back(is_numeric_thread(cols[static_cast<std::size_t>(cidx)]));
  }

  if (layout.col_ellipsis) {
    headers.push_back("...");
    widths.push_back(3);
    right_align_col.push_back(false);
  }

  struct RowCells {
    std::vector<std::string> cells;
    bool ellipsis{false};
  };

  std::vector<RowCells> rows;
  rows.reserve(layout.rows.size() + (layout.row_ellipsis ? 1 : 0));

  for (std::size_t ridx = 0; ridx < layout.rows.size(); ++ridx) {
    const auto row = layout.rows[ridx];
    if (layout.row_ellipsis && ridx == layout.rows.size() / 2) {
      rows.push_back({{}, true});
    }

    RowCells r;
    if (options.show_case_index) {
      r.cells.push_back(clip_cell(thread_value_at(frame.case_index(), row), options.max_cell_width));
    }
    for (const auto cidx : layout.cols) {
      const auto cell = clip_cell(thread_value_at(cols[static_cast<std::size_t>(cidx)], row), options.max_cell_width);
      r.cells.push_back(cell);
    }
    if (layout.col_ellipsis) {
      r.cells.push_back("...");
    }

    for (std::size_t c = 0; c < r.cells.size(); ++c) {
      widths[c] = std::max<std::int64_t>(widths[c], static_cast<std::int64_t>(r.cells[c].size()));
    }
    rows.push_back(std::move(r));
  }

  for (auto& w : widths) {
    w = std::clamp<std::int64_t>(w, options.min_column_width, options.max_cell_width);
  }

  std::vector<std::string> lines;
  lines.reserve(rows.size() + 4);

  std::string head_line;
  for (std::size_t c = 0; c < headers.size(); ++c) {
    if (c != 0) head_line += "  ";
    head_line += right_pad(clip_cell(headers[c], widths[c]), widths[c]);
  }
  lines.push_back(fit_line(head_line, terminal_width));

  for (const auto& r : rows) {
    if (r.ellipsis) {
      lines.push_back(fit_line("...", terminal_width));
      continue;
    }

    std::string line;
    for (std::size_t c = 0; c < r.cells.size(); ++c) {
      if (c != 0) line += "  ";
      const bool right_align = right_align_col[c];
      const auto clipped = clip_cell(r.cells[c], widths[c]);
      line += right_align ? left_pad(clipped, widths[c]) : right_pad(clipped, widths[c]);
    }
    lines.push_back(fit_line(line, terminal_width));
  }

  if (options.show_shape_footer) {
    lines.push_back(fmt::format("[{} cases x {} columns]", frame.nrows(), frame.ncols()));
  }

  std::string out;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    if (i) out.push_back('\n');
    out += lines[i];
  }
  return out;
}

std::string render(const Thread& thread, const DisplayOptions& options) {
  DataFrame tmp({{"value", thread}});
  return render(tmp, options);
}

std::string render_info(const DataFrame& frame, const DisplayOptions& options) {
  (void)options;
  const auto cols = frame.columns();
  const auto names = frame.column_names();

  std::unordered_map<std::string, std::int64_t> dtype_counts;
  std::uint64_t memory_bytes = estimate_bytes(frame.case_index());

  std::vector<std::string> lines;
  lines.reserve(static_cast<std::size_t>(frame.ncols()) + 8);

  const auto first_case = frame.nrows() > 0 ? thread_value_at(frame.case_index(), 0) : std::string("NA");
  const auto last_case = frame.nrows() > 0 ? thread_value_at(frame.case_index(), frame.nrows() - 1) : std::string("NA");

  lines.push_back("<class 'pluss::table::DataFrame'>");
  lines.push_back(fmt::format("CaseIndex: {} cases, {} to {}", frame.nrows(), first_case, last_case));
  lines.push_back(fmt::format("Data columns (total {} columns):", frame.ncols()));
  lines.push_back(" #  Column                     Non-Null Cases  Dtype");
  lines.push_back("--- -------------------------- --------------  ----------------");

  for (std::size_t i = 0; i < names.size(); ++i) {
    const auto nn = non_null_count(cols[i]);
    const auto dtype = cols[i].dtype_name();
    dtype_counts[dtype] += 1;
    memory_bytes += estimate_bytes(cols[i]);
    lines.push_back(fmt::format("{:>2}  {:<26} {:>14}  {}", i, clip_cell(names[i], 26), nn, dtype));
  }

  std::vector<std::string> dtype_chunks;
  dtype_chunks.reserve(dtype_counts.size());
  for (const auto& kv : dtype_counts) {
    dtype_chunks.push_back(fmt::format("{}({})", kv.first, kv.second));
  }
  std::sort(dtype_chunks.begin(), dtype_chunks.end());

  std::string dtypes_line;
  for (std::size_t i = 0; i < dtype_chunks.size(); ++i) {
    if (i) dtypes_line += ", ";
    dtypes_line += dtype_chunks[i];
  }

  lines.push_back(fmt::format("dtypes: {}", dtypes_line));
  lines.push_back(fmt::format("memory usage: {}", human_bytes(memory_bytes)));

  std::string out;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    if (i) out.push_back('\n');
    out += lines[i];
  }
  return out;
}

std::string render(const Atlas& atlas, const DisplayOptions& options) {
  std::vector<std::string> lines;
  lines.push_back(fmt::format("Atlas(depth={}, rows={}, cols={})", atlas.depth(), atlas.nrows(), atlas.ncols()));

  const auto visible = visible_indices(atlas.depth(), std::max<std::int64_t>(2, options.edge_rows * 2), options.edge_rows);
  const bool ellipsis = static_cast<std::int64_t>(visible.size()) < atlas.depth();

  DisplayOptions frame_opts = options;
  frame_opts.max_rows = std::max<std::int64_t>(5, std::min<std::int64_t>(options.max_rows, 12));

  for (std::size_t i = 0; i < visible.size(); ++i) {
    if (ellipsis && i == visible.size() / 2) {
      lines.push_back("...");
    }

    const auto idx = visible[i];
    std::string label = fmt::format("frame[{}]", idx);
    if (atlas.axis0_labels().has_value()) {
      label += fmt::format(" case={}", thread_value_at(*atlas.axis0_labels(), idx));
    }
    lines.push_back(label);
    lines.push_back(render(atlas.frame(idx), frame_opts));
  }

  std::string out;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    if (i) out.push_back('\n');
    out += lines[i];
  }
  return out;
}

void print(std::ostream& os, const Thread& thread, const DisplayOptions& options) {
  os << render(thread, options);
}

void print(std::ostream& os, const DataFrame& frame, const DisplayOptions& options) {
  os << render(frame, options);
}

void print(std::ostream& os, const Atlas& atlas, const DisplayOptions& options) {
  os << render(atlas, options);
}

void print_info(std::ostream& os, const DataFrame& frame, const DisplayOptions& options) {
  os << render_info(frame, options);
}

} // namespace pluss::table::display
