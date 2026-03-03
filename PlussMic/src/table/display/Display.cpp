#include "table/display/Display.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <memory>
#include <numeric>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <fmt/format.h>

#if !defined(_WIN32)
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#if defined(PLUSSMIC_HAVE_GTK_WEBKIT) && PLUSSMIC_HAVE_GTK_WEBKIT
#include <cairo.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#endif

#include "CodeM/core/Complex.h"
#include "CodeM/core/Half.h"
#include "CodeM/core/ScalarType.h"
#include "table/Atlas.h"
#include "table/DataFrame.h"
#include "table/Thread.h"
#include "table/display/PlatformHtmlViewer.h"

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

std::atomic<DisplayBackend> g_default_backend{DisplayBackend::Auto};

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

  auto has_negative_visible_value = [&](std::int64_t cidx) {
    const auto& col = cols[static_cast<std::size_t>(cidx)];
    if (!is_numeric_thread(col)) {
      return false;
    }

    auto row_is_negative = [&](std::int64_t ridx) {
      const auto v = thread_value_at(col, ridx);
      return !v.empty() && v.front() == '-';
    };

    for (const auto ridx : row_plan.head) {
      if (row_is_negative(ridx)) {
        return true;
      }
    }
    for (const auto ridx : row_plan.tail) {
      if (row_is_negative(ridx)) {
        return true;
      }
    }
    return false;
  };

  std::unordered_map<std::int64_t, bool> sign_pad_by_column;
  sign_pad_by_column.reserve(col_plan.head.size() + col_plan.tail.size());
  for (const auto cidx : col_plan.head) {
    sign_pad_by_column.emplace(cidx, has_negative_visible_value(cidx));
  }
  for (const auto cidx : col_plan.tail) {
    sign_pad_by_column.emplace(cidx, has_negative_visible_value(cidx));
  }

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

    auto render_data_cell = [&](std::int64_t cidx) {
      std::string value = thread_value_at(cols[static_cast<std::size_t>(cidx)], ridx);
      if (sign_pad_by_column.contains(cidx) && sign_pad_by_column.at(cidx)) {
        if (!value.empty() && value != "NA" && value.front() != '-') {
          value.insert(value.begin(), ' ');
        }
      }
      return clip_cell(std::move(value), options.max_cell_width);
    };

    for (const auto cidx : col_plan.head) {
      row.push_back(render_data_cell(cidx));
    }
    if (col_plan.ellipsis) {
      row.push_back("...");
    }
    for (const auto cidx : col_plan.tail) {
      row.push_back(render_data_cell(cidx));
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

bool compiled_with_gui_backend_() {
#if defined(PLUSSMIC_HAVE_GTK_WEBKIT) && PLUSSMIC_HAVE_GTK_WEBKIT
  return true;
#elif defined(PLUSSMIC_HAVE_WIN_WEBVIEW2) && PLUSSMIC_HAVE_WIN_WEBVIEW2
  return true;
#elif defined(PLUSSMIC_HAVE_MAC_WEBKIT) && PLUSSMIC_HAVE_MAC_WEBKIT
  return true;
#else
  return false;
#endif
}

bool runtime_gui_env_available_() {
#if defined(_WIN32) || defined(__APPLE__)
  return true;
#else
  return std::getenv("DISPLAY") != nullptr || std::getenv("WAYLAND_DISPLAY") != nullptr;
#endif
}

DisplayBackend resolve_backend_(const DisplayOptions& options) {
  const auto configured =
      options.backend == DisplayBackend::Auto ? g_default_backend.load(std::memory_order_relaxed) : options.backend;
  if (configured == DisplayBackend::Auto) {
    return (compiled_with_gui_backend_() && runtime_gui_env_available_()) ? DisplayBackend::GtkWebKit
                                                                           : DisplayBackend::Terminal;
  }
  if (configured == DisplayBackend::GtkWebKit && (!compiled_with_gui_backend_() || !runtime_gui_env_available_())) {
    return DisplayBackend::Terminal;
  }
  return configured;
}

std::string escape_html_(std::string_view text) {
  std::string out;
  out.reserve(text.size() + 16);
  for (const auto ch : text) {
    switch (ch) {
      case '&':
        out += "&amp;";
        break;
      case '<':
        out += "&lt;";
        break;
      case '>':
        out += "&gt;";
        break;
      case '"':
        out += "&quot;";
        break;
      case '\'':
        out += "&#39;";
        break;
      default:
        out.push_back(ch);
        break;
    }
  }
  return out;
}

std::string document_title_(std::string_view fallback, const DisplayOptions& options) {
  if (!options.window_title.empty()) {
    return options.window_title;
  }
  return std::string(fallback);
}

std::string native_window_title_(std::string_view fallback) {
  return std::string(fallback);
}

std::string html_shell_(std::string_view title,
                        std::string_view subtitle,
                        std::string_view body,
                        const DisplayOptions& options) {
  const auto sortable = options.html_sortable ? "true" : "false";
  const auto search = options.html_search ? "true" : "false";
  const auto copy = options.html_copy_button ? "true" : "false";

  return fmt::format(R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>{0}</title>
  <style>
    :root {{
      --paper: #f7f3ea;
      --ink: #17212b;
      --muted: #56606d;
      --grid: #d6cfc3;
      --line: #c4b9a8;
      --accent: #0f766e;
      --accent-2: #c2410c;
      --panel: rgba(255,255,255,0.82);
      --shadow: 0 22px 70px rgba(23, 33, 43, 0.18);
    }}
    * {{ box-sizing: border-box; }}
    body {{
      margin: 0;
      color: var(--ink);
      font-family: "IBM Plex Sans", "Avenir Next", "Segoe UI", sans-serif;
      background:
        radial-gradient(circle at top left, rgba(194, 65, 12, 0.10), transparent 26rem),
        radial-gradient(circle at top right, rgba(15, 118, 110, 0.12), transparent 28rem),
        linear-gradient(180deg, #f4efe4 0%, #ece5d7 100%);
    }}
    .shell {{
      min-height: 100vh;
      padding: 22px;
    }}
    .frame {{
      max-width: 1440px;
      margin: 0 auto;
      background: var(--panel);
      border: 1px solid rgba(196, 185, 168, 0.9);
      border-radius: 20px;
      overflow: hidden;
      box-shadow: var(--shadow);
      backdrop-filter: blur(10px);
    }}
    .hero {{
      padding: 22px 24px 16px;
      border-bottom: 1px solid rgba(196, 185, 168, 0.7);
      background:
        linear-gradient(135deg, rgba(255,255,255,0.88), rgba(249,245,238,0.7)),
        linear-gradient(90deg, rgba(15, 118, 110, 0.08), rgba(194, 65, 12, 0.08));
      display: flex;
      gap: 18px;
      align-items: end;
      justify-content: space-between;
      flex-wrap: wrap;
    }}
    .hero h1 {{
      margin: 0;
      font-family: "IBM Plex Serif", Georgia, serif;
      font-size: 1.65rem;
      font-weight: 600;
      letter-spacing: -0.02em;
    }}
    .hero p {{
      margin: 6px 0 0;
      color: var(--muted);
      font-size: 0.96rem;
    }}
    .tools {{
      display: flex;
      gap: 10px;
      align-items: center;
      flex-wrap: wrap;
    }}
    .tools input {{
      min-width: 240px;
      padding: 10px 12px;
      border-radius: 999px;
      border: 1px solid rgba(86, 96, 109, 0.22);
      background: rgba(255,255,255,0.86);
      color: var(--ink);
      font: inherit;
    }}
    .tools button {{
      padding: 10px 14px;
      border-radius: 999px;
      border: 0;
      background: linear-gradient(135deg, var(--accent), #155e75);
      color: white;
      font: inherit;
      cursor: pointer;
    }}
    .tools button.secondary {{
      background: linear-gradient(135deg, var(--accent-2), #9a3412);
    }}
    .content {{
      padding: 0 18px 18px;
    }}
    .summary-grid {{
      display: grid;
      gap: 12px;
      grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
      padding: 18px 6px 8px;
    }}
    .summary-card {{
      padding: 14px 16px;
      border-radius: 16px;
      border: 1px solid rgba(196, 185, 168, 0.7);
      background: rgba(255,255,255,0.76);
    }}
    .summary-card .label {{
      color: var(--muted);
      font-size: 0.78rem;
      text-transform: uppercase;
      letter-spacing: 0.08em;
    }}
    .summary-card .value {{
      margin-top: 6px;
      font-size: 1.05rem;
      font-weight: 600;
    }}
    .table-wrap {{
      overflow: auto;
      margin-top: 10px;
      border: 1px solid rgba(196, 185, 168, 0.8);
      border-radius: 16px;
      background: rgba(255,255,255,0.88);
    }}
    table {{
      width: 100%;
      border-collapse: collapse;
      font-size: 0.95rem;
    }}
    thead th {{
      position: sticky;
      top: 0;
      z-index: 2;
      background: #f7efe2;
      border-bottom: 1px solid var(--line);
      text-align: left;
      white-space: nowrap;
    }}
    th, td {{
      padding: 10px 12px;
      border-bottom: 1px solid rgba(214, 207, 195, 0.75);
      vertical-align: top;
    }}
    tbody tr:nth-child(even) {{
      background: rgba(247, 243, 234, 0.55);
    }}
    tbody tr:hover {{
      background: rgba(15, 118, 110, 0.08);
    }}
    th.sortable {{
      cursor: pointer;
      user-select: none;
    }}
    th.sortable::after {{
      content: "  ↕";
      color: rgba(86, 96, 109, 0.8);
      font-size: 0.85em;
    }}
    .sticky-left {{
      position: sticky;
      left: 0;
      z-index: 1;
      background: inherit;
    }}
    thead .sticky-left {{
      z-index: 3;
      background: #f7efe2;
    }}
    .num {{
      text-align: right;
      font-variant-numeric: tabular-nums;
    }}
    .muted {{
      color: var(--muted);
    }}
    .hidden {{
      display: none;
    }}
    .pill {{
      display: inline-block;
      padding: 4px 8px;
      border-radius: 999px;
      background: rgba(15, 118, 110, 0.10);
      color: var(--accent);
      font-size: 0.85rem;
      font-weight: 600;
    }}
    .section-label {{
      display: inline-block;
      margin: 10px 0 8px;
      color: var(--muted);
      font-size: 0.78rem;
      text-transform: uppercase;
      letter-spacing: 0.08em;
    }}
    .atlas-nav {{
      display: flex;
      gap: 10px;
      overflow: auto;
      padding: 10px 6px 6px;
      margin-bottom: 8px;
    }}
    .atlas-tab {{
      padding: 10px 14px;
      border: 1px solid rgba(196, 185, 168, 0.85);
      border-radius: 999px;
      background: rgba(255,255,255,0.84);
      color: var(--ink);
      cursor: pointer;
      white-space: nowrap;
      font: inherit;
    }}
    .atlas-tab.active {{
      background: linear-gradient(135deg, rgba(15,118,110,0.16), rgba(194,65,12,0.12));
      border-color: rgba(15, 118, 110, 0.38);
      color: var(--ink);
      font-weight: 600;
    }}
    .atlas-panel {{
      display: none;
    }}
    .atlas-panel.active {{
      display: block;
    }}
    .atlas-frame-head {{
      display: flex;
      gap: 10px;
      align-items: center;
      justify-content: space-between;
      flex-wrap: wrap;
      padding: 8px 6px 0;
    }}
    .atlas-frame-title {{
      margin: 0;
      font-family: "IBM Plex Serif", Georgia, serif;
      font-size: 1.2rem;
      font-weight: 600;
      letter-spacing: -0.01em;
    }}
    @media (max-width: 720px) {{
      .shell {{
        padding: 10px;
      }}
      .hero {{
        padding: 18px 16px 14px;
      }}
      .content {{
        padding: 0 10px 10px;
      }}
      .tools input {{
        min-width: 0;
        width: 100%;
      }}
    }}
  </style>
</head>
<body>
  <div class="shell">
    <div class="frame">
      <header class="hero">
        <div>
          <h1>{0}</h1>
          <p>{1}</p>
        </div>
        <div class="tools">
          <input id="table-search" type="search" placeholder="Search visible rows" />
          <button id="copy-visible" class="secondary" type="button">Copy visible</button>
        </div>
      </header>
      <main class="content">
        {2}
      </main>
    </div>
  </div>
  <script>
    const enableSort = {3};
    const enableSearch = {4};
    const enableCopy = {5};
    const table = document.getElementById('plussmic-table');
    const detailTables = document.querySelectorAll('table[data-filterable="true"]');
    const searchInput = document.getElementById('table-search');
    const copyButton = document.getElementById('copy-visible');

    if (!enableSearch) {{
      searchInput.classList.add('hidden');
    }}
    if (!enableCopy) {{
      copyButton.classList.add('hidden');
    }}

    function filterTables(query) {{
      const needle = query.trim().toLowerCase();
      detailTables.forEach((tbl) => {{
        if (tbl.offsetParent === null) return;
        Array.from(tbl.tBodies).forEach((tbody) => {{
          Array.from(tbody.rows).forEach((row) => {{
            const haystack = row.innerText.toLowerCase();
            row.classList.toggle('hidden', needle.length > 0 && !haystack.includes(needle));
          }});
        }});
      }});
    }}

    function sortTable(tbl, columnIndex, numeric) {{
      const tbody = tbl.tBodies[0];
      if (!tbody) return;
      const rows = Array.from(tbody.rows);
      const current = tbl.dataset.sortDir === 'asc' ? 'asc' : 'desc';
      const next = current === 'asc' ? 'desc' : 'asc';
      rows.sort((a, b) => {{
        const av = a.cells[columnIndex]?.dataset.sortValue ?? a.cells[columnIndex]?.innerText ?? '';
        const bv = b.cells[columnIndex]?.dataset.sortValue ?? b.cells[columnIndex]?.innerText ?? '';
        if (numeric) {{
          const an = Number(av);
          const bn = Number(bv);
          if (Number.isFinite(an) && Number.isFinite(bn)) {{
            return next === 'asc' ? an - bn : bn - an;
          }}
        }}
        return next === 'asc'
          ? av.localeCompare(bv, undefined, {{ numeric: true, sensitivity: 'base' }})
          : bv.localeCompare(av, undefined, {{ numeric: true, sensitivity: 'base' }});
      }});
      rows.forEach((row) => tbody.appendChild(row));
      tbl.dataset.sortDir = next;
    }}

    if (enableSort) {{
      document.querySelectorAll('th.sortable').forEach((th) => {{
        th.addEventListener('click', () => {{
          const tbl = th.closest('table');
          const columnIndex = Number(th.dataset.columnIndex || '0');
          const numeric = th.dataset.kind === 'numeric';
          sortTable(tbl, columnIndex, numeric);
        }});
      }});
    }}

    if (enableSearch) {{
      searchInput.addEventListener('input', (event) => filterTables(event.target.value));
    }}

    async function copyVisibleTable() {{
      const blocks = [];
      detailTables.forEach((tbl) => {{
        if (tbl.offsetParent === null) return;
        const header = Array.from(tbl.tHead?.rows[0]?.cells ?? []).map((cell) => cell.innerText.trim()).join('\t');
        const lines = [header];
        Array.from(tbl.tBodies[0]?.rows ?? []).forEach((row) => {{
          if (row.classList.contains('hidden')) return;
          lines.push(Array.from(row.cells).map((cell) => cell.innerText.trim()).join('\t'));
        }});
        blocks.push(lines.join('\n'));
      }});
      const payload = blocks.join('\n\n');
      try {{
        await navigator.clipboard.writeText(payload);
      }} catch (err) {{
        const area = document.createElement('textarea');
        area.value = payload;
        document.body.appendChild(area);
        area.select();
        document.execCommand('copy');
        area.remove();
      }}
    }}

    if (enableCopy) {{
      copyButton.addEventListener('click', copyVisibleTable);
    }}

    document.querySelectorAll('.atlas-tab').forEach((button) => {{
      button.addEventListener('click', () => {{
        const index = button.dataset.frameIndex;
        document.querySelectorAll('.atlas-tab').forEach((candidate) => {{
          candidate.classList.toggle('active', candidate === button);
        }});
        document.querySelectorAll('.atlas-panel').forEach((panel) => {{
          panel.classList.toggle('active', panel.dataset.frameIndex === index);
        }});
        if (enableSearch) {{
          filterTables(searchInput.value || '');
        }}
      }});
    }});
  </script>
</body>
</html>)HTML",
                     escape_html_(title),
                     escape_html_(subtitle),
                     body,
                     sortable,
                     search,
                     copy);
}

std::string render_html_pre_(std::string_view title,
                             std::string_view subtitle,
                             const std::string& content,
                             const DisplayOptions& options) {
  const auto body = fmt::format(
      R"HTML(<div class="table-wrap"><pre style="margin:0; padding:16px; font-family:'IBM Plex Mono', 'SFMono-Regular', monospace; white-space:pre-wrap;">{}</pre></div>)HTML",
      escape_html_(content));
  return html_shell_(title, subtitle, body, options);
}

std::string render_html_table_body_(const DataFrame& frame,
                                    const DisplayOptions& options,
                                    std::string_view table_id) {
  const auto cols = frame.columns();
  const auto names = frame.column_names();

  std::string body;
  body += "<div class=\"summary-grid\">";
  body += fmt::format("<div class=\"summary-card\"><div class=\"label\">Cases</div><div class=\"value\">{}</div></div>",
                      frame.nrows());
  body +=
      fmt::format("<div class=\"summary-card\"><div class=\"label\">Features</div><div class=\"value\">{}</div></div>",
                  frame.ncols());
  body += fmt::format(
      "<div class=\"summary-card\"><div class=\"label\">Case Axis</div><div class=\"value\">{}</div></div>",
      escape_html_(logical_dtype_name(frame.cases())));
  body += "</div>";

  body += "<div class=\"table-wrap\">";
  body += fmt::format("<table id=\"{}\" data-filterable=\"true\"><thead><tr>", escape_html_(table_id));

  std::size_t column_index = 0;
  if (options.show_case_index) {
    const auto case_numeric = is_numeric_thread(frame.cases());
    body += fmt::format(
        "<th class=\"sortable sticky-left{}\" data-column-index=\"{}\" data-kind=\"{}\">case</th>",
        case_numeric ? " num" : "",
        column_index++,
        case_numeric ? "numeric" : "text");
  }

  for (std::size_t i = 0; i < names.size(); ++i) {
    const auto numeric = is_numeric_thread(cols[i]);
    body += fmt::format("<th class=\"sortable{}\" data-column-index=\"{}\" data-kind=\"{}\">{}</th>",
                        numeric ? " num" : "",
                        column_index++,
                        numeric ? "numeric" : "text",
                        escape_html_(names[i]));
  }
  body += "</tr></thead><tbody>";

  for (std::int64_t row = 0; row < frame.nrows(); ++row) {
    body += "<tr>";
    if (options.show_case_index) {
      const auto case_value = thread_value_at(frame.cases(), row);
      const auto case_numeric = is_numeric_thread(frame.cases());
      body += fmt::format("<td class=\"sticky-left{}\" data-sort-value=\"{}\">{}</td>",
                          case_numeric ? " num" : "",
                          escape_html_(case_value),
                          escape_html_(case_value));
    }
    for (std::size_t col = 0; col < cols.size(); ++col) {
      const auto value = thread_value_at(cols[col], row);
      const auto numeric = is_numeric_thread(cols[col]);
      body += fmt::format("<td class=\"{}\" data-sort-value=\"{}\">{}</td>",
                          numeric ? "num" : "",
                          escape_html_(value),
                          escape_html_(value));
    }
    body += "</tr>";
  }

  body += "</tbody></table></div>";
  return body;
}

std::string render_html_table_(const DataFrame& frame,
                               std::string_view title,
                               std::string_view subtitle,
                               const DisplayOptions& options) {
  const auto body = render_html_table_body_(frame, options, "plussmic-table");
  return html_shell_(title, subtitle, body, options);
}

std::string render_html_atlas_(const Atlas& atlas, const DisplayOptions& options) {
  std::string body;
  body += "<div class=\"summary-grid\">";
  body += fmt::format("<div class=\"summary-card\"><div class=\"label\">Depth</div><div class=\"value\">{}</div></div>",
                      atlas.depth());
  body += fmt::format("<div class=\"summary-card\"><div class=\"label\">Cases</div><div class=\"value\">{}</div></div>",
                      atlas.nrows());
  body += fmt::format("<div class=\"summary-card\"><div class=\"label\">Features</div><div class=\"value\">{}</div></div>",
                      atlas.ncols());
  body += fmt::format("<div class=\"summary-card\"><div class=\"label\">Depth Labels</div><div class=\"value\">{}</div></div>",
                      atlas.axis0_labels().has_value() ? escape_html_(logical_dtype_name(*atlas.axis0_labels()))
                                                       : "None");
  body += "</div>";

  if (atlas.depth() == 0) {
    body += "<div class=\"table-wrap\"><div style=\"padding:18px;\" class=\"muted\">Atlas is empty.</div></div>";
    return html_shell_(document_title_("Atlas", options),
                       "0 frames",
                       body,
                       options);
  }

  body += "<div class=\"section-label\">Frames</div>";
  body += "<div class=\"atlas-nav\">";
  for (std::int64_t i = 0; i < atlas.depth(); ++i) {
    std::string label = fmt::format("Frame {}", i);
    if (atlas.axis0_labels().has_value()) {
      label = thread_value_at(*atlas.axis0_labels(), i);
    }
    body += fmt::format("<button type=\"button\" class=\"atlas-tab{}\" data-frame-index=\"{}\">{}</button>",
                        i == 0 ? " active" : "",
                        i,
                        escape_html_(label));
  }
  body += "</div>";

  for (std::int64_t i = 0; i < atlas.depth(); ++i) {
    std::string label = fmt::format("Frame {}", i);
    if (atlas.axis0_labels().has_value()) {
      label = thread_value_at(*atlas.axis0_labels(), i);
    }
    const auto frame = atlas.frame(i);
    body += fmt::format("<section class=\"atlas-panel{}\" data-frame-index=\"{}\">",
                        i == 0 ? " active" : "",
                        i);
    body += "<div class=\"atlas-frame-head\">";
    body += fmt::format("<h2 class=\"atlas-frame-title\">{}</h2>", escape_html_(label));
    body += fmt::format("<span class=\"pill\">frame {}</span>", i);
    body += "</div>";
    body += render_html_table_body_(frame, options, fmt::format("plussmic-table-{}", i));
    body += "</section>";
  }

  return html_shell_(document_title_("Atlas", options),
                     fmt::format("{} frames, {} cases, {} features", atlas.depth(), atlas.nrows(), atlas.ncols()),
                     body,
                     options);
}

std::string render_html_info_doc_(const DataFrame& frame, const DisplayOptions& options) {
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

  std::string body;
  body += "<div class=\"summary-grid\">";
  body += fmt::format("<div class=\"summary-card\"><div class=\"label\">Class</div><div class=\"value\">{}</div></div>",
                      "pluss::table::DataFrame");
  body += fmt::format("<div class=\"summary-card\"><div class=\"label\">Cases</div><div class=\"value\">{}</div></div>",
                      frame.nrows());
  body +=
      fmt::format("<div class=\"summary-card\"><div class=\"label\">Features</div><div class=\"value\">{}</div></div>",
                  frame.ncols());
  body += fmt::format(
      "<div class=\"summary-card\"><div class=\"label\">Case Range</div><div class=\"value\">{} <span class=\"muted\">to</span> {}</div></div>",
      escape_html_(first_case),
      escape_html_(last_case));
  body += fmt::format(
      "<div class=\"summary-card\"><div class=\"label\">Null Case Rows</div><div class=\"value\">{}</div></div>",
      frame.count_null_rows());
  body += fmt::format("<div class=\"summary-card\"><div class=\"label\">Dtypes</div><div class=\"value\">{}</div></div>",
                      escape_html_(dtype_summary_line(dtype_counts)));
  body += fmt::format("<div class=\"summary-card\"><div class=\"label\">Memory</div><div class=\"value\">{}</div></div>",
                      escape_html_(human_bytes(memory_bytes)));
  body += "</div>";

  body +=
      "<div style=\"padding:8px 6px 0;\"><span class=\"pill\">Feature inventory</span></div><div class=\"table-wrap\">";
  body += "<table id=\"plussmic-table\" data-filterable=\"true\"><thead><tr>";
  body += "<th class=\"sortable\" data-column-index=\"0\" data-kind=\"numeric\">#</th>";
  body += "<th class=\"sortable sticky-left\" data-column-index=\"1\" data-kind=\"text\">Feature</th>";
  body += "<th class=\"sortable\" data-column-index=\"2\" data-kind=\"numeric\">Non-Null Cases</th>";
  body += "<th class=\"sortable\" data-column-index=\"3\" data-kind=\"text\">Dtype</th>";
  body += "</tr></thead><tbody>";

  for (std::size_t i = 0; i < names.size(); ++i) {
    const auto nn = non_null_count(cols[i]);
    const auto dtype = logical_dtype_name(cols[i]);
    body += fmt::format(
        "<tr><td class=\"num\" data-sort-value=\"{0}\">{0}</td><td class=\"sticky-left\" data-sort-value=\"{1}\">{2}</td><td class=\"num\" data-sort-value=\"{3}\">{3}</td><td data-sort-value=\"{4}\">{5}</td></tr>",
        i,
        escape_html_(names[i]),
        escape_html_(names[i]),
        nn,
        escape_html_(dtype),
        escape_html_(dtype));
  }

  body += "</tbody></table></div>";
  return html_shell_(document_title_("DataFrame Info", options),
                     fmt::format("{} cases, {} features", frame.nrows(), frame.ncols()),
                     body,
                     options);
}

bool show_html_window_(const std::string& html, const DisplayOptions& options, std::string_view title) {
#if defined(PLUSSMIC_HAVE_GTK_WEBKIT) && PLUSSMIC_HAVE_GTK_WEBKIT
  struct SnapshotRequest {
    std::string path;
  };

  if (!runtime_gui_env_available_()) {
    return false;
  }
  if (!gtk_init_check(nullptr, nullptr)) {
    return false;
  }

  GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), native_window_title_(title).c_str());
  gtk_window_set_default_size(GTK_WINDOW(window),
                              static_cast<int>(std::max<std::int64_t>(640, options.viewer_width)),
                              static_cast<int>(std::max<std::int64_t>(420, options.viewer_height)));

  g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), nullptr);
  g_signal_connect(window,
                   "key-press-event",
                   G_CALLBACK(+[](GtkWidget*, GdkEventKey* event, gpointer) -> gboolean {
                     if (event->keyval == GDK_KEY_Escape) {
                       gtk_main_quit();
                       return TRUE;
                     }
                     return FALSE;
                   }),
                   nullptr);

  GtkWidget* root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add(GTK_CONTAINER(window), root);

  GtkWidget* toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_margin_top(toolbar, 10);
  gtk_widget_set_margin_bottom(toolbar, 6);
  gtk_widget_set_margin_start(toolbar, 12);
  gtk_widget_set_margin_end(toolbar, 12);
  gtk_box_pack_start(GTK_BOX(root), toolbar, FALSE, FALSE, 0);

  GtkWidget* save_button = gtk_button_new_with_label("Save PNG");
  gtk_box_pack_end(GTK_BOX(toolbar), save_button, FALSE, FALSE, 0);

  GtkWidget* scroller = gtk_scrolled_window_new(nullptr, nullptr);
  gtk_box_pack_start(GTK_BOX(root), scroller, TRUE, TRUE, 0);

  WebKitWebView* webview = WEBKIT_WEB_VIEW(webkit_web_view_new());
  webkit_web_view_load_html(webview, html.c_str(), "file:///");
  gtk_container_add(GTK_CONTAINER(scroller), GTK_WIDGET(webview));

  g_signal_connect(
      save_button,
      "clicked",
      G_CALLBACK(+[](GtkButton*, gpointer user_data) {
        auto* webview = WEBKIT_WEB_VIEW(user_data);
        GtkWidget* dialog = gtk_file_chooser_dialog_new("Save Table Snapshot",
                                                        nullptr,
                                                        GTK_FILE_CHOOSER_ACTION_SAVE,
                                                        "_Cancel",
                                                        GTK_RESPONSE_CANCEL,
                                                        "_Save",
                                                        GTK_RESPONSE_ACCEPT,
                                                        nullptr);
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "table-view.png");
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
          char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
          auto* request = new SnapshotRequest{filename != nullptr ? filename : "table-view.png"};
          if (filename != nullptr) {
            g_free(filename);
          }
          webkit_web_view_get_snapshot(
              webview,
              WEBKIT_SNAPSHOT_REGION_FULL_DOCUMENT,
              WEBKIT_SNAPSHOT_OPTIONS_NONE,
              nullptr,
              +[](GObject* source, GAsyncResult* result, gpointer user_data) {
                std::unique_ptr<SnapshotRequest> request(static_cast<SnapshotRequest*>(user_data));
                GError* error = nullptr;
                cairo_surface_t* surface =
                    webkit_web_view_get_snapshot_finish(WEBKIT_WEB_VIEW(source), result, &error);
                if (error != nullptr) {
                  g_error_free(error);
                  return;
                }
                if (surface != nullptr) {
                  cairo_surface_write_to_png(surface, request->path.c_str());
                  cairo_surface_destroy(surface);
                }
              },
              request);
        }
        gtk_widget_destroy(dialog);
      }),
      webview);

  gtk_widget_show_all(window);
  gtk_main();
  return true;
