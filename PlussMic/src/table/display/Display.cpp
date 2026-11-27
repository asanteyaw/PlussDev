#include "table/display/Display.h"
#include "table/display/StackPage.h"

#include <algorithm>
#include <regex>
#include <array>
#include <atomic>
#include <charconv>
#include <mutex>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>


#if !defined(_WIN32)
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#include "CodeM/core/Complex.h"
#include "CodeM/core/Half.h"
#include "CodeM/core/ScalarType.h"
#include "pluss/tils/view/html_window.h"
#include "pluss/tils/view/table_order.h"
#include "pluss/tils/view/window/export.h"
#include "pluss/tils/view/window/html_document.h"
#include "pluss/tils/view/window/html_layout.h"
#include "pluss/tils/view/window/renderer.h"
#include "pluss/tils/view/window/text_layout.h"
#include "table/Atlas.h"
#include "table/DataFrame.h"
#include "table/Shelf.h"
#include <pluss/tils/kit/output.h>

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
std::atomic<DisplayTheme> g_default_theme{DisplayTheme::Default};

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

std::vector<std::string> split_display_token_(std::string_view text,
                                              char separator = '\x1F') {
  std::vector<std::string> parts;
  std::size_t start = 0;
  while (start <= text.size()) {
    const auto stop = text.find(separator, start);
    if (stop == std::string_view::npos) {
      parts.emplace_back(text.substr(start));
      break;
    }
    parts.emplace_back(text.substr(start, stop - start));
    start = stop + 1;
  }
  return parts;
}

std::string scalar_at(const pluss::Series& s, std::int64_t row) {
  const auto format_float = [](double value, std::string_view spec) {
    // NULL is a missing cell (see shelf_value_at). A floating NaN is a value, "not a number" (an integer remainder by zero, 0/0),
    // and prints as NaN; it must not look like missing data.
    return std::isnan(value) ? std::string("NaN")
                             : pluss::tils::OutputEngine::format(pluss::tils::runtime_format(spec), value);
  };
  const auto st = s.scalar_type();
  const auto* base = s.raw_data();
  switch (st) {
    case pluss::codem::ScalarType::Bool:
      return reinterpret_cast<const bool*>(base)[row] ? "true" : "false";
    case pluss::codem::ScalarType::Int8:
      return pluss::tils::OutputEngine::format("{}", static_cast<int>(reinterpret_cast<const std::int8_t*>(base)[row]));
    case pluss::codem::ScalarType::Int16:
      return pluss::tils::OutputEngine::format("{}", reinterpret_cast<const std::int16_t*>(base)[row]);
    case pluss::codem::ScalarType::Int32:
      return pluss::tils::OutputEngine::format("{}", reinterpret_cast<const std::int32_t*>(base)[row]);
    case pluss::codem::ScalarType::Int64:
      return pluss::tils::OutputEngine::format("{}", reinterpret_cast<const std::int64_t*>(base)[row]);
    case pluss::codem::ScalarType::UInt8:
      return pluss::tils::OutputEngine::format("{}", static_cast<unsigned>(reinterpret_cast<const std::uint8_t*>(base)[row]));
    case pluss::codem::ScalarType::UInt16:
      return pluss::tils::OutputEngine::format("{}", reinterpret_cast<const std::uint16_t*>(base)[row]);
    case pluss::codem::ScalarType::UInt32:
      return pluss::tils::OutputEngine::format("{}", reinterpret_cast<const std::uint32_t*>(base)[row]);
    case pluss::codem::ScalarType::UInt64:
      return pluss::tils::OutputEngine::format("{}", reinterpret_cast<const std::uint64_t*>(base)[row]);
    case pluss::codem::ScalarType::Float16:
      return format_float(
          pluss::codem::float16_to_float(reinterpret_cast<const pluss::codem::Float16*>(base)[row]),
          "{:.6g}");
    case pluss::codem::ScalarType::BFloat16:
      return format_float(
          pluss::codem::bfloat16_to_float(reinterpret_cast<const pluss::codem::BFloat16*>(base)[row]),
          "{:.6g}");
    case pluss::codem::ScalarType::Float32:
      return format_float(reinterpret_cast<const float*>(base)[row], "{:.8g}");
    case pluss::codem::ScalarType::Float64:
      return format_float(reinterpret_cast<const double*>(base)[row], "{:.10g}");
    case pluss::codem::ScalarType::Complex64: {
      const auto v = reinterpret_cast<const pluss::codem::Complex64*>(base)[row];
      return pluss::tils::OutputEngine::format("({:.6g}{:+.6g}j)", v.real(), v.imag());
    }
    case pluss::codem::ScalarType::Complex128: {
      const auto v = reinterpret_cast<const pluss::codem::Complex128*>(base)[row];
      return pluss::tils::OutputEngine::format("({:.6g}{:+.6g}j)", v.real(), v.imag());
    }
    default:
      return "<scalar?>";
  }
}

bool is_numeric_shelf(const Shelf& t) {
  if (t.kind() != ShelfKind::Series) {
    return false;
  }
  const auto dt = t.as_series().dtype();
  return dt.is_floating() || dt.is_integral() || dt.scalar == pluss::codem::ScalarType::Bool;
}

bool series_all_zero_i64(const pluss::Series& s) {
  const auto values = s.host<std::int64_t>();
  return std::all_of(values.begin(), values.end(), [](const std::int64_t v) { return v == 0; });
}

Shelf temporal_shelf_to_display_note(const Shelf& t) {
  const auto timeline = t.as_timeline();
  const auto has_subsecond = !series_all_zero_i64(timeline.nanosecond());
  const auto has_clock_time = has_subsecond || !series_all_zero_i64(timeline.second())
                              || !series_all_zero_i64(timeline.minute())
                              || !series_all_zero_i64(timeline.hour());
  if (!has_clock_time) {
    return Shelf(timeline.strftime("%Y-%m-%d"), detail::validity_mask(t));
  }
  if (!has_subsecond) {
    return Shelf(timeline.strftime("%Y-%m-%d %H:%M:%S"), detail::validity_mask(t));
  }
  return Shelf(timeline.strftime("%Y-%m-%d %H:%M:%S.%f"), detail::validity_mask(t));
}

Shelf display_shelf(const Shelf& t) {
  if (t.kind() == ShelfKind::Timeline) {
    return temporal_shelf_to_display_note(t);
  }
  if (t.kind() == ShelfKind::Duration) {
    return t.cast().note();
  }
  return t;
}

std::string shelf_value_at(const Shelf& t, std::int64_t row) {
  if (!detail::valid_at(t, row)) {
    return "NULL";
  }

  switch (t.kind()) {
    case ShelfKind::Series:
      return scalar_at(t.as_series(), row);
    case ShelfKind::Note:
      return std::string(t.as_note().at(row));
    case ShelfKind::Timeline:
      return pluss::tils::OutputEngine::format("{}", t.as_timeline().ticks()[static_cast<std::size_t>(row)]);
    case ShelfKind::Duration:
      return std::string(t.as_duration().to_note_iso().at(row));
  }
  return "<?>";
}

std::int64_t non_null_count(const Shelf& t) {
  if (t.kind() == ShelfKind::Duration) {
    const auto values = t.as_duration().host();
    const auto valid = detail::has_mask(t)
                           ? detail::validity_mask(t)->host<bool>()
                           : std::vector<bool>(values.size(), true);
    std::int64_t count = 0;
    for (std::size_t i = 0; i < values.size(); ++i) {
      count += valid[i] && values[i] != pluss::Duration::kNaT ? 1 : 0;
    }
    return count;
  }
  if (!detail::has_mask(t)) {
    return t.size();
  }
  const auto valid = detail::validity_mask(t)->host<bool>();
  return static_cast<std::int64_t>(std::count(valid.begin(), valid.end(), true));
}

