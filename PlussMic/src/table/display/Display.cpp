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

struct RowPlan {
  std::vector<std::int64_t> head;
  std::vector<std::int64_t> tail;
  bool ellipsis{false};
};

struct ColPlan {
  std::vector<std::int64_t> head;
  std::vector<std::int64_t> tail;
  bool ellipsis{false};
};

struct TableMatrix {
  std::vector<std::string> headers;
  std::vector<bool> right_align;
  std::vector<std::int64_t> min_widths;
  std::vector<std::vector<std::string>> rows;
};

struct ResponsivePreset {
  std::int64_t edge_rows;
  std::int64_t tail_rows;
  std::int64_t edge_columns;
  std::int64_t tail_columns;
  std::int64_t min_column_width;
  std::int64_t max_cell_width;
  std::int64_t max_rows_cap;
};

struct EffectiveDisplayOptions {
  std::int64_t max_rows{15};
  std::int64_t max_columns{0};
  std::int64_t edge_rows{5};
  std::int64_t tail_rows{3};
  std::int64_t edge_columns{3};
  std::int64_t tail_columns{1};
  std::int64_t max_cell_width{28};
  std::int64_t min_column_width{4};
  std::int64_t terminal_width{120};
  std::int64_t terminal_height{40};
  bool show_shape_footer{true};
  bool show_case_index{true};
  bool color{false};
};

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
  if (width <= 0 || static_cast<std::int64_t>(line.size()) <= width) {
    return line;
  }
  if (width <= 1) {
    return line.substr(0, static_cast<std::size_t>(width));
  }
  line.resize(static_cast<std::size_t>(width - 1));
  line.push_back('~');
  return line;
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

ResponsivePreset preset_for_terminal_width(std::int64_t width, DisplayMode mode) {
  ResponsivePreset p{};
  if (width < 80) {
    p = ResponsivePreset{3, 2, 1, 1, 3, 12, 10};
  } else if (width < 110) {
    p = ResponsivePreset{4, 2, 2, 1, 4, 16, 12};
  } else if (width < 140) {
    p = ResponsivePreset{5, 3, 3, 1, 4, 22, 15};
  } else if (width < 180) {
    p = ResponsivePreset{6, 3, 4, 1, 5, 26, 20};
  } else {
    p = ResponsivePreset{8, 4, 5, 2, 6, 32, 26};
  }

  if (mode == DisplayMode::Compact) {
    p.edge_rows = std::max<std::int64_t>(2, p.edge_rows - 1);
    p.tail_rows = std::max<std::int64_t>(1, p.tail_rows - 1);
    p.edge_columns = std::max<std::int64_t>(1, p.edge_columns - 1);
    p.tail_columns = 1;
    p.max_cell_width = std::max<std::int64_t>(10, p.max_cell_width - 4);
    p.max_rows_cap = std::max<std::int64_t>(8, p.max_rows_cap - 4);
  } else if (mode == DisplayMode::Wide) {
    p.edge_rows += 1;
    p.tail_rows += 1;
    p.edge_columns += 1;
    p.tail_columns = std::max<std::int64_t>(2, p.tail_columns);
    p.max_cell_width += 4;
    p.max_rows_cap += 4;
  }
  return p;
}

EffectiveDisplayOptions resolve_effective_options(
    const DisplayOptions& input,
    const TerminalInfo& term,
    bool info_mode) {
  EffectiveDisplayOptions out;

  const auto width = input.terminal_width > 0 ? input.terminal_width : std::max<std::int64_t>(40, term.columns);
  const auto height = input.terminal_height > 0 ? input.terminal_height : std::max<std::int64_t>(20, term.rows);
  const auto preset = preset_for_terminal_width(width, input.mode);

  out.terminal_width = width;
  out.terminal_height = height;
  out.edge_rows = preset.edge_rows;
  out.tail_rows = preset.tail_rows;
  out.edge_columns = preset.edge_columns;
  out.tail_columns = preset.tail_columns;
  out.min_column_width = preset.min_column_width;
  out.max_cell_width = preset.max_cell_width;
  out.max_columns = 0;
  out.show_shape_footer = input.show_shape_footer;
  out.show_case_index = input.show_case_index;
  out.color = input.color;

  const auto budget = info_mode ? std::max<std::int64_t>(8, height - 18) : std::max<std::int64_t>(6, height - 10);
  const auto min_rows = std::max<std::int64_t>(2, out.edge_rows + out.tail_rows + 1);
  const auto auto_max_rows = std::clamp<std::int64_t>(budget, min_rows, preset.max_rows_cap);
  out.max_rows = auto_max_rows;

  if (info_mode) {
    // Keep info metadata readable on medium/large terminals.
    out.max_cell_width = std::max<std::int64_t>(out.max_cell_width, 28);
  }

  if (out.max_cell_width < out.min_column_width) {
    out.max_cell_width = out.min_column_width;
  }
  return out;
}