#elif defined(PLUSSMIC_HAVE_MAC_WEBKIT) && PLUSSMIC_HAVE_MAC_WEBKIT
  if (!detail::mac_html_viewer_available()) {
    return false;
  }
  return detail::show_macos_html_window(native_window_title_(title),
                                        html,
                                        std::max<std::int64_t>(640, options.viewer_width),
                                        std::max<std::int64_t>(420, options.viewer_height));
#elif defined(PLUSSMIC_HAVE_WIN_WEBVIEW2) && PLUSSMIC_HAVE_WIN_WEBVIEW2
  if (!detail::windows_html_viewer_available()) {
    return false;
  }
  return detail::show_windows_html_window(native_window_title_(title),
                                          html,
                                          std::max<std::int64_t>(640, options.viewer_width),
                                          std::max<std::int64_t>(420, options.viewer_height));
#else
  (void)html;
  (void)options;
  (void)title;
  return false;
#endif
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

DisplayBackend default_backend() {
  return g_default_backend.load(std::memory_order_relaxed);
}

void set_default_backend(DisplayBackend backend) {
  g_default_backend.store(backend, std::memory_order_relaxed);
}

bool gui_backend_available() {
  return compiled_with_gui_backend_() && runtime_gui_env_available_();
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

std::string render_html(const DataFrame& frame, const DisplayOptions& options) {
  return render_html_table_(
      frame,
      document_title_("DataFrame", options),
      fmt::format("{} cases, {} features", frame.nrows(), frame.ncols()),
      options);
}

std::string render_html(const Thread& thread, const DisplayOptions& options) {
  DataFrame tmp({{"value", thread}});
  return render_html(tmp, options);
}

std::string render_html(const Atlas& atlas, const DisplayOptions& options) {
  return render_html_atlas_(atlas, options);
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

std::string render_info_html(const DataFrame& frame, const DisplayOptions& options) {
  return render_html_info_doc_(frame, options);
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

void show(const DataFrame& frame, const DisplayOptions& options) {
  if (resolve_backend_(options) == DisplayBackend::GtkWebKit) {
    const auto html = render_html(frame, options);
    if (show_html_window_(html, options, "Pluss DataFrame")) {
      return;
    }
  }
  print(std::cout, frame, options);
}

void show(const Thread& thread, const DisplayOptions& options) {
  if (resolve_backend_(options) == DisplayBackend::GtkWebKit) {
    const auto html = render_html(thread, options);
    if (show_html_window_(html, options, "PlussMic Thread")) {
      return;
    }
  }
  print(std::cout, thread, options);
}

void show(const Atlas& atlas, const DisplayOptions& options) {
  if (resolve_backend_(options) == DisplayBackend::GtkWebKit) {
    const auto html = render_html(atlas, options);
    if (show_html_window_(html, options, "Pluss Atlas")) {
      return;
    }
  }
  print(std::cout, atlas, options);
}

void show_info(const DataFrame& frame, const DisplayOptions& options) {
  if (resolve_backend_(options) == DisplayBackend::GtkWebKit) {
    const auto html = render_info_html(frame, options);
    if (show_html_window_(html, options, "Pluss DataFrame")) {
      return;
    }
  }
  print_info(std::cout, frame, options);
}

void print(std::ostream& os, const Thread& thread, const DisplayOptions& options) {
  os << render(thread, options) << '\n';
}

void print(std::ostream& os, const DataFrame& frame, const DisplayOptions& options) {
  os << render(frame, options) << '\n';
}

void print(std::ostream& os, const Atlas& atlas, const DisplayOptions& options) {
  os << render(atlas, options) << '\n';
}

void print_info(std::ostream& os, const DataFrame& frame, const DisplayOptions& options) {
  os << render_info(frame, options) << '\n';
}

} // namespace pluss::table::display