std::uint64_t estimate_bytes(const Shelf& t) {
  std::uint64_t bytes = 0;
  if (t.kind() == ShelfKind::Series) {
    const auto item = t.as_series().dtype().itemsize;
    bytes += static_cast<std::uint64_t>(item) * static_cast<std::uint64_t>(t.size());
  } else if (t.kind() == ShelfKind::Timeline || t.kind() == ShelfKind::Duration) {
    bytes += static_cast<std::uint64_t>(8) * static_cast<std::uint64_t>(t.size());
  } else if (t.kind() == ShelfKind::Note) {
    const auto& n = t.as_note();
    for (std::int64_t i = 0; i < t.size(); ++i) {
      bytes += static_cast<std::uint64_t>(n.at(i).size());
    }
  }
  if (detail::has_mask(t)) {
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
  return pluss::tils::OutputEngine::format("{:.2f} {}", v, units[u]);
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

std::vector<Shelf> dataframe_columns_(const DataFrame& frame) {
  std::vector<Shelf> columns;
  const auto& names = frame.features();
  columns.reserve(names.size());
  for (const auto& name : names) {
    columns.push_back(frame.features(name));
  }
  return columns;
}

TableMatrix build_dataframe_matrix(
    const DataFrame& frame,
    const EffectiveDisplayOptions& options,
    bool compact_columns) {
  const auto row_plan = build_row_plan(frame.ncases(), options);
  const auto col_plan = build_col_plan(frame.nfeatures(), options, compact_columns);

  const auto names = frame.features();
  const auto cols = dataframe_columns_(frame);
  const auto display_cases = display_shelf(frame.labels().values());
  std::vector<Shelf> display_cols;
  display_cols.reserve(cols.size());
  for (const auto& col : cols) {
    display_cols.push_back(display_shelf(col));
  }

  auto has_negative_visible_value = [&](std::int64_t cidx) {
    const auto& col = cols[static_cast<std::size_t>(cidx)];
    if (!is_numeric_shelf(col)) {
      return false;
    }

    auto row_is_negative = [&](std::int64_t ridx) {
      const auto v = shelf_value_at(display_cols[static_cast<std::size_t>(cidx)], ridx);
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
      row.push_back(clip_cell(shelf_value_at(display_cases, ridx), options.max_cell_width));
    }

    auto render_data_cell = [&](std::int64_t cidx) {
      std::string value = shelf_value_at(display_cols[static_cast<std::size_t>(cidx)], ridx);
      if (sign_pad_by_column.contains(cidx) && sign_pad_by_column.at(cidx)) {
        if (!value.empty() && value != "NULL" && value.front() != '-') {
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
    parts.push_back(pluss::tils::OutputEngine::format("{}({})", kv.first, kv.second));
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

std::string logical_dtype(const Shelf& t) {
  switch (t.kind()) {
    case ShelfKind::Series:
      return "Series";
    case ShelfKind::Note:
      return "Note";
    case ShelfKind::Timeline:
      return "Timeline";
    case ShelfKind::Duration:
      return "Duration";
  }
  return "Unknown";
}

// The one-dimensional layout every sequence shares (a Shelf, a Stream, a Slot): position and value side by side, the
// first and last rows with a row of dots between when there are many, and a footer line saying what it holds. Only the
// rows that are shown are asked for, so the cost does not grow with the length of the sequence.
std::string sequence_text_(const SequenceView& view, const EffectiveDisplayOptions& options, std::int64_t terminal_width) {
  const auto row_plan = build_row_plan(view.size, options);
  std::vector<std::pair<std::string, std::string>> rows;
  rows.reserve(row_plan.head.size() + row_plan.tail.size() + (row_plan.ellipsis ? 1 : 0));
  const auto push_row = [&](const std::int64_t row) {
    const auto label = view.label ? view.label(row) : pluss::tils::OutputEngine::format("{}", row);
    rows.emplace_back(clip_cell(label, options.max_cell_width), clip_cell(view.value(row), options.max_cell_width));
  };
  for (const auto row : row_plan.head) {
    push_row(row);
  }
  if (row_plan.ellipsis) {
    rows.emplace_back("...", "...");
  }
  for (const auto row : row_plan.tail) {
    push_row(row);
  }
  std::int64_t label_width = 1;
  for (const auto& row : rows) {
    label_width = std::max<std::int64_t>(label_width, static_cast<std::int64_t>(row.first.size()));
  }
  std::string out;
  for (std::size_t i = 0; i < rows.size(); ++i) {
    out += fit_line(pluss::tils::OutputEngine::format("{}  {}", left_pad(rows[i].first, label_width), rows[i].second),
                    terminal_width);
    if (i + 1 < rows.size()) {
      out.push_back('\n');
    }
  }
  if (!out.empty()) {
    out.push_back('\n');
  }
  return out + view.footer;
}

// The overview layout of a collection of named parts (a registry's fields, an axel's levels): a title, optional note
// lines, then one row per part. Each column is as wide as its widest shown cell (and no wider than a cell may be); the
// last column takes what is left of the line. Only the rows that are shown are asked for.
std::string summary_text_(const SummaryView& view, const EffectiveDisplayOptions& options, std::int64_t terminal_width) {
  const std::size_t columns = view.columns.size();
  const auto row_plan = build_row_plan(view.size, options);
  std::vector<std::vector<std::string>> rows;
  rows.reserve(row_plan.head.size() + row_plan.tail.size() + (row_plan.ellipsis ? 1 : 0));
  const auto push_row = [&](const std::int64_t index) {
    auto cells = view.row(index);
    cells.resize(columns);
    for (auto& cell : cells) {
      cell = clip_cell(std::move(cell), options.max_cell_width * 2);
    }
    rows.push_back(std::move(cells));
  };
  for (const auto index : row_plan.head) {
    push_row(index);
  }
  if (row_plan.ellipsis) {
    rows.emplace_back(columns, "...");
  }
  for (const auto index : row_plan.tail) {
    push_row(index);
  }
  std::vector<std::int64_t> widths(columns, 1);
  for (std::size_t c = 0; c < columns; ++c) {
    widths[c] = static_cast<std::int64_t>(view.columns[c].size());
    for (const auto& row : rows) {
      widths[c] = std::max<std::int64_t>(widths[c], static_cast<std::int64_t>(row[c].size()));
    }
  }
  const auto line = [&](const std::vector<std::string>& cells) {
    std::string out;
    for (std::size_t c = 0; c < columns; ++c) {
      out += c + 1 < columns ? right_pad(cells[c], widths[c]) + " | " : cells[c];
    }
    return fit_line(out, terminal_width);
  };
  const std::string heavy(static_cast<std::size_t>(std::max<std::int64_t>(1, terminal_width)), '=');
  const std::string light(static_cast<std::size_t>(std::max<std::int64_t>(1, terminal_width)), '-');
  std::string out = heavy + "\n" + fit_line(view.title, terminal_width) + "\n";
  for (const auto& note : view.notes) {
    out += fit_line(note, terminal_width) + "\n";
  }
  out += light + "\n" + line(view.columns) + "\n" + light + "\n";
  for (const auto& row : rows) {
    out += line(row) + "\n";
  }
  return out + heavy;
}

std::string render_shelf_series_like_(const Shelf& shelf,
                                       const EffectiveDisplayOptions& options,
                                       std::int64_t terminal_width) {
  const auto dtype_footer = logical_dtype(shelf);

  const auto values = display_shelf(shelf);
  std::optional<Shelf> labels = std::nullopt;
  if (detail::has_labels(shelf)) {
    labels = display_shelf(detail::display_labels(shelf));
  }

  std::vector<std::string> structured_headers;
  if (labels.has_value() && labels->named()) {
    structured_headers = split_display_token_(labels->name());
    if (structured_headers.size() <= 1) {
      structured_headers.clear();
    }
  }

  const auto row_plan = build_row_plan(values.size(), options);
  if (!structured_headers.empty()) {
    std::vector<std::vector<std::string>> rows;
    rows.reserve(row_plan.head.size() + row_plan.tail.size() + (row_plan.ellipsis ? 1 : 0));

    auto push_row = [&](std::int64_t ridx) {
      auto parts = split_display_token_(shelf_value_at(*labels, ridx));
      parts.resize(structured_headers.size());
      parts.push_back(shelf_value_at(values, ridx));
      rows.push_back(std::move(parts));
    };

    for (const auto ridx : row_plan.head) {
      push_row(ridx);
    }
    if (row_plan.ellipsis) {
      std::vector<std::string> row(structured_headers.size() + 1, "...");
      rows.push_back(std::move(row));
    }
    for (const auto ridx : row_plan.tail) {
      push_row(ridx);
    }

    std::vector<std::int64_t> widths(structured_headers.size() + 1, 1);
    for (std::size_t col = 0; col < structured_headers.size(); ++col) {
      widths[col] = std::max<std::int64_t>(1, static_cast<std::int64_t>(structured_headers[col].size()));
    }
    for (const auto& row : rows) {
      for (std::size_t col = 0; col < row.size(); ++col) {
        widths[col] = std::max<std::int64_t>(widths[col], static_cast<std::int64_t>(row[col].size()));
      }
    }

    std::string out;
    std::string header;
    for (std::size_t col = 0; col < structured_headers.size(); ++col) {
      if (col > 0) {
        header += "  ";
      }
      header += right_pad(structured_headers[col], widths[col]);
    }
    if (!header.empty()) {
      out += fit_line(header, terminal_width);
      if (!rows.empty()) {
        out.push_back('\n');
      }
    }

    for (std::size_t r = 0; r < rows.size(); ++r) {
      std::string line;
      for (std::size_t col = 0; col < structured_headers.size(); ++col) {
        if (col > 0) {
          line += "  ";
        }
        line += right_pad(rows[r][col], widths[col]);
      }
      if (!line.empty()) {
        line += "  ";
      }
      line += left_pad(rows[r].back(), widths.back());
      out += fit_line(line, terminal_width);
      if (r + 1 < rows.size()) {
        out.push_back('\n');
      }
    }
    if (!out.empty()) {
      out.push_back('\n');
    }
    if (shelf.named()) {
      out += pluss::tils::OutputEngine::format("Name: {}, ptype: {}", shelf.name(), dtype_footer);
    } else {
      out += pluss::tils::OutputEngine::format("ptype: {}", dtype_footer);
    }
    return out;
  }

  SequenceView view;
  view.size = values.size();
  view.label = [&](const std::int64_t row) {
    return labels.has_value() ? shelf_value_at(*labels, row) : pluss::tils::OutputEngine::format("{}", row);
  };
  view.value = [&](const std::int64_t row) { return shelf_value_at(values, row); };
  view.footer = shelf.named() ? pluss::tils::OutputEngine::format("Name: {}, ptype: {}", shelf.name(), dtype_footer)
                              : pluss::tils::OutputEngine::format("ptype: {}", dtype_footer);
  return sequence_text_(view, options, terminal_width);
}

bool native_webview_backend_available_() {
  return pluss::view::native_backend_available();
}

bool pluss_webview_backend_available_() {
  return pluss::view::pluss_webview_backend_available();
}

bool runtime_gui_env_available_() {
  return pluss::view::runtime_gui_environment_available();
}

DisplayBackend resolve_backend_(const DisplayOptions& options) {
  const auto configured =
      options.backend == DisplayBackend::Auto ? g_default_backend.load(std::memory_order_relaxed) : options.backend;
  if (!runtime_gui_env_available_()) {
    return DisplayBackend::Terminal;
  }

  if (configured == DisplayBackend::Auto) {
    if (pluss_webview_backend_available_()) {
      return DisplayBackend::PlussWebView;
    }
    return native_webview_backend_available_() ? DisplayBackend::NativeWebView : DisplayBackend::Terminal;
  }

  if (configured == DisplayBackend::NativeWebView) {
    return native_webview_backend_available_() ? DisplayBackend::NativeWebView : DisplayBackend::Terminal;
  }

  if (configured == DisplayBackend::PlussWebView) {
    return pluss_webview_backend_available_() ? DisplayBackend::PlussWebView : DisplayBackend::Terminal;
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

// The type face a theme reads through var(--font).
constexpr const char* kSansFont = "\"Avenir Next\", \"Inter\", \"Segoe UI\", sans-serif";

struct ThemePalette {
  DisplayTheme theme;
  const char* name;   // id, lower case (used by parse_theme / the demo)
  const char* label;  // shown in the Theme menu
  // order matches kThemeTokens
  std::array<const char*, 26> v;
  const char* heat_pos;  // "r,g,b" for positive correlations
  const char* heat_neg;  // "r,g,b" for negative correlations
  const char* font{kSansFont};
};

constexpr std::array<const char*, 26> kThemeTokens{
    "paper", "panel", "ink", "title", "muted", "line", "border", "row-line", "accent", "accent-2",
    "accent-3", "accent-3-border", "on-accent", "btn", "btn-border", "hero-a", "hero-c",
    "head-bg", "case-bg", "case-head-bg", "zebra", "hover", "success", "control-border",
    "pill-bg", "grid"};

const std::array<ThemePalette, 9> kThemes{{
    {DisplayTheme::Default, "default", "Default",
     {"#f3f6fb", "#ffffff", "#1b2d4b", "#0b1b3c", "#697789", "#c3d0e2", "#cfdae9", "#e7edf5", "#c84d58",
      "#f27b72", "#203e67", "#152b4a", "#ffffff", "#203e67", "#152b4a", "#d8e2f1", "#f8d9d9", "#dae3ee",
      "#edf1f7", "#f2c8cb", "#f7f9fc", "#e7edf5", "#2b5b80", "#b9c8d9", "#e5ebf4", "#dce4ed"},
     "41,93,138", "200,77,88"},
    {DisplayTheme::Ember, "ember", "Ember",
     {"#f1e3da", "#fbf6f2", "#3a241e", "#2a1917", "#8d5646", "#e3cabd", "#e3cdc1", "#f0e0d6", "#b5412a",
      "#e8742a", "#77493b", "#4a2a22", "#fbe4d9", "#6b3a2e", "#46261e", "#eab09a", "#f6c9a8", "#f0d3c4",
      "#f6e4d9", "#e6b9a2", "#fbf1ea", "#f6dccd", "#77493b", "#d9b9a8", "#f0d3c4", "#f0e0d6"},
     "232,116,42", "90,61,56"},
    {DisplayTheme::Nardo, "nardo", "Nardo",
     {"#dcdfe2", "#f4f5f6", "#1c1f23", "#111316", "#6a7078", "#c3c7cc", "#c3c7cc", "#dfe2e5", "#d1122b",
      "#ff5a36", "#d1122b", "#9d0c20", "#ffffff", "#d1122b", "#9d0c20", "#cfd3d8", "#eceef0", "#d3d7db",
      "#e6e8eb", "#c4c9ce", "#eef0f2", "#e3e6e9", "#d1122b", "#b5bac0", "#d3d7db", "#dfe2e5"},
     "209,18,43", "58,64,72"},
    {DisplayTheme::Brooklands, "brooklands", "Brooklands",
     {"#ece6d2", "#fbf8ed", "#14261b", "#0b3b25", "#6b7a62", "#cfc7a8", "#cfc7a8", "#e6dfc4", "#0b5d3b",
      "#c9a227", "#0b5d3b", "#06402a", "#f0d77a", "#0b5d3b", "#c9a227", "#bfd8c4", "#f2e6b6", "#d7e6d4",
      "#e8f0e4", "#b9d3be", "#f5f2e2", "#e7eed9", "#c9a227", "#b8b08c", "#d7e6d4", "#e6dfc4"},
     "11,93,59", "192,128,16"},
    {DisplayTheme::Gulf, "gulf", "Gulf",
     {"#cfe7f3", "#f7fcff", "#0e2a3a", "#0a2433", "#547489", "#a9d0e4", "#a9d0e4", "#d2e8f3", "#ff6a13",
      "#ff6a13", "#ff6a13", "#d9500a", "#ffffff", "#ff6a13", "#d9500a", "#8ccbea", "#ffe3cf", "#bfe0f0",
      "#dcedf7", "#a7d3ea", "#eef7fc", "#d6ecf8", "#ff6a13", "#8fc2dc", "#bfe0f0", "#d2e8f3"},
     "255,106,19", "40,130,180"},
    {DisplayTheme::Obsidian, "obsidian", "Obsidian",
     {"#060607", "#0c0d10", "#ebe6df", "#f7e8d8", "#c9a07a", "#1f2026", "#2a2b32", "#1b1c21", "#d08043",
      "#f0a066", "#b87333", "#8a5226", "#14100c", "#b87333", "#d98a4e", "#14100c", "#2a1a0e", "#3f2615",
      "#2f1d10", "#5a3319", "#121317", "#2a1c10", "#d08043", "#34353d", "#1a1b20", "#24252b"},
     "208,128,67", "6,6,7"},
    {DisplayTheme::Indigo, "indigo", "Indigo",
     {"#f6f3ff", "#fdfcff", "#2b2149", "#19152f", "#756988", "#ded6f1", "#d4c9eb", "#ebe5f8", "#7052b4",
      "#cab4ff", "#4b347f", "#392664", "#ffffff", "#4b347f", "#392664", "#d7c8f6", "#e8dcff", "#e5dcf8",
      "#f0eafd", "#d3c0f1", "#faf8ff", "#ede5fb", "#5d4aab", "#cdbfe5", "#e8defa", "#e5dcf7"},
     "112,82,180", "147,88,130"},
    {DisplayTheme::Paperwhite, "paperwhite", "Paperwhite",
     {"#efeee9", "#fbfbf8", "#1d1c1a", "#111110", "#6f6d66", "#dedcd4", "#d4d1c7", "#e9e7e0", "#3b3a36",
      "#76736a", "#2b2a27", "#2b2a27", "#fbfbf8", "#3b3a36", "#2b2a27", "#f4f3ee", "#ebe9e1",
      "#fbfbf8", "#f6f5f0", "#fbfbf8", "#fbfbf8", "#efede6", "#9a968a", "#cfccc1",
      "#efede6", "#e4e2da"},
     "59,58,54", "163,42,29"},
    {DisplayTheme::Native, "native", "Native",
     {"#e1e9f8", "#fafcff", "#14213d", "#0f1b4d", "#5a6b8f", "#c5d3ea", "#c5d3ea", "#dbe5f4", "#0fa3a3",
      "#6a5cff", "#2b3a9e", "#1c2a78", "#ffffff", "#2b3a9e", "#1c2a78", "#b4c8fb", "#e9f6fb", "#cfdcf8",
      "#eef3fc", "#bfeee9", "#f3f7fd", "#dce9fb", "#0fa3a3", "#a9bbdc", "#cfdcf8", "#dbe5f4"},
     "15,163,163", "106,92,255"},
}};

DisplayTheme resolve_theme_(const DisplayOptions& options) {
  if (options.theme != DisplayTheme::Auto) {
    return options.theme;
  }
  return g_default_theme.load(std::memory_order_relaxed);
}

const ThemePalette& palette_for_(const DisplayTheme theme) {
  for (const auto& palette : kThemes) {
    if (palette.theme == theme) {
      return palette;
    }
  }
  return kThemes.front();
}

// A missing cell (NULL) is the one thing the table marks: its text is bold and red, the red of the theme.
// NaN and infinity are values and look like any other value; the View > Filter menu finds them.
constexpr const char* kDefaultNullInk = "#c0262d";
constexpr const char* kEmberNullInk = "#c4281c";
constexpr const char* kNardoNullInk = "#c8102e";
constexpr const char* kBrooklandsNullInk = "#b3261e";
constexpr const char* kGulfNullInk = "#c62828";
constexpr const char* kObsidianNullInk = "#ff6b5e";
constexpr const char* kIndigoNullInk = "#c9184a";
constexpr const char* kNativeNullInk = "#d63031";
constexpr const char* kPaperwhiteNullInk = "#a32a1d";

const char* null_ink_for_(const DisplayTheme theme) {
  switch (theme) {
    case DisplayTheme::Ember: return kEmberNullInk;
    case DisplayTheme::Nardo: return kNardoNullInk;
    case DisplayTheme::Brooklands: return kBrooklandsNullInk;
    case DisplayTheme::Gulf: return kGulfNullInk;
    case DisplayTheme::Obsidian: return kObsidianNullInk;
    case DisplayTheme::Indigo: return kIndigoNullInk;
    case DisplayTheme::Native: return kNativeNullInk;
    case DisplayTheme::Paperwhite: return kPaperwhiteNullInk;
    case DisplayTheme::Default:
    case DisplayTheme::Auto:
      break;
  }
  return kDefaultNullInk;
}

// The `:root { --token: value; ... }` block every other rule reads through var().
std::string theme_root_css_(const DisplayOptions& options) {
  const auto& palette = palette_for_(resolve_theme_(options));
  std::string css = ":root {";
  for (std::size_t i = 0; i < kThemeTokens.size(); ++i) {
    css += pluss::tils::OutputEngine::format(" --{}: {};", kThemeTokens[i], palette.v[i]);
  }
  css += pluss::tils::OutputEngine::format(" --miss-ink: {}; --font: {};", null_ink_for_(resolve_theme_(options)), palette.font);
  css += " --shadow: none; }";
  return css;
}

std::string html_shell_(std::string_view title,
                        std::string_view subtitle,
                        std::string_view body,
                        const DisplayOptions& options,
                        std::string_view header_extra = {}) {
  const auto sortable = options.html_sortable ? "true" : "false";
  const auto search = options.html_search ? "true" : "false";
  const std::string responsive_css = R"CSS(
    @media (max-width: 720px) {
      .shell {
        padding: 10px;
      }
      .hero {
        padding: 18px 16px 14px;
      }
      .content {
        padding: 0 10px 10px;
      }
      .tools {
        flex-wrap: wrap;
      }
      .tools input {
        min-width: 0;
        width: auto;
        flex: 1 1 100%;
      }
      .tools button {
        min-height: 44px;
      }
    }
)CSS";

  return pluss::tils::OutputEngine::format(R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>{0}</title>
  <style>
    {6}
    * {{ box-sizing: border-box; }}
    body {{
      margin: 0;
      color: var(--ink);
      font-family: var(--font);
      background: var(--paper);
    }}
    .shell {{
      min-height: 100vh;
      padding: 14px;
      background: var(--paper);
    }}
    .frame {{
      max-width: 1560px;
      margin: 0 auto;
      background: var(--panel);
      border: 1px solid var(--border);
      border-radius: 8px;
      overflow: hidden;
      box-shadow: none;
    }}
    .hero {{
      padding: 14px 16px;
      border-bottom: 1px solid var(--border);
      border-top: 5px solid var(--accent);
      background: linear-gradient(90deg, var(--hero-a), var(--hero-c));
      display: flex;
      gap: 14px;
      align-items: center;
      justify-content: space-between;
      flex-wrap: wrap;
    }}
    .hero.visiting {{
      flex-wrap: nowrap;  /* the range boxes share the row with the search box, so the header keeps its height */
    }}
    .hero.visiting p {{
      white-space: nowrap;  /* the instruction that replaces the subtitle stays on one line */
    }}
    .hero.visiting .tools {{
      margin-left: 0;  /* the header spreads title, range boxes and search evenly, so the boxes sit in the middle */
    }}
    .hero h1 {{
      margin: 0;
      font-family: var(--font);
      font-size: 1.35rem;
      font-weight: 700;
      color: var(--title);
    }}
    .hero p {{
      margin: 3px 0 0;
      color: var(--muted);
      font-size: 0.84rem;
    }}
    .tools {{
      display: flex;
      gap: 1px;
      align-items: center;
      justify-content: flex-end;
      margin-left: auto;
      flex-wrap: nowrap;
    }}
    .tools input, .visit-inline input {{
      margin: 0;
      min-width: 0;
      width: 17em;  /* explicit, so the row's width is known before it is laid out */
      height: 40px;
      min-height: 40px;
      padding: 0 12px;
      border-radius: 7px;
      border: 1px solid var(--control-border);
      background: var(--panel);
      color: var(--ink);
      font: inherit;
      line-height: 38px;
      appearance: none;
      -webkit-appearance: none;
    }}
    .visit-inline {{
      display: flex;
      gap: 0.5em;
      align-items: center;
    }}
    .visit-inline .visit-box {{
      min-width: 0;
      width: 5.5em;  /* room for five digits */
      padding: 0 0.5em;
      text-align: center;
    }}
    .tools button, .visit-inline button {{
      margin: 0;
      height: 40px;
      min-height: 40px;
      min-width: 88px;
      padding: 0 12px;
      border-radius: 7px;
      border: 1px solid var(--btn-border);
      background: var(--btn);
      color: var(--on-accent);
      font: 600 0.82rem/1 "Avenir Next", "Inter", "Segoe UI", sans-serif;
      line-height: 38px;
      appearance: none;
      -webkit-appearance: none;
      cursor: pointer;
    }}
    .tools button.reset {{
      background: var(--btn);
      border-color: var(--btn-border);
      color: var(--on-accent);
    }}
    .tools button {{
      text-align: center;
    }}
    .tools input {{
      text-align: left;
    }}
    .content {{
      padding: 0 12px 12px;
      background: var(--panel);
    }}
    .summary-grid {{
      display: grid;
      gap: 8px;
      grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
      padding: 10px 2px 4px;
    }}
    .summary-card {{
      padding: 9px 11px;
      border-radius: 7px;
      border: 1px solid var(--border);
      border-left: 4px solid var(--accent-3);
      background: var(--panel);
    }}
    .summary-card .label {{
      color: var(--muted);
      font-size: 0.68rem;
      text-transform: uppercase;
    }}
    .summary-card .value {{
      margin-top: 3px;
      font-size: 0.98rem;
      font-weight: 700;
    }}
    .table-wrap {{
      margin-top: 4px;
      border: 1px solid var(--border);
      border-radius: 7px;
      background: var(--panel);
      overflow: hidden;
    }}
    .top-bar {{
      height: 0.3em;
      background: var(--accent-3);
    }}
    .table-scroll {{
      overflow: auto;
      max-height: calc(100vh - 300px);
      border-radius: inherit;
    }}
    table {{
      border-color: var(--border);
      width: 100%;
      max-height: 28.6em;  /* about 400px at the table font; tracks the font size */
      margin: 0;
      border-collapse: collapse;
      border-radius: 6px;
      overflow: hidden;
      font-size: 0.86rem;
    }}
    thead th {{
      font-weight: 700;
      position: sticky;
      top: 0;
      z-index: 2;
      background: var(--head-bg);
      border-bottom: 1px solid var(--line);
      text-align: center;
      white-space: nowrap;
    }}
    th, td {{
      padding: 8px 10px;
      border-bottom: 1px solid var(--row-line);
      border-color: var(--line);
      vertical-align: top;
    }}
    td {{
      background: var(--panel);
    }}
    tr.even td {{
      background: var(--zebra);
    }}
    th.sortable {{
      cursor: pointer;
      user-select: none;
    }}
    th.sortable::after {{
      content: "  ↕";
      color: var(--muted);
      font-size: 0.85em;
    }}
    .sticky-left {{
      position: sticky;
      left: 0;
      z-index: 1;
      background: var(--case-bg);
      border-color: var(--success);
      border-right: 2px solid var(--success);
    }}
    thead .sticky-left {{
      z-index: 3;
      background: var(--case-head-bg);
    }}
    .num {{
      text-align: left;
      font-variant-numeric: tabular-nums;
    }}
    thead th.num {{
      text-align: center;
    }}
    .muted {{
      color: var(--muted);
    }}
    tbody tr td.missing {{
      color: var(--miss-ink);
      font-weight: 800;
    }}
    .hidden {{
      display: none;
    }}
    table.fixed {{
      table-layout: fixed;
    }}
    table.fixed th, table.fixed td {{
      overflow-wrap: anywhere;
      border-right: 1px solid var(--line);  /* column dividers in a browser view */
    }}
    table.fixed .sticky-left {{
      border-right: 2px solid var(--success);  /* the heavier divider after the label column */
    }}
    table.fixed thead th {{
      white-space: normal;
    }}
    .table-wrap.vwrap {{
      position: relative;
    }}
    .vbadge {{
      position: absolute;
      right: 18px;
      bottom: 12px;
      padding: 3px 11px;
      border-radius: 999px;
      background: var(--ink);
      color: var(--panel);
      font-size: 0.74rem;
      font-weight: 600;
      opacity: 0.88;
      pointer-events: none;
      z-index: 5;
    }}
    table[data-virtual] {{
      height: auto;
      overflow: visible;  /* the scroll area, not the table, scrolls: keeps the header sticky */
    }}
    table[data-virtual] tbody td {{
      height: 38px;
      padding-top: 0;
      padding-bottom: 0;
      box-sizing: border-box;
      white-space: nowrap;
      overflow: hidden;
      text-overflow: ellipsis;
    }}
    table[data-virtual] tbody tr.vspace td {{
      height: auto;
      padding: 0;
      border: 0;
      background: transparent;
    }}
    .pill {{
      display: inline-block;
      padding: 4px 7px;
      border-radius: 6px;
      background: var(--pill-bg);
      color: var(--accent);
      font-size: 0.76rem;
      font-weight: 600;
    }}
    .section-label {{
      display: inline-block;
      margin: 10px 0 8px;
      color: var(--muted);
      font-size: 0.78rem;
      text-transform: uppercase;
    }}
    .atlas-nav {{
      display: flex;
      flex-wrap: wrap;
      gap: 7px;
      overflow: auto;
      padding: 10px 6px 6px;
      margin-bottom: 8px;
    }}
    .atlas-frame-tabs {{
      border-bottom: 1px solid var(--row-line);
      padding-bottom: 12px;
    }}
    .atlas-current-frame {{
      min-width: 150px;
      padding: 8px 12px;
      border: 1px solid var(--control-border);
      border-radius: 7px;
      background: var(--case-bg);
      color: var(--title);
      text-align: center;
      font-weight: 700;
      white-space: nowrap;
    }}
    .atlas-tab {{
      padding: 7px 10px;
      border: 1px solid var(--control-border);
      border-radius: 7px;
      background: var(--panel);
      color: var(--ink);
      cursor: pointer;
      white-space: nowrap;
      font: inherit;
    }}
    .atlas-tab.active {{
      background: var(--accent-3);
      border-color: var(--accent-3-border);
      color: var(--on-accent);
      font-weight: 600;
    }}
    .view-picker {{
      padding: 10px 2px 8px;
    }}
    .view-toggle {{
      display: inline-block;
      width: 220px;
      margin: 0;
      padding: 8px 14px;
      border: 1px solid var(--control-border);
      border-radius: 7px;
      background: var(--panel);
      color: var(--ink);
      cursor: pointer;
      text-align: left;
      font: inherit;
    }}
    .view-menu {{
      display: none;
      width: 220px;
      margin: 4px 0 0 6px;
      padding: 4px;
      border: 1px solid var(--control-border);
      border-radius: 7px;
      background: var(--panel);
    }}
    .view-menu.open {{
      display: block;
    }}
    .tab {{
      display: block;
      width: 100%;
      padding: 7px 10px;
      border: 0;
      border-radius: 5px;
      background: var(--panel);
      color: var(--ink);
      cursor: pointer;
      text-align: left;
      font: inherit;
    }}
    .tab.active {{
      background: var(--accent-3);
      color: var(--on-accent);
      font-weight: 600;
    }}
    .tab-panel {{
      display: none;
    }}
    .tab-panel.active {{
      display: block;
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
      font-family: var(--font);
      font-size: 1rem;
      font-weight: 700;
    }}
    .atlas-panel .table-wrap table {{
      height: 300px;
    }}
    {5}
  </style>
</head>
<body>
  <div class="shell">
    <div class="frame">
      <header class="hero{8}">
        <div class="hero-title">
          <h1>{0}</h1>
          <p>{1}</p>
        </div>
        {7}
        <div class="tools">
          <input id="table-search" type="search" placeholder="Search visible rows" />
          <button id="reset-table-view" class="reset" type="button">Reset</button>
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
    const table = document.getElementById('plussmic-table');
    const detailTables = document.querySelectorAll('table[data-filterable="true"]');
    const searchInput = document.getElementById('table-search');
    const resetButton = document.getElementById('reset-table-view');

    if (!enableSearch) {{
      searchInput.classList.add('hidden');
    }}

    // Row text and sort keys are read from the DOM once and cached; filtering and sorting then
    // work on those arrays, so neither forces a layout per row (innerText did) and a reorder is
    // a single batch.
    const collator = new Intl.Collator(undefined, {{ numeric: true, sensitivity: 'base' }});
    const tableState = new WeakMap();
    function stateOf(tbl) {{
      let st = tableState.get(tbl);
      if (!st) {{
        const tbody = tbl.tBodies[0];
        const rows = tbody ? Array.from(tbody.rows) : [];
        st = {{ tbody, rows, original: rows.slice(), texts: null, hidden: new Uint8Array(rows.length), keys: new Map() }};
        tableState.set(tbl, st);
      }}
      return st;
    }}

    function filterTables(query) {{
      const needle = query.trim().toLowerCase();
      detailTables.forEach((tbl) => {{
        if (tbl._vt) {{
          virtualFilter(tbl._vt, needle);
          return;
        }}
        if (tbl.offsetParent === null) return;
        const st = stateOf(tbl);
        if (!st.tbody) return;
        if (needle.length > 0 && !st.texts) st.texts = st.rows.map((r) => r.textContent.toLowerCase());
        for (let i = 0; i < st.rows.length; i++) {{
          const hide = needle.length > 0 && st.texts[i].indexOf(needle) === -1 ? 1 : 0;
          if (st.hidden[i] !== hide) {{
            st.hidden[i] = hide;
            st.rows[i].classList.toggle('hidden', hide === 1);
          }}
        }}
      }});
    }}

    // Per column: category (0 number incl. +-inf, 1 text, 2 missing NULL, 3 NaN), numeric value and text, per row.
    function buildKeys(n, rawAt, numeric) {{
      const cat = new Uint8Array(n);
      const num = new Float64Array(n);
      const txt = new Array(n);
      for (let i = 0; i < n; i++) {{
        const trimmed = rawAt(i).trim();
        txt[i] = trimmed;
        if (trimmed === '' || trimmed === 'NULL') {{
          cat[i] = 2;
        }} else if (numeric && trimmed.toLowerCase() === 'nan') {{
          cat[i] = 3;  // NaN: not a number, after every number and just before NULL
        }} else if (numeric && (trimmed.toLowerCase() === 'inf' || trimmed.toLowerCase() === '-inf')) {{
          cat[i] = 0;
          num[i] = trimmed.toLowerCase() === 'inf' ? Infinity : -Infinity;
        }} else {{
          const v = numeric ? Number(trimmed) : NaN;
          if (numeric && Number.isFinite(v)) {{
            cat[i] = 0;
            num[i] = v;
          }} else {{
            cat[i] = 1;
          }}
        }}
      }}
      return {{ cat, num, txt }};
    }}

    // Row indices in sorted order: numbers (-inf first, +inf last), then text, then NaN, then NULL (always last).
    function sortedOrder(k, asc) {{
      const order = Array.from({{ length: k.cat.length }}, (_, i) => i);
      order.sort((a, b) => {{
        const ca = k.cat[a];
        const cb = k.cat[b];
        if (ca >= 2 || cb >= 2) return ca !== cb ? ca - cb : a - b;
        if (ca !== cb) return asc ? ca - cb : cb - ca;
        const d = ca === 0 ? (k.num[a] < k.num[b] ? -1 : (k.num[a] > k.num[b] ? 1 : 0)) : collator.compare(k.txt[a], k.txt[b]);
        if (d === 0) return a - b;
        return asc ? d : -d;
      }});
      return order;
    }}

    function sortTable(tbl, columnIndex, numeric) {{
      const current = tbl.dataset.sortDir === 'asc' ? 'asc' : 'desc';
      const next = current === 'asc' ? 'desc' : 'asc';
      tbl.dataset.sortDir = next;
      if (tbl._vt) {{
        virtualSort(tbl._vt, columnIndex, numeric, next === 'asc');
        return;
      }}
      const st = stateOf(tbl);
      if (!st.tbody) return;
      let k = st.keys.get(columnIndex);
      if (!k) {{
        k = buildKeys(st.rows.length, (i) => {{
          const cell = st.rows[i].cells[columnIndex];
          return cell ? (cell.dataset.sortValue !== undefined ? cell.dataset.sortValue : cell.textContent) : '';
        }}, numeric);
        st.keys.set(columnIndex, k);
      }}
      const order = sortedOrder(k, next === 'asc');
      const frag = document.createDocumentFragment();
      for (let i = 0; i < order.length; i++) frag.appendChild(st.rows[order[i]]);
      st.tbody.appendChild(frag);
    }}

    // ---- long tables: only the visible window of rows exists in the page ----
    const VROW_H = 38;
    const VBUFFER = 8;
    function escapeHtml(s) {{
      return s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
    }}
    function setupVirtual(tbl) {{
      const dataEl = document.getElementById(tbl.id + '-data');
      if (!dataEl) return;
      const data = JSON.parse(dataEl.textContent);
      const n = data.rows.length;
      const order = new Uint32Array(n);
      for (let i = 0; i < n; i++) order[i] = i;
      const vt = {{ tbl, data, n, cols: data.num.length, tbody: tbl.tBodies[0], scroller: tbl.closest('.table-scroll'),
                  order, view: order, texts: null, keys: new Map(), needle: '', raf: 0 }};
      vt.badge = document.getElementById(tbl.id + '-badge');
      tbl._vt = vt;
      vt.scroller.addEventListener('scroll', () => {{
        if (!vt.raf) vt.raf = requestAnimationFrame(() => {{ vt.raf = 0; renderVirtual(vt); }});
      }});
      window.addEventListener('resize', () => renderVirtual(vt));
      renderVirtual(vt);
      requestAnimationFrame(() => renderVirtual(vt));  // once the scroll area has its real height
    }}
    function renderVirtual(vt) {{
      const total = vt.view.length;
      const top = vt.scroller.scrollTop;
      // Before layout settles the scroll area can still be tiny, so draw enough rows to fill the
      // window; the readout uses the real height once there is one.
      const clientH = vt.scroller.clientHeight;
      const height = Math.max(clientH, window.innerHeight);
      const first = Math.max(0, Math.floor(top / VROW_H) - VBUFFER);
      const last = Math.min(total, Math.ceil((top + height) / VROW_H) + VBUFFER);
      const spacer = (rows) => '<tr class="vspace"><td colspan="' + vt.cols + '" style="height:' + (rows * VROW_H) + 'px"></td></tr>';
      const parts = [spacer(first)];
      for (let k = first; k < last; k++) {{
        const r = vt.data.rows[vt.view[k]];
        let h = k % 2 === 1 ? '<tr class="even">' : '<tr>';
        for (let c = 0; c < vt.cols; c++) {{
          const text = r[c];
          let cls = vt.data.sticky && c === 0 ? 'sticky-left' : '';
          if (vt.data.num[c]) cls += (cls ? ' ' : '') + 'num';
          if (text === 'NULL') cls += (cls ? ' ' : '') + 'missing';
          else if (text === 'NAN' || text === 'INF' || text === '-INF') cls += (cls ? ' ' : '') + 'nonfinite ' + (text === 'NAN' ? 'nan' : 'inf');
          h += '<td' + (cls ? ' class="' + cls + '"' : '') + '>' + escapeHtml(text) + '</td>';
        }}
        parts.push(h + '</tr>');
      }}
      parts.push(spacer(total - last));
      vt.tbody.innerHTML = parts.join('');
      if (vt.badge) {{
        const topRow = Math.min(total, Math.floor(top / VROW_H) + 1);
        const bottomRow = Math.min(total, topRow + Math.max(1, Math.floor((clientH || height) / VROW_H)) - 1);
        vt.badge.textContent = total === 0 ? 'No matching rows'
          : 'Rows ' + topRow.toLocaleString() + '\u2013' + bottomRow.toLocaleString() + ' of ' + total.toLocaleString()
            + (total !== vt.n ? ' (filtered from ' + vt.n.toLocaleString() + ')' : '');
      }}
    }}
    function applyVirtualView(vt) {{
      if (vt.needle.length > 0) {{
        if (!vt.texts) vt.texts = vt.data.rows.map((r) => r.join(' ').toLowerCase());
        vt.view = vt.order.filter((i) => vt.texts[i].indexOf(vt.needle) !== -1);
      }} else {{
        vt.view = vt.order;
      }}
      vt.scroller.scrollTop = 0;
      renderVirtual(vt);
    }}
    function virtualSort(vt, columnIndex, numeric, asc) {{
      let k = vt.keys.get(columnIndex);
      if (!k) {{
        k = buildKeys(vt.n, (i) => vt.data.rows[i][columnIndex], numeric);
        vt.keys.set(columnIndex, k);
      }}
      vt.order = Uint32Array.from(sortedOrder(k, asc));
      applyVirtualView(vt);
    }}
    function virtualFilter(vt, needle) {{
      vt.needle = needle;
      applyVirtualView(vt);
    }}
    function virtualReset(vt) {{
      for (let i = 0; i < vt.n; i++) vt.order[i] = i;
      vt.needle = '';
      applyVirtualView(vt);
    }}
    document.querySelectorAll('table[data-virtual="true"]').forEach(setupVirtual);

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

    resetButton.addEventListener('click', () => {{
      searchInput.value = '';
      filterTables('');
      detailTables.forEach((tbl) => {{
        if (tbl._vt) {{
          virtualReset(tbl._vt);
          delete tbl.dataset.sortDir;
          return;
        }}
        const st = stateOf(tbl);
        if (!st.tbody) return;
        const frag = document.createDocumentFragment();
        st.original.forEach((row) => frag.appendChild(row));
        st.tbody.appendChild(frag);
        delete tbl.dataset.sortDir;
      }});
    }});
    document.querySelectorAll('button.view-toggle').forEach((toggle) => {{
      toggle.addEventListener('click', () => {{
        document.getElementById(toggle.dataset.dropdown).classList.toggle('open');
      }});
    }});
    // The page's views, and the function that switches between them. A host that offers its own
    // View control (Apple's window has one in its toolbar) reads plussViews, removes the in-page
    // picker, and calls plussSelectView.
    const plussViews = Array.from(document.querySelectorAll('button.tab')).map((b) => ({{
      id: b.id, label: b.textContent.trim(), group: b.dataset.tabGroup, target: b.dataset.tabTarget }}));
    function plussSelectView(tabId) {{
      const view = plussViews.find((v) => v.id === tabId);
      if (!view) return;
      document.querySelectorAll('button.tab[data-tab-group="' + view.group + '"]').forEach((b) => b.classList.toggle('active', b.id === tabId));
      document.querySelectorAll('.tab-panel[data-tab-group="' + view.group + '"]').forEach((p) => p.classList.toggle('active', p.id === view.target));
      document.querySelectorAll('table[data-virtual="true"]').forEach((t) => t._vt && renderVirtual(t._vt));
      const label = document.getElementById('view-current');
      if (label) label.textContent = view.label;
    }}
    window.plussViews = plussViews;
    window.plussSelectView = plussSelectView;
    document.querySelectorAll('button.tab').forEach((btn) => {{
      btn.addEventListener('click', () => {{
        plussSelectView(btn.id);
        const menu = btn.closest('.view-menu');
        if (menu) menu.classList.remove('open');
      }});
    }});
    const atlasPanels = Array.from(document.querySelectorAll('.atlas-panel'));
    const atlasTabs = Array.from(document.querySelectorAll('.atlas-frame-tabs .atlas-tab'));
    const atlasAll = document.getElementById('atlas-show-all');
    const atlasStatus = document.getElementById('atlas-current-frame');
    function showAtlasFrame(index) {{
      if (atlasPanels.length === 0) return;
      const wrapped = ((index % atlasPanels.length) + atlasPanels.length) % atlasPanels.length;
      atlasPanels.forEach((panel, panelIndex) => panel.classList.toggle('active', panelIndex === wrapped));
      atlasTabs.forEach((tab, tabIndex) => {{
        const selected = tabIndex === wrapped;
        tab.classList.toggle('active', selected);
        tab.setAttribute('aria-pressed', String(selected));
      }});
      atlasAll?.classList.remove('active');
      atlasAll?.setAttribute('aria-pressed', 'false');
      if (atlasStatus) atlasStatus.textContent =
        atlasTabs[wrapped].textContent + ' (' + (wrapped + 1) + ' of ' + atlasPanels.length + ')';
      if (enableSearch) filterTables(searchInput.value || '');
    }}
    function showAllAtlasFrames() {{
      atlasPanels.forEach((panel) => panel.classList.add('active'));
      atlasTabs.forEach((tab) => {{
        tab.classList.remove('active');
        tab.setAttribute('aria-pressed', 'false');
      }});
      atlasAll?.classList.add('active');
      atlasAll?.setAttribute('aria-pressed', 'true');
      if (atlasStatus) atlasStatus.textContent = 'All ' + atlasPanels.length + ' frames';
      if (enableSearch) filterTables(searchInput.value || '');
    }}
    atlasTabs.forEach((tab, index) =>
      tab.addEventListener('click', () => showAtlasFrame(index)));
    atlasAll?.addEventListener('click', showAllAtlasFrames);
    document.getElementById('atlas-first')?.addEventListener('click', () => showAtlasFrame(0));
    document.getElementById('atlas-last')?.addEventListener('click', () => showAtlasFrame(atlasPanels.length - 1));
    document.getElementById('atlas-previous')?.addEventListener('click', () => {{
      const active = atlasAll?.classList.contains('active') ? 0 :
        atlasPanels.findIndex((panel) => panel.classList.contains('active'));
      showAtlasFrame((active < 0 ? 0 : active) - 1);
    }});
    document.getElementById('atlas-next')?.addEventListener('click', () => {{
      const active = atlasAll?.classList.contains('active') ? -1 :
        atlasPanels.findIndex((panel) => panel.classList.contains('active'));
      showAtlasFrame((active < 0 ? -1 : active) + 1);
    }});
  </script>
</body>
</html>)HTML",
                     escape_html_(title),
                     escape_html_(subtitle),
                     body,
                     sortable,
                     search,
                     responsive_css,
                     theme_root_css_(options),
                     header_extra,
                     header_extra.empty() ? "" : " visiting");
}

std::string render_html_pre_(std::string_view title,
                             std::string_view subtitle,
                             const std::string& content,
                             const DisplayOptions& options) {
  const auto body = pluss::tils::OutputEngine::format(
      R"HTML(<div class="table-wrap"><div class="table-scroll"><pre style="margin:0; padding:16px; font-family:'IBM Plex Mono', 'SFMono-Regular', monospace; white-space:pre-wrap;">{}</pre></div></div>)HTML",
      escape_html_(content));
  return html_shell_(title, subtitle, body, options);
}

struct HtmlBodyStyle {
  std::string_view corner_label{"case"};  // header text of the case/row-label column
  bool show_cards{true};                  // Cases / Features / Case Axis cards above the table
  bool blank_nulls{false};                // render null cells empty instead of "NULL"
  bool heatmap{false};                    // tint numeric cells by value in [-1, 1] (correlations)
  bool format_numbers{false};             // apply DisplayOptions::number_format to numeric cells
  bool format_numeric_text{false};        // transpose() stores summary measurements as text
  bool virtual_rows{false};               // long frame in a browser view: rows are drawn by script, a window at a time
  // Columns that hold counts (shown as whole numbers): decimals and scientific notation
  // leave them alone, thousands separators still apply.
  std::vector<std::string> count_columns{};
};

// Background tint for a value in [-1, 1]: teal for positive, red for negative.
std::string heat_style_(const double v, const ThemePalette& palette) {
  if (!std::isfinite(v)) {
    return {};
  }
  const double a = std::min(1.0, std::abs(v));
  const int alpha = static_cast<int>(std::lround(a * 150.0));
  return pluss::tils::OutputEngine::format(" style=\"background-color: rgba({}, {});\"",
                                           v >= 0.0 ? palette.heat_pos : palette.heat_neg, alpha / 255.0);
}

// Rewrites one numeric cell for `format`. Applies to every numeric column (integers too);
// `count_like` cells stay whole numbers. Text that is not a plain number is left alone.
std::string format_number_cell_(const std::string& text, const bool count_like, const NumberFormat format) {
  if (format == NumberFormat::Default || text.empty() || text == "NULL") {
    return text;
  }
  char* end = nullptr;
  const double v = std::strtod(text.c_str(), &end);
  if (end == text.c_str() || *end != '\0' || !std::isfinite(v)) {
    return text;
  }
  char buf[64];
  const auto group = [](std::string digits) {
    const bool negative = !digits.empty() && digits[0] == '-';
    if (negative) {
      digits.erase(0, 1);
    }
    const auto dot = digits.find('.');
    std::string integer = digits.substr(0, dot);
    const std::string fraction = dot == std::string::npos ? std::string{} : digits.substr(dot);
    for (int i = static_cast<int>(integer.size()) - 3; i > 0; i -= 3) {
      integer.insert(static_cast<std::size_t>(i), ",");
    }
    return (negative ? "-" : "") + integer + fraction;
  };
  switch (format) {
    case NumberFormat::Decimals0:
    case NumberFormat::Decimals1:
    case NumberFormat::Decimals2:
    case NumberFormat::Decimals3:
    case NumberFormat::Decimals4: {
      if (count_like) {
        return text;
      }
      const int decimals = static_cast<int>(format) - static_cast<int>(NumberFormat::Decimals0);
      std::snprintf(buf, sizeof(buf), "%.*f", decimals, v);
      return buf;
    }
    case NumberFormat::Thousands:
      std::snprintf(buf, sizeof(buf), count_like ? "%.0f" : "%.2f", v);
      return group(buf);
    case NumberFormat::Scientific:
      if (count_like) {
        return text;
      }
      std::snprintf(buf, sizeof(buf), "%.3e", v);
      return buf;
    case NumberFormat::Default:
      break;
  }
  return text;
}

// Escapes `text` for HTML and, for a browser view, adds a soft break (<wbr>) after _ - . / so a
// long snake_case name can wrap at its separators instead of overflowing.
std::string escape_html_breakable_(const std::string_view text) {
  std::string out;
  out.reserve(text.size() + 16);
  for (std::size_t i = 0; i < text.size(); ++i) {
    out += escape_html_(text.substr(i, 1));
    const char ch = text[i];
    if ((ch == '_' || ch == '-' || ch == '.' || ch == '/') && i + 1 < text.size()) {
      out += "<wbr>";
    }
  }
  return out;
}

// Appends `text` as a JSON string. '<' is escaped so the payload can sit in a <script> element.
void append_json_string_(std::string& out, const std::string_view text) {
  out.push_back('"');
  for (const char ch : text) {
    switch (ch) {
      case '"':
        out += "\\\"";
        break;
      case '\\':
        out += "\\\\";
        break;
      case '<':
        out += "\\u003c";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        if (static_cast<unsigned char>(ch) < 0x20) {
          char buf[8];
          std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned>(static_cast<unsigned char>(ch)));
          out += buf;
        } else {
          out.push_back(ch);
        }
    }
  }
  out.push_back('"');
}

// The frame around every table, short or long: the accent bar on top, then the scrolling area. One place, so a
// long table and a short one cannot drift apart; the only thing a long table changes is where its rows come from.
std::string table_frame_open_(const bool virtual_rows = false) {
  return virtual_rows ? "<div class=\"table-wrap vwrap\"><div class=\"top-bar\"></div><div class=\"table-scroll\">"
                      : "<div class=\"table-wrap\"><div class=\"top-bar\"></div><div class=\"table-scroll\">";
}

constexpr std::string_view kTableFrameClose = "</div></div>";

std::string render_html_table_body_(const DataFrame& frame,
                                    const DisplayOptions& options,
                                    std::string_view table_id,
                                    const HtmlBodyStyle& style = {}) {
  const auto corner_label = style.corner_label;
  const auto show_cards = style.show_cards;
  const auto cols = dataframe_columns_(frame);
  const auto display_cases = display_shelf(frame.labels().values());
  std::vector<Shelf> display_cols;
  display_cols.reserve(cols.size());
  for (const auto& col : cols) {
    display_cols.push_back(display_shelf(col));
  }
  const auto names = frame.features();

  // Per column: 1 = measurement (any numeric type), 2 = count column; bool and text are
  // never reformatted.
  std::vector<std::uint8_t> number_kind(cols.size(), 0);
  if (style.format_numbers && options.number_format != NumberFormat::Default) {
    for (std::size_t c = 0; c < cols.size(); ++c) {
      if (!is_numeric_shelf(cols[c]) && !style.format_numeric_text) {
        continue;
      }
      if (is_numeric_shelf(cols[c])) {
        const auto dt = cols[c].as_series().dtype();
        if (!dt.is_floating() && !dt.is_integral()) {
          continue;  // bool
        }
      }
      const bool is_count = std::find(style.count_columns.begin(), style.count_columns.end(), names[c]) !=
                            style.count_columns.end();
      number_kind[c] = is_count ? 2 : 1;
    }
  }

  std::string body;
  if (show_cards) {
    body += "<div class=\"summary-grid\">";
    body += pluss::tils::OutputEngine::format("<div class=\"summary-card\"><div class=\"label\">Cases</div><div class=\"value\">{}</div></div>",
                        frame.ncases());
    body +=
        pluss::tils::OutputEngine::format("<div class=\"summary-card\"><div class=\"label\">Features</div><div class=\"value\">{}</div></div>",
                    frame.nfeatures());
    body += pluss::tils::OutputEngine::format(
        "<div class=\"summary-card\"><div class=\"label\">Case Axis</div><div class=\"value\">{}</div></div>",
        escape_html_(logical_dtype(frame.labels().values())));
    body += "</div>";
  }

  body += table_frame_open_(style.virtual_rows);
  // The label column starts at a fixed default width like every other column (nothing is sized
  // from the labels, so long feature names never blow it out); the user drags it to taste.
  constexpr int kLabelColumnPx = 140;
  const int case_width_px = options.show_case_index ? kLabelColumnPx : 0;

  // In a browser view every column, the label column included, has the same fixed width (like a
  // spreadsheet; the page scrolls sideways) and text wraps inside it. The in-house view sizes its
  // columns itself.
  const bool browser_view = options.backend == DisplayBackend::NativeWebView;
  constexpr int kBrowserColumnPx = 150;
  const std::size_t total_columns = cols.size() + (options.show_case_index ? 1 : 0);
  // A long table also uses the cheaper fixed layout: the browser then lays out from the first row
  // instead of measuring every cell of the whole table.
  std::string table_attributes;
  if (browser_view || frame.ncases() > 5000 || style.virtual_rows) {
    table_attributes += " class=\"fixed\"";
  }
  if (browser_view) {
    table_attributes += pluss::tils::OutputEngine::format(
        " style=\"width:{}px\"", static_cast<std::int64_t>(total_columns) * kBrowserColumnPx);
  }
  if (style.virtual_rows) {
    table_attributes += " data-virtual=\"true\"";
  }
  body += pluss::tils::OutputEngine::format("<table id=\"{}\" data-filterable=\"true\"{}>",
                                            escape_html_(table_id), table_attributes);
  if (browser_view) {
    body += "<colgroup>";
    for (std::size_t i = 0; i < total_columns; ++i) {
      body += pluss::tils::OutputEngine::format("<col style=\"width:{}px\">", kBrowserColumnPx);
    }
    body += "</colgroup>";
  } else if (options.show_case_index) {
    body += "<colgroup>";
    body += pluss::tils::OutputEngine::format(
        "<col class=\"case-col\" style=\"width:{}px; min-width:{}px; max-width:{}px;\">",
        case_width_px,
        case_width_px,
        case_width_px);
    for (std::size_t i = 0; i < names.size(); ++i) {
      (void)i;
      body += "<col>";
    }
    body += "</colgroup>";
  }
  body += "<thead><tr>";

  std::size_t column_index = 0;
  if (options.show_case_index) {
    const auto case_numeric = is_numeric_shelf(frame.labels().values());
    body += pluss::tils::OutputEngine::format(
        "<th class=\"sortable sticky-left{}\" data-column-index=\"{}\" data-kind=\"{}\">{}</th>",
        case_numeric ? " num" : "",
        column_index++,
        case_numeric ? "numeric" : "text",
        browser_view ? escape_html_breakable_(corner_label) : escape_html_(corner_label));
  }

  for (std::size_t i = 0; i < names.size(); ++i) {
    const auto numeric = is_numeric_shelf(cols[i]);
    body += pluss::tils::OutputEngine::format("<th class=\"sortable{}\" data-column-index=\"{}\" data-kind=\"{}\">{}</th>",
                        numeric ? " num" : "",
                        column_index++,
                        numeric ? "numeric" : "text",
                        browser_view ? escape_html_breakable_(names[i]) : escape_html_(names[i]));
  }
  body += "</tr></thead><tbody>";

  if (style.virtual_rows) {
    // The rows travel as data and a short script draws the visible window; see the page script.
    body += "</tbody></table>";
    std::string data = "{\"num\":[";
    bool first_flag = true;
    const auto flag = [&](const bool on) {
      if (!first_flag) {
        data.push_back(',');
      }
      first_flag = false;
      data.push_back(on ? '1' : '0');
    };
    if (options.show_case_index) {
      flag(is_numeric_shelf(frame.labels().values()));
    }
    for (const auto& col : cols) {
      flag(is_numeric_shelf(col));
    }
    data += options.show_case_index ? "],\"sticky\":1,\"rows\":[" : "],\"sticky\":0,\"rows\":[";
    data.reserve(data.size() + static_cast<std::size_t>(frame.ncases()) * (cols.size() * 14 + 12));
    for (std::int64_t row = 0; row < frame.ncases(); ++row) {
      data += row == 0 ? "[" : ",[";
      bool first_cell = true;
      if (options.show_case_index) {
        append_json_string_(data, shelf_value_at(display_cases, row));
        first_cell = false;
      }
      for (std::size_t col = 0; col < cols.size(); ++col) {
        if (!first_cell) {
          data.push_back(',');
        }
        first_cell = false;
        append_json_string_(data, shelf_value_at(display_cols[col], row));
      }
      data.push_back(']');
    }
    data += "]}";
    body += pluss::tils::OutputEngine::format("<script type=\"application/json\" id=\"{}-data\">", escape_html_(table_id));
    body += data;
    body += "</script></div>";
    // Position readout: the scrollbar thumb of a very long table moves by fractions of a pixel,
    // so the script also says which rows are on screen.
    body += pluss::tils::OutputEngine::format("<div class=\"vbadge\" id=\"{}-badge\"></div>", escape_html_(table_id));
    body += "</div>";
    return body;
  }

  // Compact markup: the page for a long frame is dominated by cell tags, so attributes that
  // add nothing are left out (an empty class, a sort value equal to the text, a row index, the
  // class of odd rows).
  const bool case_numeric = options.show_case_index && is_numeric_shelf(frame.labels().values());
  std::vector<std::uint8_t> col_numeric(cols.size(), 0);
  for (std::size_t c = 0; c < cols.size(); ++c) {
    col_numeric[c] = is_numeric_shelf(cols[c]) ? 1 : 0;
  }
  const auto& heat_palette = palette_for_(resolve_theme_(options));
  body.reserve(body.size() + static_cast<std::size_t>(frame.ncases()) * (cols.size() * 34 + 48));

  for (std::int64_t row = 0; row < frame.ncases(); ++row) {
    body += (row % 2 == 1) ? "<tr class=\"even\">" : "<tr>";
    if (options.show_case_index) {
      const auto case_value = shelf_value_at(display_cases, row);
      body += "<td class=\"sticky-left";
      if (case_numeric) {
        body += " num";
      }
      if (case_value == "NULL") {
        body += " missing";
      }
      body += "\">";
      body += browser_view ? escape_html_breakable_(case_value) : escape_html_(case_value);
      body += "</td>";
    }
    for (std::size_t col = 0; col < cols.size(); ++col) {
      const auto value = shelf_value_at(display_cols[col], row);
      const bool numeric = col_numeric[col] != 0;
      const bool is_null = value == "NULL";
      const bool is_nonfinite = numeric && (value == "NaN" || value == "inf" || value == "-inf");
      auto shown = (style.blank_nulls && is_null) ? std::string{} : value;
      if (number_kind[col] != 0) {
        shown = format_number_cell_(value, number_kind[col] == 2, options.number_format);
      }
      if (is_nonfinite) {
        shown = (value == "NaN") ? "NAN" : (value == "inf" ? "INF" : "-INF");  // PlussView shows the loud forms; head()/tail() keep NaN/inf
      }
      const auto& sort_value = (style.blank_nulls && is_null) ? std::string{} : value;
      body += "<td";
      const bool missing_cell = is_null && !style.blank_nulls;
      if (numeric || missing_cell) {
        body += " class=\"";
        body += numeric ? (missing_cell ? "num missing"
                                   : (is_nonfinite ? (value == "NaN" ? "num nonfinite nan" : "num nonfinite inf") : "num"))
                    : "missing";
        body += "\"";
      }
      if (sort_value != shown) {
        body += " data-sort-value=\"";
        body += escape_html_(sort_value);
        body += "\"";
      }
      if (style.heatmap && numeric && !is_null && !is_nonfinite) {
        body += heat_style_(std::strtod(value.c_str(), nullptr), heat_palette);
      }
      body += ">";
      body += escape_html_(shown);
      body += "</td>";
    }
    body += "</tr>";
  }

  body += "</tbody></table></div></div>";
  return body;
}

std::vector<bool> shelf_flags_(const Shelf& flags_shelf, const std::int64_t n) {
  std::vector<bool> out(static_cast<std::size_t>(n), false);
  try {
    const auto flags = flags_shelf.as_series().to(pluss::codem::ScalarType::Bool).host<bool>();
    for (std::size_t i = 0; i < flags.size() && i < out.size(); ++i) {
      out[i] = flags[i];
    }
  } catch (const std::exception&) {
  }
  return out;
}

// One Bool per case: does the row hold a cell of the kind the filter looks for? Each cell is tested for what it
// is (missing, NaN, infinite), never by its text, so a name such as "Nancy" is never mistaken for a NaN. This is
// the DataFrame's own isnull / isnan / isinf, folded across the features with any.
Shelf row_filter_mask_(const DataFrame& frame, const DisplayFilter filter) {
  std::optional<Shelf> mask;
  const auto add = [&](const Shelf& flags) { mask = mask.has_value() ? Shelf(*mask | flags) : flags; };
  if (frame.nfeatures() > 0) {
    if (filter == DisplayFilter::Invalid || filter == DisplayFilter::Null) {
      add(frame.isnull().any(Axis::Feature));
    }
    if (filter == DisplayFilter::Invalid || filter == DisplayFilter::NaN) {
      add(frame.isnan().any(Axis::Feature));
    }
    if (filter == DisplayFilter::Invalid || filter == DisplayFilter::Infinite) {
      add(frame.isinf().any(Axis::Feature));
    }
  }
  return mask.has_value() ? *mask : Shelf(pluss::make_series(std::vector<bool>(static_cast<std::size_t>(frame.ncases()), false)));
}

// The frame with only the rows the filter keeps.
DataFrame filtered_frame_(const DataFrame& frame, const DisplayFilter filter) {
  return frame.filter(row_filter_mask_(frame, filter));
}

// The View > Visit choice as the row window every table applies.
pluss::view::RowWindow row_window_for_(const DisplayOptions& options) {
  using Kind = pluss::view::RowWindow::Kind;
  pluss::view::RowWindow window;
  window.count = options.visit_rows;
  switch (options.visit) {
    case DisplayVisit::First: window.kind = Kind::First; break;
    case DisplayVisit::Middle: window.kind = Kind::Middle; break;
    case DisplayVisit::Last: window.kind = Kind::Last; break;
    case DisplayVisit::Custom:
      window.kind = Kind::Range;
      window.begin = options.visit_begin;
      window.end = options.visit_end;
      break;
    case DisplayVisit::All: break;
  }
  return window;
}

// The rows of a short table that are shown: the ones the filter keeps, then the ones the window visits, both
// through the DataFrame's own filter and slice.
struct ShownRows {
  DataFrame frame;
  std::string subtitle;
};

ShownRows shown_rows_(const DataFrame& frame, const DisplayOptions& options) {
  DataFrame shown = options.filter != DisplayFilter::None ? filtered_frame_(frame, options.filter) : frame;
  if (options.visit != DisplayVisit::All) {
    const auto [begin, end] = pluss::view::window_bounds(row_window_for_(options), shown.ncases());
    shown = shown.slice(begin, end);
  }
  const bool reduced = shown.ncases() != frame.ncases();
  return {shown, reduced ? pluss::tils::OutputEngine::format("{} of {} cases, {} features", shown.ncases(), frame.ncases(),
                                                              frame.nfeatures())
                         : pluss::tils::OutputEngine::format("{} cases, {} features", frame.ncases(), frame.nfeatures())};
}

class DataFrameVirtualTableModel final
    : public OptionsModel {
 public:
  explicit DataFrameVirtualTableModel(const DataFrame& frame)
      : cases_(display_shelf(frame.labels().values())),
        columns_(dataframe_columns_(frame)),
        frame_(frame),
        names_(frame.features()),
        rows_(frame.ncases()) {
    numeric_.reserve(columns_.size());
    search_columns_.reserve(columns_.size());
    for (const auto& raw : columns_) {
      // Measurements and counts follow the Format menu; bool and text never do.
      const bool number = is_numeric_shelf(raw) && (raw.as_series().dtype().is_floating() || raw.as_series().dtype().is_integral());
      formattable_.push_back(number);
    }
    std::unordered_set<const void*> shared_numeric_payloads;
    for (std::size_t index = 0; index < columns_.size(); ++index) {
      auto& column = columns_[index];
      column = display_shelf(column);
      numeric_.push_back(is_numeric_shelf(column));
      bool searchable = true;
      if (column.kind() == ShelfKind::Series &&
          !detail::has_mask(column)) {
        const void* payload = column.as_series().raw_data();
        searchable = shared_numeric_payloads.insert(payload).second;
      }
      if (searchable) {
        search_columns_.push_back(index);
      }
    }
  }

  std::int64_t row_count() const noexcept override {
    return rows_;
  }

  std::int64_t column_count() const noexcept override {
    return static_cast<std::int64_t>(columns_.size());
  }

  std::string row_header() const override {
    return "case";
  }

  std::string row_label(const std::int64_t row) const override {
    return shelf_value_at(cases_, row);
  }

  std::string column_name(const std::int64_t column) const override {
    if (column < 0 ||
        static_cast<std::size_t>(column) >= names_.size()) {
      return {};
    }
    return names_[static_cast<std::size_t>(column)];
  }

  bool column_numeric(const std::int64_t column) const noexcept override {
    return column >= 0 &&
           static_cast<std::size_t>(column) < numeric_.size() &&
           numeric_[static_cast<std::size_t>(column)];
  }

  pluss::view::VirtualTableCell cell(
      const std::int64_t row,
      const std::int64_t column) const override {
    if (row < 0 || row >= rows_ || column < 0 ||
        static_cast<std::size_t>(column) >= columns_.size()) {
      return {};
    }
    const auto& source = columns_[static_cast<std::size_t>(column)];
    const bool missing = !detail::valid_at(source, row);
    std::string text = missing ? std::string("NULL") : shelf_value_at(source, row);
    const bool nonfinite = !missing && (text == "NaN" || text == "inf" || text == "-inf") && is_numeric_shelf(source);
    if (nonfinite) {
      text = (text == "NaN") ? "NAN" : (text == "inf" ? "INF" : "-INF");  // PlussView shows the loud forms
    } else if (!missing && formattable_[static_cast<std::size_t>(column)]) {
      const auto format = static_cast<NumberFormat>(number_format_.load(std::memory_order_acquire));
      if (format != NumberFormat::Default) {
        text = format_number_cell_(text, /*count_like=*/false, format);
      }
    }
    return pluss::view::VirtualTableCell{
        .text = std::move(text),
        .missing = missing,
        .nonfinite = nonfinite,
        .infinite = nonfinite && text != "NAN",
    };  }

  // The rows the View > Filter menu keeps. Called from the menu's thread while the search thread reads it.
  void set_filter(const DisplayFilter filter) const {
    std::shared_ptr<const std::vector<bool>> keep;
    if (filter != DisplayFilter::None) {
      keep = std::make_shared<const std::vector<bool>>(shelf_flags_(row_filter_mask_(frame_, filter), rows_));
    }
    const std::scoped_lock lock(filter_mutex_);
    keep_ = std::move(keep);
    filtering_.store(keep_ != nullptr, std::memory_order_release);
  }

  bool filtering() const noexcept override {
    return filtering_.load(std::memory_order_acquire);
  }

  // How numbers are written (View > Format); the stored values do not change.
  void set_number_format(const NumberFormat format) const {
    number_format_.store(static_cast<int>(format), std::memory_order_release);
  }

  // The filter, the visit and the number format of a set of options, all at once.
  void apply(const DisplayOptions& options) const override {
    set_filter(options.filter);
    set_window(row_window_for_(options));
    set_number_format(options.number_format);
  }

  // How many rows the filter and the visit leave.
  std::int64_t visible_rows() const {
    std::shared_ptr<const std::vector<bool>> keep;
    pluss::view::RowWindow window;
    {
      const std::scoped_lock lock(filter_mutex_);
      keep = keep_;
      window = window_;
    }
    std::int64_t rows = rows_;
    if (keep != nullptr) {
      rows = static_cast<std::int64_t>(std::count(keep->begin(), keep->end(), true));
    }
    const auto [first, last] = pluss::view::window_bounds(window, rows);
    return last - first;
  }

  // The rows the View > Visit menu visits, among those the filter, search and sort leave.
  void set_window(const pluss::view::RowWindow& window) const {
    const std::scoped_lock lock(filter_mutex_);
    window_ = window;
  }

  pluss::view::RowWindow row_window() const override {
    const std::scoped_lock lock(filter_mutex_);
    return window_;
  }

  bool row_allowed(const std::int64_t row) const override {
    std::shared_ptr<const std::vector<bool>> keep;
    {
      const std::scoped_lock lock(filter_mutex_);
      keep = keep_;
    }
    return keep == nullptr || (row >= 0 && static_cast<std::size_t>(row) < keep->size() &&
                               (*keep)[static_cast<std::size_t>(row)]);
  }

  bool row_matches(const std::int64_t row,
                   const std::string_view folded_query) const override {
    if (row < 0 || row >= rows_ || folded_query.empty()) {
      return folded_query.empty();
    }
    const auto contains_query = [&](const std::string& value) {
      std::string folded;
      folded.reserve(value.size());
      for (const char ch : value) {
        folded.push_back(static_cast<char>(
            std::tolower(static_cast<unsigned char>(ch))));
      }
      return folded.find(folded_query) != std::string::npos;
    };
    if (contains_query(shelf_value_at(cases_, row))) {
      return true;
    }
    for (const std::size_t index : search_columns_) {
      if (contains_query(shelf_value_at(columns_[index], row))) {
        return true;
      }
    }
    return false;
  }

 private:
  Shelf cases_{};
  std::vector<Shelf> columns_{};
  DataFrame frame_{};  // the frame as stored: what NULL, NaN and infinity are decided on
  mutable std::mutex filter_mutex_{};
  mutable std::shared_ptr<const std::vector<bool>> keep_{};
  mutable pluss::view::RowWindow window_{};
  mutable std::atomic<bool> filtering_{false};
  mutable std::atomic<int> number_format_{0};
  std::vector<bool> formattable_{};
  std::vector<std::string> names_{};
  std::vector<bool> numeric_{};
  std::vector<std::size_t> search_columns_{};
  std::int64_t rows_{0};
};

// Body of the data-table panel when the in-house viewer paints the rows from a model: the
// summary cards and an empty table marked as virtual.
// The cards above a table: a label and a value each.
std::string summary_cards_html_(const std::vector<std::pair<std::string, std::string>>& cards) {
  std::string out = "<div class=\"summary-grid\">";
  for (const auto& [label, value] : cards) {
    out += pluss::tils::OutputEngine::format(
        "<div class=\"summary-card\"><div class=\"label\">{}</div><div class=\"value\">{}</div></div>",
        escape_html_(label), escape_html_(value));
  }
  return out + "</div>";
}

// The frame and the empty table the viewer fills from a model (the same for a DataFrame and a ragged catalog).
std::string virtual_table_frame_() {
  return table_frame_open_() + "<table id=\"plussmic-table\" data-virtual-table=\"true\"></table>" +
         std::string(kTableFrameClose);
}

std::string virtual_table_body_(const DataFrame& frame) {
  return summary_cards_html_({{"Cases", std::to_string(frame.ncases())},
                              {"Features", std::to_string(frame.nfeatures())},
                              {"Case Axis", logical_dtype(frame.labels().values())}}) +
         virtual_table_frame_();
}

// ---------------------------------------------------------------------------
// Shared helpers for the profiling views (Missing / Outliers). Every view renders as a table so the viewer's table scrolling works.
// ---------------------------------------------------------------------------

// Series of doubles with some entries unavailable (shown blank in the table).
Shelf masked_f64_(const std::vector<double>& values, const std::vector<std::uint8_t>& ok) {
  const auto mask = pluss::make_series(ok).to(pluss::codem::ScalarType::Bool);
  return Shelf(pluss::make_series(values), mask);
}

// Per-case "is missing" flags for one column.
std::vector<bool> missing_flags_(const Shelf& col, const std::int64_t n) {
  return shelf_flags_(col.isnull(), n);
}

HtmlBodyStyle profile_style_(std::string_view corner, std::vector<std::string> count_columns = {}) {
  HtmlBodyStyle style;
  style.corner_label = corner;
  style.show_cards = true;  // each view shows the Cases / Features / Case Axis of its own table
  style.blank_nulls = true;
  style.format_numbers = true;
  style.count_columns = std::move(count_columns);
  return style;
}

// "Default" shows the tidy default rounding; any explicit number format starts from the
// full-precision values instead.
DataFrame auto_round_(const DataFrame& table, const std::int64_t decimals, const DisplayOptions& options) {
  return options.number_format == NumberFormat::Default ? DataFrame(table.round(decimals)) : table;
}

bool is_stat_numeric_(const Shelf& col) {
  return is_numeric_shelf(col) && col.as_series().dtype().scalar != pluss::codem::ScalarType::Bool;
}

// ---------------------------------------------------------------------------
// The profiling views of a frame (Summary, Correlation, Missing, Outliers). Computing one
// (percentiles, correlations, ...) is the expensive part and turning the result into HTML is
// cheap, so StatsViews computes a view only when it is first asked for and keeps the result: a
// theme or number-format change then only re-renders, and a view nobody opens is never computed.
// ---------------------------------------------------------------------------
struct ViewResult {
  DataFrame table;         // unrounded
  std::size_t numeric{0};  // correlation: how many numeric columns the frame has
  bool pairs{false};       // correlation: the strongest pairs instead of the full matrix
};

// Match the public numerical summary exactly; the other profiling view owns
// null and nonfinite diagnostics.
std::optional<ViewResult> compute_summary_(const DataFrame& frame) {
  try {
    return ViewResult{DataFrame(frame.describe(false).transpose(false)), 0, false};
  } catch (const std::exception&) {
    return std::nullopt;
  }
}

// Pearson correlation between numeric columns. Up to kMatrixColumns numeric columns it is the
// full matrix; beyond that an N x N matrix is unreadable (and grows quadratically), so it lists
// only the strongest pairs instead. Empty when there are fewer than two numeric columns or
// correlation is undefined for the frame.
std::optional<ViewResult> compute_correlation_(const DataFrame& frame) {
  constexpr std::size_t kMatrixColumns = 25;
  constexpr std::size_t kMaxPairs = 50;
  std::size_t numeric = 0;
  for (const auto& col : dataframe_columns_(frame)) {
    numeric += is_stat_numeric_(col) ? 1 : 0;
  }
  if (numeric < 2) {
    return std::nullopt;
  }
  try {
    const DataFrame corr_full = frame.corr();
    if (numeric <= kMatrixColumns) {
      return ViewResult{corr_full, numeric, false};
    }

    // Strongest pairs by |r|, upper triangle only.
    const auto matrix_cols = dataframe_columns_(corr_full);
    const auto matrix_names = corr_full.features();
    struct Pair {
      std::size_t a, b;
      double r;
    };
    std::vector<Pair> pairs;
    std::vector<std::vector<double>> values;
    values.reserve(matrix_cols.size());
    for (const auto& col : matrix_cols) {
      values.push_back(col.as_series().to(pluss::codem::ScalarType::Float64).host<double>());
    }
    for (std::size_t a = 0; a < values.size(); ++a) {
      for (std::size_t b = a + 1; b < values.size() && b < values[a].size() + a + 1; ++b) {
        // Row b of column a is the correlation of features b and a.
        if (b >= values[a].size()) {
          continue;
        }
        const double r = values[a][b];
        if (std::isfinite(r)) {
          pairs.push_back({a, b, r});
        }
      }
    }
    const std::size_t keep = std::min(kMaxPairs, pairs.size());
    std::partial_sort(pairs.begin(), pairs.begin() + static_cast<std::ptrdiff_t>(keep), pairs.end(),
                      [](const Pair& x, const Pair& y) { return std::abs(x.r) > std::abs(y.r); });
    pairs.resize(keep);
    if (pairs.empty()) {
      return std::nullopt;
    }
    std::vector<std::string> first, second;
    std::vector<double> r;
    for (const auto& p : pairs) {
      first.push_back(matrix_names[p.a]);
      second.push_back(matrix_names[p.b]);
      r.push_back(p.r);
    }
    std::vector<std::pair<std::string, Shelf>> out;
    out.emplace_back("feature", Shelf(pluss::make_note(first)));
    out.emplace_back("with", Shelf(pluss::make_note(second)));
    out.emplace_back("r", Shelf(pluss::make_series(r)));
    return ViewResult{DataFrame(std::move(out)), numeric, true};
  } catch (const std::exception&) {
    return std::nullopt;
  }
}

// Separate absent cells from nonfinite numeric values. The last row counts cases where any column has each
// condition. Everything is the DataFrame's own isnull / isnan / isinf and sum: one pass per kind over the
// whole frame, with an extra "(any column)" feature that folds each row across the features with any.
std::optional<ViewResult> compute_missing_(const DataFrame& frame) {
  const auto n = frame.ncases();
  if (frame.nfeatures() == 0 || n == 0) {
    return std::nullopt;
  }
  try {
    constexpr std::string_view kAnyColumn = "(any column)";
    auto row_names = frame.features();
    row_names.emplace_back(kAnyColumn);
    const std::array<std::pair<std::string, DataFrame>, 3> kinds{{
        {"null", frame.isnull()},
        {"NaN", frame.isnan()},
        {"infinity", frame.isinf()},
    }};
    std::vector<std::pair<std::string, Shelf>> out;
    for (const auto& [label, flags] : kinds) {
      const DataFrame with_any = flags.insert(flags.nfeatures(), std::string(kAnyColumn), flags.any(Axis::Feature));
      const auto counts = with_any.sum().as_series().to(pluss::codem::ScalarType::Int64);
      out.emplace_back(label, Shelf(counts));
      out.emplace_back(label + " %", Shelf(counts.to(pluss::codem::ScalarType::Float64) * (100.0 / static_cast<double>(n))));
    }
    return ViewResult{DataFrame(std::move(out), Shelf(pluss::make_note(row_names))), 0, false};
  } catch (const std::exception&) {
    return std::nullopt;
  }
}

// Per numeric column, by the 1.5 x IQR rule: the floor and ceiling (the lower and upper
// fences), how many values fall below / above them, the percent of the column that is outlying,
// and the single most extreme value. (To find where the outliers are, sort the Table view by
// that column: the extremes come first.)
std::optional<ViewResult> compute_outliers_(const DataFrame& frame) {
  const auto cols = dataframe_columns_(frame);
  const auto names = frame.features();
  const auto n = frame.ncases();

  std::vector<std::string> row_names;
  std::vector<double> lower, upper, below, above, share, extreme;
  std::vector<std::uint8_t> has_fence, has_extreme;
  for (std::size_t c = 0; c < cols.size(); ++c) {
    if (!is_stat_numeric_(cols[c])) {
      continue;
    }
    row_names.push_back(names[c]);
    double lo = 0.0, hi = 0.0, nb = 0.0, na = 0.0, pct = 0.0, ext = 0.0;
    bool fenced = false, has_ext = false;
    try {
      const auto q = cols[c].percentiles({0.25, 0.75}).as_series().to(pluss::codem::ScalarType::Float64).host<double>();
      const auto values = cols[c].as_series().to(pluss::codem::ScalarType::Float64).host<double>();
      const auto missing = missing_flags_(cols[c], n);
      if (q.size() >= 2 && q[1] - q[0] > 0.0) {
        const double iqr = q[1] - q[0];
        lo = q[0] - 1.5 * iqr;
        hi = q[1] + 1.5 * iqr;
        fenced = true;
        double valid = 0.0, best = -1.0;
        for (std::int64_t i = 0; i < n && i < static_cast<std::int64_t>(values.size()); ++i) {
          const double v = values[static_cast<std::size_t>(i)];
          if (missing[static_cast<std::size_t>(i)] || !std::isfinite(v)) {
            continue;
          }
          valid += 1.0;
          double beyond = 0.0;
          if (v < lo) {
            nb += 1.0;
            beyond = (lo - v) / iqr;
          } else if (v > hi) {
            na += 1.0;
            beyond = (v - hi) / iqr;
          } else {
            continue;
          }
          if (beyond > best) {
            best = beyond;
            ext = v;
            has_ext = true;
          }
        }
        pct = valid > 0.0 ? 100.0 * (nb + na) / valid : 0.0;
      }
    } catch (const std::exception&) {
      fenced = false;
      has_ext = false;
    }
    lower.push_back(lo);
    upper.push_back(hi);
    below.push_back(nb);
    above.push_back(na);
    share.push_back(pct);
    extreme.push_back(ext);
    has_fence.push_back(fenced ? 1 : 0);
    has_extreme.push_back(has_ext ? 1 : 0);
  }
  if (row_names.empty()) {
    return std::nullopt;
  }
  std::vector<std::pair<std::string, Shelf>> out;
  out.emplace_back("floor", masked_f64_(lower, has_fence));
  out.emplace_back("ceiling", masked_f64_(upper, has_fence));
  out.emplace_back("below", masked_f64_(below, has_fence));
  out.emplace_back("above", masked_f64_(above, has_fence));
  out.emplace_back("percent", masked_f64_(share, has_fence));
  out.emplace_back("extreme", masked_f64_(extreme, has_extreme));
  const DataFrame table(std::move(out), Shelf(pluss::make_note(row_names)));
  return ViewResult{table, 0, false};
}

std::string render_summary_(const ViewResult& result, const DisplayOptions& options) {
  auto style = profile_style_("feature", {"count"});
  style.format_numeric_text = true;
  return render_html_table_body_(result.table, options, "plussmic-summary", style);
}

std::string render_correlation_(const ViewResult& result, const DisplayOptions& options) {
  HtmlBodyStyle style = profile_style_(result.pairs ? "rank" : "feature");
  style.heatmap = true;
  std::string html =
      render_html_table_body_(auto_round_(result.table, 3, options), options, "plussmic-corr", style);
  if (result.pairs) {
    html += pluss::tils::OutputEngine::format(
        "<div class=\"muted\" style=\"padding:8px 2px;\">{} numeric columns: showing the {} strongest pairs instead of the full matrix.</div>",
        result.numeric, result.table.ncases());
  }
  return html;
}

std::string render_missing_(const ViewResult& result, const DisplayOptions& options) {
  return render_html_table_body_(auto_round_(result.table, 1, options), options, "plussmic-missing",
                                 profile_style_("feature", {"null", "NaN", "infinity"}));
}

std::string render_outliers_(const ViewResult& result, const DisplayOptions& options) {
  return render_html_table_body_(auto_round_(result.table, 4, options), options, "plussmic-outliers",
                                 profile_style_("feature", {"below", "above"}));
}

// The views of one frame, by id, in menu order. available() is cheap (column types only);
// render() computes on first use and then reuses the result.
struct ViewInfo {
  std::string id;
  std::string label;
};

// Where the view tabs of a page come from: which views apply, and each one rendered for the current options.
class ViewSource {
 public:
  virtual ~ViewSource() = default;
  virtual std::vector<ViewInfo> available() = 0;
  virtual std::string render(const std::string& id, const DisplayOptions& options) = 0;
};

class StatsViews final : public ViewSource {
 public:
  explicit StatsViews(DataFrame frame) : frame_(std::move(frame)) {}

  using Info = ViewInfo;

  // Views that can apply to this frame. A view listed here may still turn out to be undefined
  // (render() then returns an empty string).
  std::vector<Info> available() override {
    const auto columns = dataframe_columns_(frame_);
    std::size_t numeric = 0;
    for (const auto& col : columns) {
      numeric += is_stat_numeric_(col) ? 1 : 0;
    }
    std::vector<Info> out;
    const bool has_describable_column =
        std::any_of(columns.begin(), columns.end(), [](const Shelf& col) { return is_numeric_shelf(col); });
    if (has_describable_column) {
      out.push_back({"summary", "Summary"});
    }
    if (numeric >= 2) {
      out.push_back({"correlation", "Correlation"});
    }
    if (frame_.ncases() > 0 && frame_.nfeatures() > 0) {
      out.push_back({"missing", "Validity"});
    }
    if (numeric >= 1) {
      out.push_back({"outliers", "Outliers"});
    }
    return out;
  }

  std::string render(const std::string& id, const DisplayOptions& options) override {
    const auto& result = result_for_(id);
    if (!result.has_value()) {
      return {};
    }
    if (id == "summary") {
      return render_summary_(*result, options);
    }
    if (id == "correlation") {
      return render_correlation_(*result, options);
    }
    if (id == "missing") {
      return render_missing_(*result, options);
    }
    return render_outliers_(*result, options);
  }

 private:
  const std::optional<ViewResult>& result_for_(const std::string& id) {
    const auto found = cache_.find(id);
    if (found != cache_.end()) {
      return found->second;
    }
    std::optional<ViewResult> computed;
    if (id == "summary") {
      computed = compute_summary_(frame_);
    } else if (id == "correlation") {
      computed = compute_correlation_(frame_);
    } else if (id == "missing") {
      computed = compute_missing_(frame_);
    } else if (id == "outliers") {
      computed = compute_outliers_(frame_);
    }
    return cache_.emplace(id, std::move(computed)).first->second;
  }

  DataFrame frame_;
  std::map<std::string, std::optional<ViewResult>> cache_;
};

// The page for a frame: the data table plus, as views of the View menu, the profiling views.
// `views` supplies (and caches) those views. With `lazy` their panels are left empty for the
// window to fill the first time each is opened; `virtual_model` means the in-house viewer paints
// the data table's rows from a model.
// The ids of the controls in the custom range bar (see visit_bar_html_) and of the entries of View > Visit.
constexpr std::string_view kVisitApplyId = "visit-apply";
constexpr const char* kVisitBeginId = "visit-begin";
constexpr const char* kVisitEndId = "visit-end";

// What the header's subtitle says while the custom range is being typed.
constexpr std::string_view kVisitInstruction =
    "Numbers only. Leave a box blank for the start or the end.";

// The controls that open in the header's toolbar, beside the search box, under View > Visit > Custom: the first
// position, the one to stop before, and Apply. They go away once applied.
std::string visit_inline_html_(const DisplayOptions& options) {
  const auto shown = [](const std::int64_t value, const std::int64_t unset) {
    return value == unset ? std::string{} : std::to_string(value);
  };
  return pluss::tils::OutputEngine::format(
      "<div class=\"visit-inline\"><input class=\"visit-box\" id=\"{0}\" type=\"text\" placeholder=\"0\" value=\"{2}\" />"
      "<input class=\"visit-box\" id=\"{1}\" type=\"text\" placeholder=\"1\" value=\"{3}\" />"
      "<button id=\"{4}\" type=\"button\">Apply</button></div>",
      kVisitBeginId, kVisitEndId, shown(options.visit_begin, 0),
      shown(options.visit_end, std::numeric_limits<std::int64_t>::max()), std::string(kVisitApplyId));
}

// The views of a ragged catalog. Every one is computed from the streams as they are, never from a padded
// rectangle: the per-key views (Summary, Outliers) run the DataFrame views on each key alone and stack the rows,
// Validity counts each key's NULLs, NaNs and infinities and, separately, the entries it does not have, and
// Correlation uses the entries every key shares (the first ones under head alignment, the last under tail).
class CatalogViews final : public ViewSource {
 public:
  using Stream = std::pair<std::string, std::shared_ptr<const Shelf>>;

  // `part` and `entries` are the words for a column and its items: key and entries for a catalog, field and states for a
  // registry.
  CatalogViews(std::vector<Stream> streams, std::string part, std::string entries)
      : streams_(std::move(streams)), part_(std::move(part)), entries_(std::move(entries)) {
    for (const auto& [name, shelf] : streams_) {
      longest_ = std::max(longest_, shelf->size());
    }
    shortest_ = streams_.empty() ? 0 : longest_;
    for (const auto& [name, shelf] : streams_) {
      shortest_ = std::min(shortest_, shelf->size());
    }
  }

  std::vector<ViewInfo> available() override {
    std::size_t numeric = 0;
    for (const auto& [name, shelf] : streams_) {
      numeric += is_stat_numeric_(*shelf) ? 1 : 0;
    }
    std::vector<ViewInfo> out;
    if (numeric >= 1) {
      out.push_back({"summary", "Summary"});
    }
    if (numeric >= 2 && shortest_ > 0) {
      out.push_back({"correlation", "Correlation"});
    }
    if (!streams_.empty()) {
      out.push_back({"missing", "Validity"});
    }
    if (numeric >= 1) {
      out.push_back({"outliers", "Outliers"});
    }
    if (!streams_.empty()) {
      out.push_back({"layout", "Raggedness"});
    }
    return out;
  }

  std::string render(const std::string& id, const DisplayOptions& options) override {
    const auto& result = result_for_(id, options);
    if (!result.has_value()) {
      return {};
    }
    if (id == "summary") {
      return render_summary_(*result, options);
    }
    if (id == "correlation") {
      return render_correlation_(*result, options) +
             pluss::tils::OutputEngine::format(
                 "<div class=\"muted\" style=\"padding:8px 2px;\">Over the {} {} every {} has, taken from the {}.</div>",
                 shortest_, entries_, part_, options.align == DisplayAlign::Tail ? "end" : "start");
    }
    if (id == "missing") {
      return render_html_table_body_(auto_round_(result->table, 1, options), options, "plussmic-missing",
                                     profile_style_(part_, {"null", "NaN", "infinity", "absent"}));
    }
    if (id == "layout") {
      // The whole catalog's raggedness as cards, then one row per key.
      auto style = profile_style_(part_, {entries_, "absent", "valid", "from", "to"});
      style.show_cards = false;
      return ragged_cards_() + render_html_table_body_(auto_round_(result->table, 1, options), options,
                                                       "plussmic-ragged", style);
    }
    return render_outliers_(*result, options);
  }

 private:
  DataFrame one_(const Stream& stream) const {
    return DataFrame({{stream.first, *stream.second}});
  }

  // The rows of a per-key view, stacked key by key.
  template <class Compute>
  std::optional<ViewResult> stacked_(const Compute& compute) const {
    std::optional<DataFrame> all;
    for (const auto& stream : streams_) {
      if (!is_stat_numeric_(*stream.second)) {
        continue;
      }
      const auto part = compute(one_(stream));
      if (!part.has_value()) {
        continue;
      }
      all = all.has_value() ? all->extend(part->table) : part->table;
    }
    if (!all.has_value()) {
      return std::nullopt;
    }
    return ViewResult{*all, 0, false};
  }

  static std::int64_t count_of_(const Shelf& flags) {
    return flags.as_series().to(pluss::codem::ScalarType::Int64).sum().host<std::int64_t>().front();
  }

  std::optional<ViewResult> validity_() const {
    std::vector<std::string> keys;
    std::vector<std::int64_t> nulls, nans, infs, absent;
    std::vector<double> total;
    for (const auto& [name, shelf] : streams_) {
      keys.push_back(name);
      nulls.push_back(count_of_(shelf->isnull()));
      nans.push_back(is_numeric_shelf(*shelf) ? count_of_(shelf->isnan()) : 0);
      infs.push_back(is_numeric_shelf(*shelf) ? count_of_(shelf->isinf()) : 0);
      absent.push_back(longest_ - shelf->size());
      total.push_back(static_cast<double>(shelf->size()));
    }
    const auto percent = [&](const std::vector<std::int64_t>& counts, const bool of_longest) {
      std::vector<double> out;
      for (std::size_t i = 0; i < counts.size(); ++i) {
        const double whole = of_longest ? static_cast<double>(longest_) : total[i];
        out.push_back(whole > 0.0 ? 100.0 * static_cast<double>(counts[i]) / whole : 0.0);
      }
      return out;
    };
    std::vector<std::pair<std::string, Shelf>> columns;
    columns.emplace_back("null", Shelf(pluss::make_series(nulls)));
    columns.emplace_back("null %", Shelf(pluss::make_series(percent(nulls, false))));
    columns.emplace_back("NaN", Shelf(pluss::make_series(nans)));
    columns.emplace_back("NaN %", Shelf(pluss::make_series(percent(nans, false))));
    columns.emplace_back("infinity", Shelf(pluss::make_series(infs)));
    columns.emplace_back("infinity %", Shelf(pluss::make_series(percent(infs, false))));
    columns.emplace_back("absent", Shelf(pluss::make_series(absent)));
    columns.emplace_back("absent %", Shelf(pluss::make_series(percent(absent, true))));
    return ViewResult{DataFrame(std::move(columns), Shelf(pluss::make_note(keys))), 0, false};
  }

  // How ragged the catalog is, at a glance.
  std::string ragged_cards_() const {
    std::int64_t total = 0;
    std::int64_t full = 0;
    for (const auto& [name, shelf] : streams_) {
      total += shelf->size();
      full += shelf->size() == longest_ ? 1 : 0;
    }
    const std::int64_t keys = static_cast<std::int64_t>(streams_.size());
    const double mean = keys > 0 ? static_cast<double>(total) / static_cast<double>(keys) : 0.0;
    return summary_cards_html_({
        {"Ragged", longest_ == shortest_ ? "no" : "yes"},
        {"Longest", std::to_string(longest_)},
        {"Shortest", std::to_string(shortest_)},
        {"Spread", std::to_string(longest_ - shortest_)},
        {"Mean length", pluss::tils::OutputEngine::format("{:.1f}", mean)},
        {"Full-length keys", pluss::tils::OutputEngine::format("{} of {}", full, keys)},
        {"Entries every key has", std::to_string(shortest_)},
    });
  }

  // One row per key: its type, how many entries it has and lacks, how much of the longest key it covers, how many
  // of its entries are not NULL, and the rows it fills under the current alignment.
  std::optional<ViewResult> layout_(const DisplayOptions& options) const {
    std::vector<std::string> keys, dtypes;
    std::vector<std::int64_t> entries, absent, valid, from, to;
    std::vector<double> coverage;
    for (const auto& [name, shelf] : streams_) {
      const std::int64_t n = shelf->size();
      const std::int64_t start = options.align == DisplayAlign::Tail ? longest_ - n : 0;
      keys.push_back(name);
      dtypes.push_back(logical_dtype(*shelf));
      entries.push_back(n);
      absent.push_back(longest_ - n);
      coverage.push_back(longest_ > 0 ? 100.0 * static_cast<double>(n) / static_cast<double>(longest_) : 0.0);
      valid.push_back(n - count_of_(shelf->isnull()));
      from.push_back(start);
      to.push_back(start + n);
    }
    std::vector<std::pair<std::string, Shelf>> columns;
    columns.emplace_back("dtype", Shelf(pluss::make_note(dtypes)));
    columns.emplace_back(entries_, Shelf(pluss::make_series(entries)));
    columns.emplace_back("absent", Shelf(pluss::make_series(absent)));
    columns.emplace_back("coverage %", Shelf(pluss::make_series(coverage)));
    columns.emplace_back("valid", Shelf(pluss::make_series(valid)));
    columns.emplace_back("from", Shelf(pluss::make_series(from)));
    columns.emplace_back("to", Shelf(pluss::make_series(to)));
    return ViewResult{DataFrame(std::move(columns), Shelf(pluss::make_note(keys))), 0, false};
  }

  std::optional<ViewResult> shared_correlation_(const DisplayOptions& options) const {
    std::vector<std::pair<std::string, Shelf>> columns;
    for (const auto& [name, shelf] : streams_) {
      if (!is_stat_numeric_(*shelf)) {
        continue;
      }
      const std::int64_t n = shelf->size();
      columns.emplace_back(name, options.align == DisplayAlign::Tail ? shelf->slice(n - shortest_, n) : shelf->slice(0, shortest_));
    }
    return compute_correlation_(DataFrame(std::move(columns)));
  }

  const std::optional<ViewResult>& result_for_(const std::string& id, const DisplayOptions& options) {
    const std::string key = id + (options.align == DisplayAlign::Tail ? ":tail" : ":head");
    const auto found = cache_.find(key);
    if (found != cache_.end()) {
      return found->second;
    }
    std::optional<ViewResult> computed;
    try {
      if (id == "summary") {
        computed = stacked_([](const DataFrame& one) { return compute_summary_(one); });
      } else if (id == "outliers") {
        computed = stacked_([](const DataFrame& one) { return compute_outliers_(one); });
      } else if (id == "missing") {
        computed = validity_();
      } else if (id == "layout") {
        computed = layout_(options);
      } else if (id == "correlation") {
        computed = shared_correlation_(options);
      }
    } catch (const std::exception&) {
      computed = std::nullopt;
    }
    return cache_.emplace(key, std::move(computed)).first->second;
  }

  std::vector<Stream> streams_;
  std::string part_;     // the word for a column: key, field
  std::string entries_;  // the word for its items: entries, states
  std::int64_t longest_{0};
  std::int64_t shortest_{0};
  std::map<std::string, std::optional<ViewResult>> cache_;
};

// The page body with its view tabs: the table first, then each view the source offers (filled when first opened
// where the window can do that, otherwise now). A page with only the table is just the table.
std::string tabbed_body_(std::string table_body, ViewSource& views, const DisplayOptions& options, const bool lazy) {
  std::string body;
    struct Panel {
      std::string id;
      std::string label;
      std::string html;
      bool lazy{false};
    };
    std::vector<Panel> panels;
    panels.push_back({"table", "Table", std::move(table_body), false});
    for (const auto& info : views.available()) {
      Panel panel{info.id, info.label, {}, lazy};
      if (!lazy) {
        panel.html = views.render(info.id, options);
        if (panel.html.empty()) {
          continue;  // undefined for this frame: no tab
        }
      }
      panels.push_back(std::move(panel));
    }
    if (panels.size() > 1) {
      std::string items;
      std::string sections;
      for (std::size_t i = 0; i < panels.size(); ++i) {
        const auto& panel = panels[i];
        items += pluss::tils::OutputEngine::format(
            "<button id=\"tab-{0}\" type=\"button\" class=\"tab{2}\" data-tab-group=\"main\" data-tab-target=\"panel-{0}\" data-dropdown-menu=\"view-menu\" data-dropdown-label=\"view-current\">{1}</button>",
            panel.id, panel.label, i == 0 ? " active" : "");
        sections += pluss::tils::OutputEngine::format(
            "<section id=\"panel-{0}\" class=\"tab-panel{1}\" data-tab-group=\"main\"{3}>{2}</section>",
            panel.id, i == 0 ? " active" : "", panel.html,
            panel.lazy ? pluss::tils::OutputEngine::format(" data-lazy=\"{}\"", panel.id) : std::string{});
      }
      std::string tabs = pluss::tils::OutputEngine::format(
          "<div class=\"view-picker\"><button id=\"view-toggle\" type=\"button\" class=\"view-toggle\" data-dropdown=\"view-menu\">View: <span id=\"view-current\">{}</span> \u25BE</button>"
          "<div id=\"view-menu\" class=\"view-menu\">{}</div></div>",
          panels.front().label, items);
      body = tabs + sections;
    } else {
      body = std::move(panels.front().html);
    }
    return body;
}

std::string render_html_table_(const DataFrame& frame,
                               std::string_view title,
                               std::string_view subtitle,
                               const DisplayOptions& options,
                               ViewSource& views,
                               const bool lazy = false,
                               const bool virtual_model = false) {
  HtmlBodyStyle table_style;
  table_style.format_numbers = true;
  // A browser view lays out every row of a long table, which is slow to open, sort and search;
  // past this many rows the rows are drawn by script, a window at a time.
  constexpr std::int64_t kBrowserVirtualRows = 2000;
  table_style.virtual_rows =
      options.backend == DisplayBackend::NativeWebView && frame.ncases() > kBrowserVirtualRows;
  auto body = virtual_model ? virtual_table_body_(frame)
                            : render_html_table_body_(frame, options, "plussmic-table", table_style);
  if (options.html_summary) {
    body = tabbed_body_(std::move(body), views, options, lazy);
  }
  return html_shell_(title, options.visit_editing ? std::string_view(kVisitInstruction) : subtitle, body, options,
                     options.visit_editing ? visit_inline_html_(options) : std::string{});
}

std::string render_html_atlas_(const Atlas& atlas, const DisplayOptions& options) {
  std::optional<Shelf> display_axis0;
  if (atlas.axis0_labels().has_value()) {
    display_axis0 = display_shelf(*atlas.axis0_labels());
  }

  std::string body;
  body += "<div class=\"summary-grid\">";
  body += pluss::tils::OutputEngine::format("<div class=\"summary-card\"><div class=\"label\">Depth</div><div class=\"value\">{}</div></div>",
                      atlas.depth());
  body += pluss::tils::OutputEngine::format("<div class=\"summary-card\"><div class=\"label\">Cases</div><div class=\"value\">{}</div></div>",
                      atlas.ncases());
  body += pluss::tils::OutputEngine::format("<div class=\"summary-card\"><div class=\"label\">Features</div><div class=\"value\">{}</div></div>",
                      atlas.nfeatures());
  body += pluss::tils::OutputEngine::format("<div class=\"summary-card\"><div class=\"label\">Depth Labels</div><div class=\"value\">{}</div></div>",
                      atlas.axis0_labels().has_value() ? escape_html_(logical_dtype(*atlas.axis0_labels()))
                                                       : "None");
  body += "</div>";

  if (atlas.depth() == 0) {
    body += table_frame_open_() + "<div style=\"padding:18px;\" class=\"muted\">Atlas is empty.</div>" + std::string(kTableFrameClose);
    return html_shell_(document_title_("Atlas", options),
                       "0 frames",
                       body,
                       options);
  }

  body += "<div class=\"section-label\">Frames</div>";
  body += "<div class=\"atlas-nav\">";
  body += "<button id=\"atlas-first\" type=\"button\" class=\"atlas-tab\">First</button>";
  body += "<button id=\"atlas-previous\" type=\"button\" class=\"atlas-tab\">Previous</button>";
  std::string first_label = "Frame 0";
  if (display_axis0.has_value()) {
    first_label = shelf_value_at(*display_axis0, 0);
  }
  body += pluss::tils::OutputEngine::format(
      "<span id=\"atlas-current-frame\" class=\"atlas-current-frame\" role=\"status\">{} (1 of {})</span>",
      escape_html_(first_label), atlas.depth());
  body += "<button id=\"atlas-next\" type=\"button\" class=\"atlas-tab\">Next</button>";
  body += "<button id=\"atlas-last\" type=\"button\" class=\"atlas-tab\">Last</button>";
  body += "<button id=\"atlas-show-all\" type=\"button\" class=\"atlas-tab\" aria-pressed=\"false\">All frames</button>";
  body += "</div>";
  body += "<div class=\"atlas-nav atlas-frame-tabs\" role=\"group\" aria-label=\"Select a frame\">";

  for (std::int64_t i = 0; i < atlas.depth(); ++i) {
    std::string label = pluss::tils::OutputEngine::format("Frame {}", i);
    if (display_axis0.has_value()) {
      label = shelf_value_at(*display_axis0, i);
    }
    body += pluss::tils::OutputEngine::format(
        "<button id=\"atlas-tab-{}\" type=\"button\" class=\"atlas-tab{}\" data-frame-index=\"{}\" aria-pressed=\"{}\">{}</button>",
        i, i == 0 ? " active" : "", i, i == 0 ? "true" : "false", escape_html_(label));
  }
  body += "</div>";

  for (std::int64_t i = 0; i < atlas.depth(); ++i) {
    std::string label = pluss::tils::OutputEngine::format("Frame {}", i);
    if (display_axis0.has_value()) {
      label = shelf_value_at(*display_axis0, i);
    }
    const auto frame = atlas.frame(i);
    body += pluss::tils::OutputEngine::format("<section class=\"atlas-panel{}\" data-frame-index=\"{}\">",
                        i == 0 ? " active" : "",
                        i);
    body += "<div class=\"atlas-frame-head\">";
    body += pluss::tils::OutputEngine::format("<h2 class=\"atlas-frame-title\">{}</h2>", escape_html_(label));
    body += pluss::tils::OutputEngine::format("<span class=\"pill\">frame {}</span>", i);
    body += "</div>";
    body += render_html_table_body_(frame, options, pluss::tils::OutputEngine::format("plussmic-table-{}", i));
    body += "</section>";
  }

  return html_shell_(document_title_("Atlas", options),
                     pluss::tils::OutputEngine::format("{} frames, {} cases, {} features", atlas.depth(), atlas.ncases(), atlas.nfeatures()),
                     body,
                     options);
}


std::string render_html_info_doc_(const DataFrame& frame, const DisplayOptions& options) {
  const auto cols = dataframe_columns_(frame);
  const auto display_cases = display_shelf(frame.labels().values());
  const auto names = frame.features();

  std::unordered_map<std::string, std::int64_t> dtype_counts;
  std::uint64_t memory_bytes = estimate_bytes(frame.labels().values());
  for (std::size_t i = 0; i < names.size(); ++i) {
    const auto dtype = logical_dtype(cols[i]);
    dtype_counts[dtype] += 1;
    memory_bytes += estimate_bytes(cols[i]);
  }

  const auto first_case = frame.ncases() > 0 ? shelf_value_at(display_cases, 0) : std::string("NULL");
  const auto last_case = frame.ncases() > 0 ? shelf_value_at(display_cases, frame.ncases() - 1) : std::string("NULL");

  std::string body;
  body += "<div class=\"summary-grid\">";
  body += pluss::tils::OutputEngine::format("<div class=\"summary-card\"><div class=\"label\">Class</div><div class=\"value\">{}</div></div>",
                      "pluss::table::DataFrame");
  body += pluss::tils::OutputEngine::format("<div class=\"summary-card\"><div class=\"label\">Cases</div><div class=\"value\">{}</div></div>",
                      frame.ncases());
  body +=
      pluss::tils::OutputEngine::format("<div class=\"summary-card\"><div class=\"label\">Features</div><div class=\"value\">{}</div></div>",
                  frame.nfeatures());
  body += pluss::tils::OutputEngine::format(
      "<div class=\"summary-card\"><div class=\"label\">Case Range</div><div class=\"value\">{} <span class=\"muted\">to</span> {}</div></div>",
      escape_html_(first_case),
      escape_html_(last_case));
  const auto null_case_rows = frame.nfeatures() == 0
                                  ? std::int64_t{0}
                                  : frame.isnull().any(Axis::Case).sum().as_series().host<std::int64_t>().front();
  body += pluss::tils::OutputEngine::format(
      "<div class=\"summary-card\"><div class=\"label\">Null Case Rows</div><div class=\"value\">{}</div></div>",
      null_case_rows);
  body += pluss::tils::OutputEngine::format("<div class=\"summary-card\"><div class=\"label\">PTypes</div><div class=\"value\">{}</div></div>",
                      escape_html_(dtype_summary_line(dtype_counts)));
  body += pluss::tils::OutputEngine::format("<div class=\"summary-card\"><div class=\"label\">Memory</div><div class=\"value\">{}</div></div>",
                      escape_html_(human_bytes(memory_bytes)));
  body += "</div>";

  body +=
      "<div style=\"padding:8px 6px 0;\"><span class=\"pill\">Feature inventory</span></div>";
  body += table_frame_open_();
  body += "<table id=\"plussmic-table\" data-filterable=\"true\"><thead><tr>";
  body += "<th class=\"sortable\" data-column-index=\"0\" data-kind=\"numeric\">#</th>";
  body += "<th class=\"sortable sticky-left\" data-column-index=\"1\" data-kind=\"text\">Feature</th>";
  body += "<th class=\"sortable\" data-column-index=\"2\" data-kind=\"numeric\">Non-Null Cases</th>";
  body += "<th class=\"sortable\" data-column-index=\"3\" data-kind=\"text\">PType</th>";
  body += "</tr></thead><tbody>";

  for (std::size_t i = 0; i < names.size(); ++i) {
    const auto nn = non_null_count(cols[i]);
    const auto dtype = logical_dtype(cols[i]);
    body += pluss::tils::OutputEngine::format(
        "<tr><td class=\"num\" data-sort-value=\"{0}\">{0}</td><td class=\"sticky-left\" data-sort-value=\"{1}\">{2}</td><td class=\"num\" data-sort-value=\"{3}\">{3}</td><td data-sort-value=\"{4}\">{5}</td></tr>",
        i,
        escape_html_(names[i]),
        escape_html_(names[i]),
        nn,
        escape_html_(dtype),
        escape_html_(dtype));
  }

  body += "</tbody></table></div></div>";
  return html_shell_(document_title_("DataFrame Info", options),
                     pluss::tils::OutputEngine::format("{} cases, {} features", frame.ncases(), frame.nfeatures()),
                     body,
                     options);
}

struct NumberFormatEntry {
  NumberFormat format;
  const char* id;
  const char* label;
};

constexpr std::array<NumberFormatEntry, 8> kNumberFormats{{
    {NumberFormat::Default, "default", "Default"},
    {NumberFormat::Decimals0, "0", "0 dp"},
    {NumberFormat::Decimals1, "1", "1 dp"},
    {NumberFormat::Decimals2, "2", "2 dp"},
    {NumberFormat::Decimals3, "3", "3 dp"},
    {NumberFormat::Decimals4, "4", "4 dp"},
    {NumberFormat::Thousands, "thousands", "Thousands"},
    {NumberFormat::Scientific, "scientific", "Scientific"},
}};

// The button id of the Table tab in the view picker (see render_html_table_).
constexpr std::string_view kTableTabId = "tab-table";

struct VisitEntry {
  DisplayVisit visit;
  const char* id;
  std::string label;
};

struct FilterEntry {
  DisplayFilter filter;
  const char* id;
  const char* label;
};

constexpr std::array<FilterEntry, 4> kFilters{{
    {DisplayFilter::Invalid, "invalid", "Invalid"},
    {DisplayFilter::Null, "null", "Null"},
    {DisplayFilter::NaN, "nan", "NaN"},
    {DisplayFilter::Infinite, "inf", "Inf"},
}};

bool show_html_window_(const std::string& html,
                       const DisplayOptions& options,
                       std::string_view title,
                       const DisplayBackend backend,
                       std::shared_ptr<const pluss::view::VirtualTableModel>
                           virtual_table_model = {},
                       const bool table_viewport_scroll_only = false,
                       std::function<std::string(const DisplayOptions&)> render_with = {},
                       const bool offer_number_format = false,
                       std::function<std::string(const DisplayOptions&, std::string_view)> render_panel_with = {},
                       const bool offer_filter = false,
                       std::function<void(const DisplayOptions&)> on_rows = {},
                       const bool offer_align = false,
                       const std::int64_t levels = 1) {
  if (backend == DisplayBackend::Terminal) {
    return false;
  }

  pluss::view::HtmlWindowOptions window_opts{};
  window_opts.title = native_window_title_(title);
  window_opts.snapshot_name = "table-view.png";
  window_opts.width = std::max<std::int64_t>(640, options.viewer_width);
  window_opts.height = std::max<std::int64_t>(420, options.viewer_height);
  window_opts.virtual_table_model = std::move(virtual_table_model);
  window_opts.table_viewport_scroll_only = table_viewport_scroll_only;
  if (render_with) {
    // Window menus re-render the page. Both menus edit one shared options state, so a
    // theme change keeps the chosen number format and the other way round.
    const auto state = std::make_shared<DisplayOptions>(options);
    if (render_panel_with) {
      // Fills a view the first time it is opened, with whatever theme and number format are current.
      window_opts.render_panel = [state, render_panel_with = std::move(render_panel_with)](std::string_view id) {
        return render_panel_with(*state, id);
      };
    }
    if (offer_number_format) {
      pluss::view::HtmlMenu menu;
      menu.id = "format";
      menu.title = "Format";
      menu.in_view_menu = true;  // a cascading entry inside the View menu
      for (const auto& entry : kNumberFormats) {
        menu.options.push_back(pluss::view::HtmlMenuOption{entry.id, entry.label});
      }
      menu.render = [state, render_with, on_rows](std::string_view id) {
        for (const auto& entry : kNumberFormats) {
          if (id == entry.id) {
            state->number_format = entry.format;
            if (on_rows) {
              on_rows(*state);
            }
            return render_with(*state);
          }
        }
        return std::string{};
      };
      window_opts.menus.push_back(std::move(menu));
    }
    if (offer_filter) {
      pluss::view::HtmlMenu menu;
      menu.id = "filter";
      menu.title = "Filter";
      menu.in_view_menu = true;  // a cascading entry inside the View menu
      for (const auto& entry : kFilters) {
        menu.options.push_back(pluss::view::HtmlMenuOption{entry.id, entry.label});
      }
      // Choosing the filter that is already on turns it off, so there is no entry that means "everything".
      menu.render = [state, render_with, on_rows](std::string_view id) {
        for (const auto& entry : kFilters) {
          if (id == entry.id) {
            state->filter = state->filter == entry.filter ? DisplayFilter::None : entry.filter;
            if (on_rows) {
              on_rows(*state);
            }
            return render_with(*state);
          }
        }
        return std::string{};
      };
      menu.selected = [state]() {
        for (const auto& entry : kFilters) {
          if (state->filter == entry.filter) {
            return std::string(entry.id);
          }
        }
        return std::string("off");  // matches no entry, so none is checked
      };
      window_opts.menus.push_back(std::move(menu));
      // Going back to the Table view takes the filter and the visit off, so the table is the plain one again.
      window_opts.on_view = [state, render_with, on_rows](std::string_view button_id) {
        if (button_id != kTableTabId ||
            (state->filter == DisplayFilter::None && state->visit == DisplayVisit::All && !state->visit_editing &&
             state->align == DisplayAlign::Head)) {
          return std::string{};
        }
        state->filter = DisplayFilter::None;
        state->visit = DisplayVisit::All;
        state->visit_editing = false;
        state->align = DisplayAlign::Head;
        if (on_rows) {
          on_rows(*state);
        }
        return render_with(*state);
      };
    }
    {
      // View > Visit: the first, middle or last rows, or a range typed into the box that appears. Choosing the
      // active entry again turns it off, as in Filter.
      pluss::view::HtmlMenu menu;
      menu.id = "visit";
      menu.title = "Visit";
      menu.in_view_menu = true;
      const auto visits = std::make_shared<std::array<VisitEntry, 4>>(std::array<VisitEntry, 4>{{
          {DisplayVisit::First, "first", pluss::tils::OutputEngine::format("First {}", state->visit_rows)},
          {DisplayVisit::Middle, "middle", pluss::tils::OutputEngine::format("Middle {}", state->visit_rows)},
          {DisplayVisit::Last, "last", pluss::tils::OutputEngine::format("Last {}", state->visit_rows)},
          {DisplayVisit::Custom, "custom", "Custom..."},
      }});
      for (const auto& entry : *visits) {
        pluss::view::HtmlMenuOption option{entry.id, entry.label};
        if (entry.visit == DisplayVisit::Custom) {
          // Asked for in a dialog where the window can show one; elsewhere the page offers the boxes (see render).
          option.prompt_title = "Visit rows";
          option.prompt_message = "Positions count from 0 and both ends are included: 3 to 8 shows rows 3 through 8. Leave a box blank for the start or the end.";
          option.prompt_accept = "Apply";
          option.prompt = {{"From", "0"}, {"To", "1"}};
        }
        menu.options.push_back(std::move(option));
      }
      menu.prompt_values = [state](std::string_view) {
        const auto text = [](const std::int64_t value, const std::int64_t unset) {
          return value == unset ? std::string{} : std::to_string(value);
        };
        return std::vector<std::string>{text(state->visit_begin, 0),
                                        text(state->visit_end, std::numeric_limits<std::int64_t>::max())};
      };
      menu.render_prompted = [state, render_with, on_rows](std::string_view, const std::vector<std::string>& values) {
        const auto read = [&](const std::size_t at, const std::int64_t fallback) {
          if (at >= values.size() || values[at].empty()) {
            return fallback;
          }
          std::int64_t value = fallback;
          std::from_chars(values[at].data(), values[at].data() + values[at].size(), value);
          return value;
        };
        state->visit = DisplayVisit::Custom;
        state->visit_editing = false;
        state->visit_begin = read(0, 0);
        state->visit_end = read(1, std::numeric_limits<std::int64_t>::max());
        if (on_rows) {
          on_rows(*state);
        }
        return render_with(*state);
      };
      menu.render = [state, render_with, on_rows, visits](std::string_view id) {
        for (const auto& entry : *visits) {
          if (id == entry.id) {
            if (entry.visit == DisplayVisit::Custom) {
              state->visit_editing = true;  // open the box; the range applies when Apply is pressed
            } else {
              state->visit = state->visit == entry.visit ? DisplayVisit::All : entry.visit;
              state->visit_editing = false;
            }
            if (on_rows) {
              on_rows(*state);
            }
            return render_with(*state);
          }
        }
        return std::string{};
      };
      menu.selected = [state, visits]() {
        for (const auto& entry : *visits) {
          if (state->visit == entry.visit || (state->visit_editing && entry.visit == DisplayVisit::Custom)) {
            return std::string(entry.id);
          }
        }
        return std::string("off");
      };
      window_opts.menus.push_back(std::move(menu));
      // Apply on the custom range: the two boxes hold positions, an empty one meaning the start or the end.
      window_opts.on_action = [state, render_with, on_rows](std::string_view button_id,
                                                           const std::unordered_map<std::string, std::string>& inputs) {
        if (button_id != kVisitApplyId) {
          return std::string{};
        }
        const auto read = [&](const char* key, const std::int64_t fallback) {
          const auto it = inputs.find(key);
          if (it == inputs.end() || it->second.empty()) {
            return fallback;
          }
          std::int64_t value = fallback;
          const auto& text = it->second;
          std::from_chars(text.data(), text.data() + text.size(), value);
          return value;
        };
        state->visit = DisplayVisit::Custom;
        state->visit_editing = false;  // the range is set: the box goes away until Custom is chosen again
        state->visit_begin = read(kVisitBeginId, 0);
        state->visit_end = read(kVisitEndId, std::numeric_limits<std::int64_t>::max());
        if (on_rows) {
          on_rows(*state);
        }
        return render_with(*state);
      };
    }
    if (offer_align) {
      // View > Align: where keys of different lengths sit against each other. One of the two always applies.
      pluss::view::HtmlMenu menu;
      menu.id = "align";
      menu.title = "Align";
      menu.in_view_menu = true;
      menu.options.push_back(pluss::view::HtmlMenuOption{"head", "Head: every key from the top"});
      menu.options.push_back(pluss::view::HtmlMenuOption{"tail", "Tail: every key to the bottom"});
      menu.render = [state, render_with, on_rows](std::string_view id) {
        state->align = id == "tail" ? DisplayAlign::Tail : DisplayAlign::Head;
        if (on_rows) {
          on_rows(*state);
        }
        return render_with(*state);
      };
      menu.selected = [state]() { return std::string(state->align == DisplayAlign::Tail ? "tail" : "head"); };
      window_opts.menus.push_back(std::move(menu));
    }
    if (levels > 1) {
      // Level: which table of a stack (an atlas's frame, an archive's catalog, an axel's registry) the window shows.
      // The nearest ones are a click away; any other is asked for by number, since a stack can be hundreds deep.
      pluss::view::HtmlMenu menu;
      menu.id = "level";
      menu.title = "Level";
      menu.options.push_back(pluss::view::HtmlMenuOption{"first", "First"});
      menu.options.push_back(pluss::view::HtmlMenuOption{"previous", "Previous"});
      menu.options.push_back(pluss::view::HtmlMenuOption{"next", "Next"});
      menu.options.push_back(pluss::view::HtmlMenuOption{"last", "Last"});
      pluss::view::HtmlMenuOption go{"goto", "Go to..."};
      go.prompt_title = "Go to level";
      go.prompt_message = pluss::tils::OutputEngine::format("Levels count from 0 to {}.", levels - 1);
      go.prompt_accept = "Go";
      go.prompt = {{"Level", "0"}};
      menu.options.push_back(std::move(go));
      const auto move_to = [state, render_with, on_rows, levels](const std::int64_t level) {
        state->level = std::clamp<std::int64_t>(level, 0, levels - 1);
        if (on_rows) {
          on_rows(*state);
        }
        return render_with(*state);
      };
      menu.prompt_values = [state](std::string_view) { return std::vector<std::string>{std::to_string(state->level)}; };
      menu.render_prompted = [move_to, state](std::string_view, const std::vector<std::string>& values) {
        std::int64_t level = state->level;
        if (!values.empty() && !values.front().empty()) {
          std::from_chars(values.front().data(), values.front().data() + values.front().size(), level);
        }
        return move_to(level);
      };
      menu.render = [move_to, state, levels](std::string_view id) {
        if (id == "first") return move_to(0);
        if (id == "previous") return move_to(state->level - 1);
        if (id == "next") return move_to(state->level + 1);
        if (id == "last") return move_to(levels - 1);
        return std::string{};
      };
      menu.selected = []() { return std::string("off"); };
      window_opts.menus.push_back(std::move(menu));
    }
    {
      pluss::view::HtmlMenu menu;
      menu.id = "theme";
      menu.title = "Theme";
      for (const auto& palette : kThemes) {
        if (palette.theme == DisplayTheme::Native) {
          continue;  // only used for Apple's native web view
        }
        menu.options.push_back(pluss::view::HtmlMenuOption{palette.name, palette.label});
      }
      menu.render = [state, render_with](std::string_view id) {
        if (!parse_theme(std::string(id), state->theme)) {
          return std::string{};
        }
        return render_with(*state);
      };
      window_opts.menus.push_back(std::move(menu));
    }
  }
  switch (backend) {
    case DisplayBackend::PlussWebView:
      window_opts.backend_preference = pluss::view::BackendPreference::PlussWebViewOnly;
      break;
    case DisplayBackend::NativeWebView:
      window_opts.backend_preference = pluss::view::BackendPreference::NativeWebView;
      break;
    case DisplayBackend::Auto:
      window_opts.backend_preference = pluss::view::BackendPreference::Auto;
      break;
    case DisplayBackend::Terminal:
      return false;
  }
  return pluss::view::show_html_window(html, window_opts);
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

DisplayTheme default_theme() {
  return g_default_theme.load(std::memory_order_relaxed);
}

void default_theme(DisplayTheme theme) {
  g_default_theme.store(theme == DisplayTheme::Auto ? DisplayTheme::Default : theme, std::memory_order_relaxed);
}

const char* theme_name(DisplayTheme theme) {
  return palette_for_(theme == DisplayTheme::Auto ? default_theme() : theme).name;
}

bool parse_theme(const std::string& name, DisplayTheme& out) {
  std::string lower = name;
  for (auto& ch : lower) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  for (const auto& palette : kThemes) {
    if (lower == palette.name) {
      out = palette.theme;
      return true;
    }
  }
  return false;
}

void default_backend(DisplayBackend backend) {
  g_default_backend.store(backend, std::memory_order_relaxed);
}

bool gui_available() {
  return runtime_gui_env_available_();
}

std::string render(const DataFrame& frame, const DisplayOptions& options) {
  const auto term = detect_terminal();
  const auto effective = resolve_effective_options(options, term, false);
  const auto terminal_width = effective.terminal_width > 0 ? effective.terminal_width : term.columns;

  auto working = effective;
  bool compact = false;
  auto matrix = build_dataframe_matrix(frame, working, compact);

  while (estimate_matrix_width(matrix, working) > terminal_width) {
    if (!compact && frame.nfeatures() > 1) {
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
    out += pluss::tils::OutputEngine::format("\n[{} cs x {} feat]", frame.ncases(), frame.nfeatures());
  }
  return out;
}

std::string render(const Shelf& shelf, const DisplayOptions& options) {
  const auto term = detect_terminal();
  const auto effective = resolve_effective_options(options, term, false);
  const auto terminal_width = effective.terminal_width > 0 ? effective.terminal_width : term.columns;
  return render_shelf_series_like_(shelf, effective, terminal_width);
}

std::string render_html(const DataFrame& frame, const DisplayOptions& options) {
  StatsViews views(frame);
  const auto shown = shown_rows_(frame, options);
  return render_html_table_(shown.frame, document_title_("DataFrame", options), shown.subtitle, options, views);
}

std::string render_html(const Shelf& shelf, const DisplayOptions& options) {
  const auto title = document_title_(shelf.named() ? shelf.name() : "Shelf", options);
  const auto subtitle = pluss::tils::OutputEngine::format("{} cases, dtype {}", shelf.size(), shelf.dtype());
  return render_html_pre_(title, subtitle, render(shelf, options), options);
}


pluss::view::RowWindow row_window_for(const DisplayOptions& options) {
  return row_window_for_(options);
}

std::string format_number(const std::string& text, const NumberFormat format) {
  return format == NumberFormat::Default ? text : format_number_cell_(text, /*count_like=*/false, format);
}

std::string render_html_model_(const ModelPage& page, CatalogViews& views, const DisplayOptions& options) {
  // A stack's page says what the level shown is; any other page says the same whatever the options.
  std::string subtitle = page.subtitle;
  auto cards = page.cards;
  if (page.describe) {
    page.describe(options, subtitle, cards);
  }
  const std::string body =
      tabbed_body_(summary_cards_html_(cards) + virtual_table_frame_(), views, options, /*lazy=*/true);
  return html_shell_(page.title, options.visit_editing ? std::string_view(kVisitInstruction) : std::string_view(subtitle),
                     body, options, options.visit_editing ? visit_inline_html_(options) : std::string{});
}

std::string render_html(const Atlas& atlas, const DisplayOptions& options) {
  return render_html_atlas_(atlas, options);
}

std::string render_info(const DataFrame& frame, const DisplayOptions& options) {
  const auto term = detect_terminal();
  const auto effective = resolve_effective_options(options, term, true);
  const auto terminal_width = effective.terminal_width > 0 ? effective.terminal_width : term.columns;

  const auto cols = dataframe_columns_(frame);
  const auto display_cases = display_shelf(frame.labels().values());
  const auto names = frame.features();

  std::unordered_map<std::string, std::int64_t> dtype_counts;
  std::uint64_t memory_bytes = estimate_bytes(frame.labels().values());

  for (std::size_t i = 0; i < names.size(); ++i) {
    const auto dtype = logical_dtype(cols[i]);
    dtype_counts[dtype] += 1;
    memory_bytes += estimate_bytes(cols[i]);
  }

  const auto first_case = frame.ncases() > 0 ? shelf_value_at(display_cases, 0) : std::string("NULL");
  const auto last_case = frame.ncases() > 0 ? shelf_value_at(display_cases, frame.ncases() - 1) : std::string("NULL");

  TableMatrix summary;
  summary.headers = {"Field", "Value"};
  summary.right_align = {false, false};
  summary.min_widths = {5, 8};
  const auto null_case_rows = frame.nfeatures() == 0
                                  ? std::int64_t{0}
                                  : frame.isnull().any(Axis::Case).sum().as_series().host<std::int64_t>().front();
  summary.rows = {
      {"Class", "pluss::table::DataFrame"},
      {"Cases", pluss::tils::OutputEngine::format("{}", frame.ncases())},
      {"Features", pluss::tils::OutputEngine::format("{}", frame.nfeatures())},
      {"Case Range", pluss::tils::OutputEngine::format("{} -> {}", first_case, last_case)},
      {"Null Case Rows", pluss::tils::OutputEngine::format("{}", null_case_rows)},
      {"PTypes", dtype_summary_line(dtype_counts)},
      {"Memory", human_bytes(memory_bytes)},
  };

  TableMatrix details;
  details.headers = {"#", "Feature", "Non-Null Cases", "PType"};
  details.right_align = {false, false, false, false};
  details.min_widths = {2, 6, 6, 5};
  const auto detail_plan = build_row_plan(static_cast<std::int64_t>(names.size()), effective);
  details.rows.reserve(detail_plan.head.size() + detail_plan.tail.size() + (detail_plan.ellipsis ? 1 : 0));

  auto push_detail_row = [&](std::size_t i) {
    const auto nn = non_null_count(cols[i]);
    details.rows.push_back({
        pluss::tils::OutputEngine::format("{}", i),
        names[i],
        pluss::tils::OutputEngine::format("{}", nn),
        logical_dtype(cols[i]),
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

std::string info_html(const DataFrame& frame, const DisplayOptions& options) {
  return render_html_info_doc_(frame, options);
}

static StackFacts atlas_facts_(const Atlas& atlas);

std::string render(const Atlas& atlas, const DisplayOptions& options) { return render_stack_facts(atlas_facts_(atlas), options); }

// ---------------------------------------------------------------------------
// The table as a notebook shows it: a plain HTML table, not PlussView's page. A notebook has no terminal to wrap
// wide tables in, so it needs none of PlussView's window chrome; the output is a fragment whose styles are scoped to
// it, it takes the notebook's own colours (light or dark), and it is exactly as tall as its rows.
// ---------------------------------------------------------------------------

// A long frame is shown as its first and last rows with a row of dots between, so the output stays a page long
// however many cases there are; the footer says how many there are in all.
constexpr std::int64_t kNotebookRowLimit = 60;  // up to this many cases, every one is shown
constexpr std::int64_t kNotebookEdgeRows = 5;   // beyond it, this many from each end

// The look is the one a notebook reader knows from a data-frame table: no vertical rules, one rule under the header,
// everything right-aligned, a bold case column, alternate rows tinted and the row under the pointer highlighted.
// Every colour is here; the text and the background are the notebook's own.
constexpr const char* kNotebookStyle =
    ".pluss-table{--pluss-rule:rgba(128,128,128,.45);--pluss-stripe:rgba(128,128,128,.08);"
    "--pluss-hover:rgba(128,128,128,.18);--pluss-null:#d6453d;--pluss-muted:rgba(128,128,128,.9);"
    "display:block;overflow-x:auto;margin:.25em 0}"
    ".pluss-table table{border-collapse:collapse;border:none;font-size:.92em;font-variant-numeric:tabular-nums}"
    ".pluss-table th,.pluss-table td{padding:.45em .7em;border:none;text-align:right;white-space:nowrap;vertical-align:middle}"
    ".pluss-table thead{border-bottom:1px solid var(--pluss-rule);vertical-align:bottom}"
    ".pluss-table th{font-weight:700}"
    ".pluss-table tbody tr:nth-child(even){background:var(--pluss-stripe)}"
    ".pluss-table tbody tr:hover{background:var(--pluss-hover)}"
    ".pluss-table .null{font-weight:700;color:var(--pluss-null)}"
    ".pluss-table .gap td{text-align:center;color:var(--pluss-muted)}"
    ".pluss-table .foot{margin-top:.3em;color:var(--pluss-muted);font-size:.85em}";

// The notebook table of any model PlussView can paint (a frame, a catalog's ragged grid, a registry's): the header, the
// rows (the first and last few of a long one), blank where a column has no entry, and the footer line. Only the rows
// that are shown are read from the model.
std::string notebook_model_html_(const pluss::view::VirtualTableModel& model, const std::string& footer) {
  const std::int64_t rows = model.row_count();
  const std::int64_t columns = model.column_count();
  std::string html = std::string("<div class=\"pluss-table\"><style>") + kNotebookStyle + "</style><table><thead><tr><th>" +
                     escape_html_(model.row_header()) + "</th>";
  for (std::int64_t c = 0; c < columns; ++c) {
    html += "<th>" + escape_html_(model.column_name(c)) + "</th>";
  }
  html += "</tr></thead><tbody>";
  const bool cut = rows > kNotebookRowLimit;
  const auto write_row = [&](const std::int64_t row) {
    html += "<tr><th>" + escape_html_(model.row_label(row)) + "</th>";
    for (std::int64_t c = 0; c < columns; ++c) {
      const auto cell = model.cell(row, c);
      if (cell.absent) {
        html += "<td></td>";  // the column has no entry here: nothing is drawn, and it is not a NULL
      } else {
        html += std::string(cell.missing ? "<td class=\"null\">" : "<td>") + escape_html_(cell.text) + "</td>";
      }
    }
    html += "</tr>";
  };
  for (std::int64_t row = 0; row < rows; ++row) {
    if (cut && row >= kNotebookEdgeRows && row < rows - kNotebookEdgeRows) {
      if (row == kNotebookEdgeRows) {
        html += "<tr class=\"gap\"><td colspan=\"" + std::to_string(columns + 1) + "\">&hellip;</td></tr>";
      }
      continue;
    }
    write_row(row);
  }
  return html + "</tbody></table><div class=\"foot\">" + footer + "</div></div>";
}

// An overview (a stack's levels, say) as a model: the first column of the overview labels the rows.
class SummaryGridModel final : public pluss::view::VirtualTableModel {
 public:
  explicit SummaryGridModel(const SummaryView& view) : view_(view) {}
  std::int64_t row_count() const noexcept override { return view_.size; }
  std::int64_t column_count() const noexcept override { return static_cast<std::int64_t>(view_.columns.size()) - 1; }
  std::string row_header() const override { return view_.columns.empty() ? std::string{} : view_.columns.front(); }
  std::string row_label(const std::int64_t row) const override { return row_at_(row).front(); }
  std::string column_name(const std::int64_t column) const override { return view_.columns[static_cast<std::size_t>(column) + 1]; }
  bool column_numeric(std::int64_t) const noexcept override { return false; }
  pluss::view::VirtualTableCell cell(const std::int64_t row, const std::int64_t column) const override {
    return pluss::view::VirtualTableCell{.text = row_at_(row)[static_cast<std::size_t>(column) + 1]};
  }
  bool row_matches(std::int64_t, const std::string_view) const override { return true; }

 private:
  // The row last asked for is kept, since a notebook reads a row's cells one after another.
  const std::vector<std::string>& row_at_(const std::int64_t row) const {
    if (row != cached_row_) {
      cached_ = view_.row(row);
      cached_.resize(view_.columns.size());
      cached_row_ = row;
    }
    return cached_;
  }
  const SummaryView& view_;
  mutable std::int64_t cached_row_{-1};
  mutable std::vector<std::string> cached_{};
};

std::string notebook_table_html_(const DataFrame& frame) {
  const DataFrameVirtualTableModel model(frame);
  return notebook_model_html_(model, pluss::tils::OutputEngine::format("{} cases &times; {} features", model.row_count(), model.column_count()));
}

// Inside the PlussII notebook the kernel sets PLUSS_NOTEBOOK; a table is then handed back to the notebook as
// HTML (the marker line the kernel turns into a display message) instead of opening a window.
bool in_notebook() {
  const char* flag = std::getenv("PLUSS_NOTEBOOK");
  return flag != nullptr && *flag != '\0';
}

static bool show_in_notebook_(const std::string& html) {
  if (!in_notebook()) {
    return false;
  }
  std::cout << "\x1e<<pluss-mime text/html " << html.size() << ">>\n" << html << std::flush;
  return true;
}

// The page of a DataFrame as a window (or a picture) shows it: the table painted from the model, with the subtitle
// saying how many rows the filter and the visit leave.
std::string frame_page_(const DataFrame& frame, StatsViews& views, const std::shared_ptr<DataFrameVirtualTableModel>& model,
                        const DisplayOptions& options, const bool lazy) {
  const std::int64_t visible = model->visible_rows();
  const auto subtitle = visible != frame.ncases()
                            ? pluss::tils::OutputEngine::format("{} of {} cases, {} features", visible, frame.ncases(), frame.nfeatures())
                            : pluss::tils::OutputEngine::format("{} cases, {} features", frame.ncases(), frame.nfeatures());
  return render_html_table_(frame, document_title_("DataFrame", options), subtitle, options, views, lazy,
                            /*virtual_model=*/true);
}

namespace {
bool save_page_png_(const std::string& html, const pluss::view::VirtualTableModel* model, const std::string& path,
                    const int width, const int height) {
  namespace window = pluss::view::window;
  const auto document = window::parse_html_document(html);
  window::HtmlRenderConfig config{};
  config.virtual_table_model = model;
  std::vector<std::int64_t> rows;
  if (model != nullptr && (model->filtering() || model->row_window().kind != pluss::view::RowWindow::Kind::All)) {
    for (std::int64_t row = 0; row < model->row_count(); ++row) {
      if (model->row_allowed(row)) {
        rows.push_back(row);
      }
    }
    const auto [first, last] = pluss::view::window_bounds(model->row_window(), static_cast<std::int64_t>(rows.size()));
    rows = std::vector<std::int64_t>(rows.begin() + first, rows.begin() + last);
    config.virtual_row_order = &rows;
  }
  const window::SizeI size{width, height};
  const auto scene = window::build_html_document_scene(document, size, config);
  window::PixelCanvas canvas(size);
  canvas.clear(window::Color::rgba(0, 0, 0, 255));
  window::SoftwareRenderer renderer;
  std::vector<window::RenderEvent> events;
  renderer.render(scene, canvas, events);
  return window::save_canvas_png(canvas, path);
}
}  // namespace

bool save_png(const ModelPage& page, const std::string& path, const DisplayOptions& options, const int width,
              const int height, const std::string& view) {
  if (page.on_options) {
    page.on_options(options);
  }
  std::string html = render_html_model(page, options);
  if (!view.empty() && view != "table") {
    // The page holds every view; the one that is active is the one drawn.
    const auto replace_first = [&html](const std::string& from, const std::string& to) {
      if (const auto at = html.find(from); at != std::string::npos) {
        html.replace(at, from.size(), to);
      }
    };
    replace_first("<section id=\"panel-table\" class=\"tab-panel active\"", "<section id=\"panel-table\" class=\"tab-panel\"");
    // The picker names the view that is open.
    const std::string target = "data-tab-target=\"panel-" + view + "\"";
    if (const auto at = html.find(target); at != std::string::npos) {
      const auto open = html.find('>', at);
      const auto close = html.find("</button>", open);
      if (open != std::string::npos && close != std::string::npos) {
        replace_first("<span id=\"view-current\">Table</span>",
                      "<span id=\"view-current\">" + html.substr(open + 1, close - open - 1) + "</span>");
      }
    }
    replace_first("<section id=\"panel-" + view + "\" class=\"tab-panel\"",
                  "<section id=\"panel-" + view + "\" class=\"tab-panel active\"");
  }
  return save_page_png_(html, page.model.get(), path, width, height);
}

bool save_png(const DataFrame& frame, const std::string& path, const DisplayOptions& options, const int width,
              const int height) {
  StatsViews views(frame);
  const auto model = std::make_shared<DataFrameVirtualTableModel>(frame);
  model->apply(options);
  const auto html = frame_page_(frame, views, model, options, /*lazy=*/false);
  return save_page_png_(html, model.get(), path, width, height);
}

std::string render_notebook_html(const pluss::view::VirtualTableModel& model, const std::string& footer) {
  return notebook_model_html_(model, footer);
}

std::string render_notebook_summary(const SummaryView& view, const std::string& footer) {
  const SummaryGridModel model(view);
  return notebook_model_html_(model, footer);
}

bool show_in_notebook(const std::string& html) { return show_in_notebook_(html); }

std::string render_notebook_html(const Atlas& atlas) { return render_notebook_stack_facts(atlas_facts_(atlas)); }

std::string render_notebook_html(const DataFrame& frame) {
  return notebook_table_html_(frame);
}

void show(const DataFrame& frame, const DisplayOptions& options) {
  if (show_in_notebook_(notebook_table_html_(frame))) {
    return;
  }
  const auto backend = resolve_backend_(options);
  if (backend != DisplayBackend::Terminal) {
    auto render_opts = options;
    render_opts.backend = backend;
    const std::int64_t rows = std::max<std::int64_t>(0, frame.ncases());
    const std::int64_t columns = std::max<std::int64_t>(0, frame.nfeatures());
    // PlussView paints every DataFrame table from the frame itself, at any size: one table, with the same
    // features, whether it has five rows or five million. Only a browser view lays the rows out as HTML.
    const bool use_virtual_table = backend == DisplayBackend::PlussWebView && rows > 0 && columns > 0;
    std::shared_ptr<const pluss::view::VirtualTableModel> virtual_model;
    const DataFrame frame_copy = frame;
    // PlussView computes each profiling view when it is first opened and keeps the result, so
    // opening the window is not delayed by views nobody uses and a theme or number-format change
    // only re-renders. (A browser view builds every view up front.)
    const bool in_house = backend == DisplayBackend::PlussWebView;
    const auto views = std::make_shared<StatsViews>(frame_copy);
    std::shared_ptr<DataFrameVirtualTableModel> filterable_model;
    if (use_virtual_table) {
      filterable_model = std::make_shared<DataFrameVirtualTableModel>(frame);
      filterable_model->apply(render_opts);
      virtual_model = filterable_model;
    }
    const auto render_with = [frame_copy, views, filterable_model, use_virtual_table, in_house](const DisplayOptions& themed) {
      // A table in a window is painted from its model, which holds the filter and the visit; a browser view lays out
      // the rows that remain.
      if (use_virtual_table) {
        return frame_page_(frame_copy, *views, filterable_model, themed, /*lazy=*/in_house);
      }
      const auto shown = shown_rows_(frame_copy, themed);
      return render_html_table_(shown.frame, document_title_("DataFrame", themed), shown.subtitle, themed, *views,
                                /*lazy=*/in_house, /*virtual_model=*/false);
    };
    const auto html = render_with(render_opts);
    std::function<std::string(const DisplayOptions&, std::string_view)> render_panel_with;
    if (in_house) {
      render_panel_with = [views](const DisplayOptions& current, const std::string_view id) {
        auto panel = views->render(std::string(id), current);
        return panel.empty()
                   ? std::string("<div class=\"muted\" style=\"padding:18px;\">This view is not available for this frame.</div>")
                   : panel;
      };
    }
    if (show_html_window_(html,
                          render_opts,
                          "Pluss DataFrame",
                          backend,
                          std::move(virtual_model),
                          backend == DisplayBackend::PlussWebView,
                          in_house ? std::function<std::string(const DisplayOptions&)>(render_with)
                                   : std::function<std::string(const DisplayOptions&)>{},
                          /*offer_number_format=*/in_house,
                          std::move(render_panel_with),
                          /*offer_filter=*/in_house,
                          [filterable_model](const DisplayOptions& current) {
                            if (filterable_model != nullptr) {
                              filterable_model->apply(current);
                            }
                          })) {
      return;
    }
  }
  print(std::cout, frame, options);
}

void show(const Shelf& shelf, const DisplayOptions& options) {
  // A shelf is not a table: a notebook shows it the way the terminal does, as the text it is, not as a window or a
  // grid.
  if (in_notebook()) {
    print(std::cout, shelf, options);
    return;
  }
  const auto backend = resolve_backend_(options);
  if (backend != DisplayBackend::Terminal) {
    auto render_opts = options;
    render_opts.backend = backend;
    const auto html = render_html(shelf, render_opts);
    if (show_html_window_(html, render_opts, "PlussMic Shelf", backend)) {
      return;
    }
  }
  print(std::cout, shelf, options);
}

// An atlas in the PlussView window: one frame at a time, as a table of its own (see StackPage.h).
static ModelPage atlas_page_(const Atlas& atlas) {
  const Atlas held = atlas;
  std::optional<Shelf> labels;
  if (atlas.axis0_labels().has_value()) {
    labels = display_shelf(*atlas.axis0_labels());
  }
  StackSource source;
  source.title = "Atlas";
  source.unit = "frame";
  source.depth = atlas.depth();
  source.label = [labels](const std::int64_t i) { return labels.has_value() ? shelf_value_at(*labels, i) : std::string{}; };
  source.table = [held](const std::int64_t i) -> std::shared_ptr<OptionsModel> {
    return std::make_shared<DataFrameVirtualTableModel>(held.frame(i));
  };
  source.cards = [held](const std::int64_t i) {
    const auto frame = held.frame(i);
    return std::vector<std::pair<std::string, std::string>>{{"Cases", std::to_string(frame.ncases())},
                                                              {"Features", std::to_string(frame.nfeatures())}};
  };
  return stack_model_page(std::move(source));
}

void show(const Atlas& atlas, const DisplayOptions& options) {
  if (show_in_notebook_(render_notebook_html(atlas))) {
    return;
  }
  if (atlas.depth() > 0 && show_model(atlas_page_(atlas), options)) {
    return;
  }
  print(std::cout, atlas, options);
}

std::string render_html_model(const ModelPage& page, const DisplayOptions& options) {
  std::vector<CatalogViews::Stream> streams;
  for (const auto& [name, shelf] : page.streams) {
    streams.emplace_back(name, shelf);
  }
  CatalogViews views(std::move(streams), page.part_word, page.entries_word);
  const std::string body =
      tabbed_body_(summary_cards_html_(page.cards) + virtual_table_frame_(), views, options, /*lazy=*/false);
  return html_shell_(page.title, page.subtitle, body, options);
}

bool show_model(const ModelPage& page, const DisplayOptions& options) {
  const auto backend = resolve_backend_(options);
  if (backend != DisplayBackend::PlussWebView || page.model == nullptr) {
    return false;  // a model-painted table needs PlussView's own window
  }
  auto render_opts = options;
  render_opts.backend = backend;
  std::vector<CatalogViews::Stream> streams;
  for (const auto& [name, shelf] : page.streams) {
    streams.emplace_back(name, shelf);
  }
  const auto views = std::make_shared<CatalogViews>(std::move(streams), page.part_word, page.entries_word);
  const auto render_with = [page, views](const DisplayOptions& current) { return render_html_model_(page, *views, current); };
  const auto render_panel_with = [views](const DisplayOptions& current, const std::string_view id) {
    auto panel = views->render(std::string(id), current);
    return panel.empty() ? std::string("<div class=\"muted\" style=\"padding:18px;\">This view is not available for this catalog.</div>")
                         : panel;
  };
  if (page.on_options) {
    page.on_options(render_opts);
  }
  return show_html_window_(render_with(render_opts), render_opts, "Pluss " + page.title, backend, page.model,
                           /*table_viewport_scroll_only=*/true, render_with, /*offer_number_format=*/true,
                           render_panel_with, /*offer_filter=*/true, page.on_options, page.offer_align, page.levels);
}


void show_info(const DataFrame& frame, const DisplayOptions& options) {
  const auto backend = resolve_backend_(options);
  if (backend != DisplayBackend::Terminal) {
    auto render_opts = options;
    render_opts.backend = backend;
    const auto html = info_html(frame, render_opts);
    if (show_html_window_(html, render_opts, "Pluss DataFrame", backend)) {
      return;
    }
  }
  print_info(std::cout, frame, options);
}

std::string render_sequence(const SequenceView& view, const DisplayOptions& options) {
  const auto term = detect_terminal();
  const auto effective = resolve_effective_options(options, term, false);
  const auto width = effective.terminal_width > 0 ? effective.terminal_width : term.columns;
  return sequence_text_(view, effective, width);
}

void print_sequence(std::ostream& os, const SequenceView& view, const DisplayOptions& options) {
  os << render_sequence(view, options) << '\n';
}

std::string render_summary(const SummaryView& view, const DisplayOptions& options) {
  const auto term = detect_terminal();
  const auto effective = resolve_effective_options(options, term, false);
  const auto width = effective.terminal_width > 0 ? effective.terminal_width : term.columns;
  return summary_text_(view, effective, width);
}

void print_summary(std::ostream& os, const SummaryView& view, const DisplayOptions& options) {
  os << render_summary(view, options) << '\n';
}

// The first and last values of a sequence on one line, "[a, b, c, ..., z]", for a column that previews what a part
// holds. How many are shown at each end is named here, once, for every overview.
constexpr std::int64_t kPreviewHead = 3;
constexpr std::int64_t kPreviewTail = 1;

std::string preview_text(const std::int64_t size, const std::function<std::string(std::int64_t)>& value,
                         const std::int64_t max_width) {
  std::vector<std::string> parts;
  const bool cut = size > kPreviewHead + kPreviewTail;
  const std::int64_t head = cut ? kPreviewHead : size;
  for (std::int64_t i = 0; i < head; ++i) {
    parts.push_back(value(i));
  }
  if (cut) {
    parts.emplace_back("...");
    for (std::int64_t i = size - kPreviewTail; i < size; ++i) {
      parts.push_back(value(i));
    }
  }
  return clip_cell("[" + pluss::tils::OutputEngine::join(parts, ", ") + "]", max_width);
}

std::string render_stack_facts(const StackFacts& stack, const DisplayOptions& options) {
  SummaryView view;
  view.title = stack.type;
  view.columns = {"fact", "value"};
  view.size = static_cast<std::int64_t>(stack.facts.size());
  view.row = [&stack](const std::int64_t i) {
    const auto& fact = stack.facts[static_cast<std::size_t>(i)];
    return std::vector<std::string>{fact.first, fact.second};
  };
  return render_summary(view, options);
}

void print_stack_facts(std::ostream& os, const StackFacts& stack, const DisplayOptions& options) {
  os << render_stack_facts(stack, options) << '\n';
}

std::string render_notebook_stack_facts(const StackFacts& stack) {
  SummaryView view;
  view.columns = {stack.type, "value"};
  view.size = static_cast<std::int64_t>(stack.facts.size());
  view.row = [&stack](const std::int64_t i) {
    const auto& fact = stack.facts[static_cast<std::size_t>(i)];
    return std::vector<std::string>{fact.first, fact.second};
  };
  return render_notebook_summary(view, "");
}

std::string span_text(const std::int64_t smallest, const std::int64_t largest, const std::int64_t total, const std::int64_t count) {
  if (count <= 0) {
    return "none";
  }
  if (smallest == largest) {
    return pluss::tils::OutputEngine::format("{} each", smallest);
  }
  return pluss::tils::OutputEngine::format("{} to {} ({} in all)", smallest, largest, total);
}

// How many kinds a tally spells out before it says how many more there are: a stack of many tables can hold many
// types, and a fact must stay one line.
constexpr std::size_t kTallyKinds = 4;

std::string tally_text(const std::vector<std::pair<std::string, std::int64_t>>& counts) {
  if (counts.empty()) {
    return "none";
  }
  auto sorted = counts;
  std::stable_sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
  std::string text;
  for (std::size_t i = 0; i < sorted.size() && i < kTallyKinds; ++i) {
    text += (i > 0 ? ", " : "") + sorted[i].first + " \u00d7 " + std::to_string(sorted[i].second);
  }
  if (sorted.size() > kTallyKinds) {
    text += pluss::tils::OutputEngine::format(", and {} more kinds", sorted.size() - kTallyKinds);
  }
  return text;
}

// An atlas is a stack of frames. Its terminal and notebook face is a short list of facts, the same length however many
// frames it holds or however many columns they have; the frames themselves are for the PlussView window.
static StackFacts atlas_facts_(const Atlas& atlas) {
  StackFacts stack;
  stack.type = "Atlas";
  const auto& frames = atlas.frames();
  std::int64_t cases_min = 0, cases_max = 0, cases_total = 0, features_min = 0, features_max = 0, features_total = 0;
  bool same_names = true;
  std::vector<std::string> first_names;
  std::map<std::string, std::int64_t> kinds;
  for (std::size_t i = 0; i < frames.size(); ++i) {
    const auto& frame = frames[i];
    const auto cases = frame.ncases();
    const auto features = frame.nfeatures();
    cases_min = i == 0 ? cases : std::min(cases_min, cases);
    cases_max = i == 0 ? cases : std::max(cases_max, cases);
    features_min = i == 0 ? features : std::min(features_min, features);
    features_max = i == 0 ? features : std::max(features_max, features);
    cases_total += cases;
    features_total += features;
    const auto names = frame.features();
    if (i == 0) {
      first_names = names;
    } else if (same_names && names != first_names) {
      same_names = false;
    }
    for (const auto& column : dataframe_columns_(frame)) {
      ++kinds[logical_dtype(column)];
    }
  }
  const auto depth = static_cast<std::int64_t>(frames.size());
  stack.facts.emplace_back("depth", pluss::tils::OutputEngine::format("{} frames", depth));
  stack.facts.emplace_back("cases", span_text(cases_min, cases_max, cases_total, depth));
  stack.facts.emplace_back("features", span_text(features_min, features_max, features_total, depth));
  stack.facts.emplace_back("feature names", depth == 0 ? "none" : (same_names ? "the same in every frame" : "differ between frames"));
  stack.facts.emplace_back("ptypes", tally_text(std::vector<std::pair<std::string, std::int64_t>>(kinds.begin(), kinds.end())));
  if (atlas.axis0_labels().has_value()) {
    const auto labels = display_shelf(*atlas.axis0_labels());
    stack.facts.emplace_back("depth labels",
                             pluss::tils::OutputEngine::format("{}: {} to {}", logical_dtype(labels), shelf_value_at(labels, 0),
                                                               shelf_value_at(labels, labels.size() - 1)));
  } else {
    stack.facts.emplace_back("depth labels", "none");
  }
  return stack;
}

std::string first_cell_text(const Shelf& shelf) { return shelf_value_at(shelf, 0); }

void print(std::ostream& os, const Shelf& shelf, const DisplayOptions& options) {
  os << render(shelf, options) << '\n';
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