RowPlan build_row_plan(std::int64_t total_rows, const EffectiveDisplayOptions& options) {
  RowPlan p;
  if (total_rows <= 0) {
    return p;
  }

  if (options.max_rows <= 0 || total_rows <= options.max_rows) {
    p.head.reserve(static_cast<std::size_t>(total_rows));
    for (std::int64_t i = 0; i < total_rows; ++i) {
      p.head.push_back(i);
    }
    return p;
  }

  std::int64_t head = std::max<std::int64_t>(1, options.edge_rows);
  std::int64_t tail = std::max<std::int64_t>(1, options.tail_rows);
  head = std::min<std::int64_t>(head, total_rows);
  tail = std::min<std::int64_t>(tail, total_rows - head);

  while (head + tail + 1 > options.max_rows) {
    if (tail > 1) {
      --tail;
    } else if (head > 1) {
      --head;
    } else {
      break;
    }
  }

  if (head + tail >= total_rows) {
    p.head.reserve(static_cast<std::size_t>(total_rows));
    for (std::int64_t i = 0; i < total_rows; ++i) {
      p.head.push_back(i);
    }
    return p;
  }

  p.head.reserve(static_cast<std::size_t>(head));
  p.tail.reserve(static_cast<std::size_t>(tail));
  for (std::int64_t i = 0; i < head; ++i) {
    p.head.push_back(i);
  }
  for (std::int64_t i = total_rows - tail; i < total_rows; ++i) {
    p.tail.push_back(i);
  }
  p.ellipsis = true;
  return p;
}

ColPlan build_col_plan(std::int64_t total_cols, const EffectiveDisplayOptions& options, bool force_compact) {
  ColPlan p;
  if (total_cols <= 0) {
    return p;
  }

  const bool capped = options.max_columns > 0 && total_cols > options.max_columns;
  const bool compact = force_compact || capped;

  if (!compact) {
    p.head.reserve(static_cast<std::size_t>(total_cols));
    for (std::int64_t i = 0; i < total_cols; ++i) {
      p.head.push_back(i);
    }
    return p;
  }

  const std::int64_t head_count = std::max<std::int64_t>(1, options.edge_columns);
  const std::int64_t tail_count = std::max<std::int64_t>(1, options.tail_columns);

  const auto h = std::min<std::int64_t>(head_count, total_cols);
  const auto t = std::min<std::int64_t>(tail_count, total_cols - h);

  if (h + t >= total_cols) {
    p.head.reserve(static_cast<std::size_t>(total_cols));
    for (std::int64_t i = 0; i < total_cols; ++i) {
      p.head.push_back(i);
    }
    return p;
  }

  p.head.reserve(static_cast<std::size_t>(h));
  p.tail.reserve(static_cast<std::size_t>(t));
  for (std::int64_t i = 0; i < h; ++i) {
    p.head.push_back(i);
  }
  for (std::int64_t i = total_cols - t; i < total_cols; ++i) {
    p.tail.push_back(i);
  }
  p.ellipsis = true;
  return p;
}

std::int64_t table_line_width(const std::vector<std::int64_t>& widths) {
  if (widths.empty()) {
    return 0;
  }
  std::int64_t out = widths[0];
  if (widths.size() >= 2) {
    out += 3 + widths[1]; // " | " between index-like col and first real col
    for (std::size_t i = 2; i < widths.size(); ++i) {
      out += 2 + widths[i]; // "  " between real cols
    }
  }
  return out;
}

std::vector<std::int64_t> compute_widths(
    const TableMatrix& matrix,
    const EffectiveDisplayOptions& options) {
  std::vector<std::int64_t> widths(matrix.headers.size(), options.min_column_width);
  for (std::size_t i = 0; i < matrix.headers.size(); ++i) {
    widths[i] = std::max<std::int64_t>(matrix.min_widths[i],
                                       std::min<std::int64_t>(options.max_cell_width,
                                                              static_cast<std::int64_t>(matrix.headers[i].size())));
  }

  for (const auto& row : matrix.rows) {
    for (std::size_t i = 0; i < row.size() && i < widths.size(); ++i) {
      const auto cell_len = static_cast<std::int64_t>(row[i].size());
      widths[i] = std::max<std::int64_t>(widths[i], std::min<std::int64_t>(options.max_cell_width, cell_len));
    }
  }
  return widths;
}

void shrink_widths_to_fit(
    std::vector<std::int64_t>& widths,
    const std::vector<std::int64_t>& min_widths,
    std::int64_t terminal_width) {
  if (terminal_width <= 0 || widths.empty()) {
    return;
  }

  while (table_line_width(widths) > terminal_width) {
    std::size_t best = widths.size();
    for (std::size_t i = 0; i < widths.size(); ++i) {
      if (widths[i] > min_widths[i]) {
        if (best == widths.size() || widths[i] > widths[best]) {
          best = i;
        }
      }
    }
    if (best == widths.size()) {
      break;
    }
    --widths[best];
  }
}

std::string row_line(
    const std::vector<std::string>& cells,
    const std::vector<std::int64_t>& widths,
    const std::vector<bool>& right_align) {
  std::string line;
  line.reserve(static_cast<std::size_t>(table_line_width(widths)) + 8);
  if (widths.empty()) {
    return line;
  }

  const auto first_raw = cells.empty() ? std::string{} : cells[0];
  const auto first = clip_cell(first_raw, widths[0]);
  line += right_align[0] ? left_pad(first, widths[0]) : right_pad(first, widths[0]);

  if (widths.size() >= 2) {
    const auto second_raw = cells.size() > 1 ? cells[1] : std::string{};
    const auto second = clip_cell(second_raw, widths[1]);
    line += " | ";
    line += right_align[1] ? left_pad(second, widths[1]) : right_pad(second, widths[1]);

    for (std::size_t i = 2; i < widths.size(); ++i) {
      const auto raw = i < cells.size() ? cells[i] : std::string{};
      const auto clipped = clip_cell(raw, widths[i]);
      line += "  ";
      line += right_align[i] ? left_pad(clipped, widths[i]) : right_pad(clipped, widths[i]);
    }
  }
  return line;
}

std::string render_ascii_table(
    const TableMatrix& matrix,
    const EffectiveDisplayOptions& options,
    std::int64_t terminal_width) {
  if (matrix.headers.empty()) {
    return "<empty>";
  }

  auto widths = compute_widths(matrix, options);
  shrink_widths_to_fit(widths, matrix.min_widths, terminal_width);

  const auto line_w = table_line_width(widths);
  const std::string top = std::string(static_cast<std::size_t>(line_w), '=');
  const std::string mid = std::string(static_cast<std::size_t>(line_w), '-');

  std::vector<std::string> lines;
  lines.reserve(matrix.rows.size() + 6);
  lines.push_back(fit_line(top, terminal_width));
  lines.push_back(fit_line(row_line(matrix.headers, widths, matrix.right_align), terminal_width));
  lines.push_back(fit_line(mid, terminal_width));

  for (const auto& r : matrix.rows) {
    lines.push_back(fit_line(row_line(r, widths, matrix.right_align), terminal_width));
  }

  lines.push_back(fit_line(mid, terminal_width));

  std::string out;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    if (i) {
      out.push_back('\n');
    }
    out += lines[i];
  }
  return out;
}

TableMatrix build_dataframe_matrix(
    const DataFrame& frame,
    const EffectiveDisplayOptions& options,
    bool compact_columns) {
  const auto row_plan = build_row_plan(frame.nrows(), options);
  const auto col_plan = build_col_plan(frame.ncols(), options, compact_columns);

  const auto names = frame.column_names();
  const auto cols = frame.columns();

  TableMatrix m;
  if (options.show_case_index) {
    m.headers.push_back("");
    m.right_align.push_back(false);
    m.min_widths.push_back(std::max<std::int64_t>(4, options.min_column_width));
  }

  auto add_real_col = [&](std::int64_t cidx) {
    const auto header = clip_cell(names[static_cast<std::size_t>(cidx)], options.max_cell_width);
    m.headers.push_back(header);
    m.right_align.push_back(false);
    m.min_widths.push_back(options.min_column_width);
  };

  for (const auto cidx : col_plan.head) {
    add_real_col(cidx);
  }
  if (col_plan.ellipsis) {
    m.headers.push_back("...");
    m.right_align.push_back(false);
    m.min_widths.push_back(3);
  }
  for (const auto cidx : col_plan.tail) {
    add_real_col(cidx);
  }

  auto build_data_row = [&](std::int64_t ridx) {
    std::vector<std::string> row;
    row.reserve(m.headers.size());

    if (options.show_case_index) {
      row.push_back(clip_cell(thread_value_at(frame.cases(), ridx), options.max_cell_width));
    }

    for (const auto cidx : col_plan.head) {
      row.push_back(clip_cell(thread_value_at(cols[static_cast<std::size_t>(cidx)], ridx), options.max_cell_width));
    }
    if (col_plan.ellipsis) {
      row.push_back("...");
    }
    for (const auto cidx : col_plan.tail) {
      row.push_back(clip_cell(thread_value_at(cols[static_cast<std::size_t>(cidx)], ridx), options.max_cell_width));
    }
    return row;
  };

  m.rows.reserve(row_plan.head.size() + row_plan.tail.size() + (row_plan.ellipsis ? 1 : 0));
  for (const auto ridx : row_plan.head) {
    m.rows.push_back(build_data_row(ridx));
  }
  if (row_plan.ellipsis) {
    m.rows.push_back(std::vector<std::string>(m.headers.size(), "..."));
  }
  for (const auto ridx : row_plan.tail) {
    m.rows.push_back(build_data_row(ridx));
  }

  return m;
}

std::int64_t estimate_matrix_width(const TableMatrix& matrix, const EffectiveDisplayOptions& options) {
  const auto widths = compute_widths(matrix, options);
  return table_line_width(widths);
}

std::string dtype_summary_line(const std::unordered_map<std::string, std::int64_t>& dtype_counts) {
  std::vector<std::string> parts;
  parts.reserve(dtype_counts.size());
  for (const auto& kv : dtype_counts) {
    parts.push_back(fmt::format("{}({})", kv.first, kv.second));
  }
  std::sort(parts.begin(), parts.end());

  std::string line;
  for (std::size_t i = 0; i < parts.size(); ++i) {
    if (i) {
      line += ", ";
    }
    line += parts[i];
  }
  return line;
}

std::string logical_dtype_name(const Thread& t) {
  switch (t.kind()) {
    case ThreadKind::Series:
      return "Series";
    case ThreadKind::Note:
      return "Note";
    case ThreadKind::Timeline:
      return "Timeline";
    case ThreadKind::TimeIndex:
      return "TimeIndex";
    default:
      return "Unknown";
  }
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
  const auto effective = resolve_effective_options(options, term, false);
  const auto terminal_width = effective.terminal_width > 0 ? effective.terminal_width : term.columns;

  auto working = effective;
  bool compact = false;
  auto matrix = build_dataframe_matrix(frame, working, compact);

  while (estimate_matrix_width(matrix, working) > terminal_width) {
    if (!compact && frame.ncols() > 1) {
      compact = true;
      matrix = build_dataframe_matrix(frame, working, compact);
      continue;
    }
    if (working.max_cell_width > std::max<std::int64_t>(4, working.min_column_width)) {
      --working.max_cell_width;
      matrix = build_dataframe_matrix(frame, working, compact);
      continue;
    }
    if (working.edge_columns > 1) {
      --working.edge_columns;
      compact = true;
      matrix = build_dataframe_matrix(frame, working, compact);
      continue;
    }
    if (working.tail_columns > 1) {
      --working.tail_columns;
      compact = true;
      matrix = build_dataframe_matrix(frame, working, compact);
      continue;
    }
    if (working.min_column_width > 2) {
      --working.min_column_width;
      matrix = build_dataframe_matrix(frame, working, compact);
      continue;
    }
    break;
  }

  std::string out = render_ascii_table(matrix, working, terminal_width);
  if (working.show_shape_footer) {
    out += fmt::format("\n[{} cs x {} feat]", frame.nrows(), frame.ncols());
  }
  return out;
}

std::string render(const Thread& thread, const DisplayOptions& options) {
  DataFrame tmp({{"value", thread}});
  return render(tmp, options);
}

std::string render_info(const DataFrame& frame, const DisplayOptions& options) {
  const auto term = detect_terminal();
  const auto effective = resolve_effective_options(options, term, true);
  const auto terminal_width = effective.terminal_width > 0 ? effective.terminal_width : term.columns;

  const auto cols = frame.columns();
  const auto names = frame.column_names();

  std::unordered_map<std::string, std::int64_t> dtype_counts;
  std::uint64_t memory_bytes = estimate_bytes(frame.cases());

  for (std::size_t i = 0; i < names.size(); ++i) {
    const auto dtype = logical_dtype_name(cols[i]);
    dtype_counts[dtype] += 1;
    memory_bytes += estimate_bytes(cols[i]);
  }

  const auto first_case = frame.nrows() > 0 ? thread_value_at(frame.cases(), 0) : std::string("NA");
  const auto last_case = frame.nrows() > 0 ? thread_value_at(frame.cases(), frame.nrows() - 1) : std::string("NA");

  TableMatrix summary;
  summary.headers = {"Field", "Value"};
  summary.right_align = {false, false};
  summary.min_widths = {5, 8};
  summary.rows = {
      {"Class", "pluss::table::DataFrame"},
      {"Cases", fmt::format("{}", frame.nrows())},
      {"Features", fmt::format("{}", frame.ncols())},
      {"Case Range", fmt::format("{} -> {}", first_case, last_case)},
      {"Null Case Rows", fmt::format("{}", frame.count_null_rows())},
      {"Dtypes", dtype_summary_line(dtype_counts)},
      {"Memory", human_bytes(memory_bytes)},
  };

  TableMatrix details;
  details.headers = {"#", "Feature", "Non-Null Cases", "Dtype"};
  details.right_align = {false, false, false, false};
  details.min_widths = {2, 6, 6, 5};
  const auto detail_plan = build_row_plan(static_cast<std::int64_t>(names.size()), effective);
  details.rows.reserve(detail_plan.head.size() + detail_plan.tail.size() + (detail_plan.ellipsis ? 1 : 0));

  auto push_detail_row = [&](std::size_t i) {
    const auto nn = non_null_count(cols[i]);
    details.rows.push_back({
        fmt::format("{}", i),
        names[i],
        fmt::format("{}", nn),
        logical_dtype_name(cols[i]),
    });
  };

  for (const auto i : detail_plan.head) {
    push_detail_row(static_cast<std::size_t>(i));
  }
  if (detail_plan.ellipsis) {
    details.rows.push_back({"...", "...", "...", "..."});
  }
  for (const auto i : detail_plan.tail) {
    push_detail_row(static_cast<std::size_t>(i));
  }

  std::string out;
  out += render_ascii_table(summary, effective, terminal_width);
  out += "\n";
  out += render_ascii_table(details, effective, terminal_width);
  return out;
}

std::string render(const Atlas& atlas, const DisplayOptions& options) {
  const auto term = detect_terminal();
  const auto effective = resolve_effective_options(options, term, false);
  std::vector<std::string> lines;
  lines.push_back(fmt::format("Atlas(depth={}, rows={}, cols={})", atlas.depth(), atlas.nrows(), atlas.ncols()));

  const auto total = atlas.depth();
  const auto max_frames = std::max<std::int64_t>(2, effective.edge_rows + effective.tail_rows);
  std::vector<std::int64_t> visible;
  bool ellipsis = false;
  if (total <= max_frames) {
    visible.reserve(static_cast<std::size_t>(total));
    for (std::int64_t i = 0; i < total; ++i) {
      visible.push_back(i);
    }
  } else {
    const auto head = std::max<std::int64_t>(1, effective.edge_rows);
    const auto tail = std::max<std::int64_t>(1, effective.tail_rows);
    for (std::int64_t i = 0; i < head; ++i) {
      visible.push_back(i);
    }
    for (std::int64_t i = total - tail; i < total; ++i) {
      visible.push_back(i);
    }
    ellipsis = true;
  }

  DisplayOptions frame_opts = options;
  if (frame_opts.mode == DisplayMode::Wide) {
    frame_opts.mode = DisplayMode::Auto;
  }

  for (std::size_t i = 0; i < visible.size(); ++i) {
    if (ellipsis && i == static_cast<std::size_t>(effective.edge_rows)) {
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
    if (i) {
      out.push_back('\n');
    }
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
