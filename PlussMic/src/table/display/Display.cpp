#include "table/display/Display.h"

#include <algorithm>
#include <array>
#include <atomic>
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
    const auto valid = detail::has_validity_mask(t)
                           ? detail::validity_mask(t)->host<bool>()
                           : std::vector<bool>(values.size(), true);
    std::int64_t count = 0;
    for (std::size_t i = 0; i < values.size(); ++i) {
      count += valid[i] && values[i] != pluss::Duration::kNaT ? 1 : 0;
    }
    return count;
  }
  if (!detail::has_validity_mask(t)) {
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
  if (detail::has_validity_mask(t)) {
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

std::string render_shelf_series_like_(const Shelf& shelf,
                                       const EffectiveDisplayOptions& options,
                                       std::int64_t terminal_width) {
  const auto dtype_footer = logical_dtype(shelf);

  const auto values = display_shelf(shelf);
  std::optional<Shelf> labels = std::nullopt;
  if (detail::has_display_labels(shelf)) {
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

  std::vector<std::pair<std::string, std::string>> rows;
  rows.reserve(row_plan.head.size() + row_plan.tail.size() + (row_plan.ellipsis ? 1 : 0));

  auto push_row = [&](std::int64_t ridx) {
    const auto idx = labels.has_value() ? shelf_value_at(*labels, ridx) : pluss::tils::OutputEngine::format("{}", ridx);
    const auto val = shelf_value_at(values, ridx);
    rows.emplace_back(clip_cell(idx, options.max_cell_width), clip_cell(val, options.max_cell_width));
  };

  for (const auto ridx : row_plan.head) {
    push_row(ridx);
  }
  if (row_plan.ellipsis) {
    rows.emplace_back("...", "...");
  }
  for (const auto ridx : row_plan.tail) {
    push_row(ridx);
  }

  std::int64_t idx_width = 1;
  for (const auto& row : rows) {
    idx_width = std::max<std::int64_t>(idx_width, static_cast<std::int64_t>(row.first.size()));
  }

  std::string out;
  for (std::size_t i = 0; i < rows.size(); ++i) {
    const auto line = pluss::tils::OutputEngine::format("{}  {}",
                                  left_pad(rows[i].first, idx_width),
                                  rows[i].second);
    out += fit_line(line, terminal_width);
    if (i + 1 < rows.size()) {
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

struct ThemePalette {
  DisplayTheme theme;
  const char* name;   // id, lower case (used by parse_theme / the demo)
  const char* label;  // shown in the Theme menu
  // order matches kThemeTokens
  std::array<const char*, 29> v;
  const char* heat_pos;  // "r,g,b" for positive correlations
  const char* heat_neg;  // "r,g,b" for negative correlations
};

constexpr std::array<const char*, 29> kThemeTokens{
    "paper", "panel", "ink", "title", "muted", "line", "border", "row-line", "accent", "accent-2",
    "accent-3", "accent-3-border", "on-accent", "btn", "btn-border", "hero-a", "hero-b", "hero-c",
    "head-bg", "case-bg", "case-head-bg", "zebra", "hover", "success", "control-border", "miss-ink",
    "miss-bg", "pill-bg", "grid"};

const std::array<ThemePalette, 8> kThemes{{
    {DisplayTheme::Default, "default", "Default",
     {"#f3f6fb", "#ffffff", "#1b2d4b", "#0b1b3c", "#697789", "#dce4ef", "#cfdae9", "#e7edf5", "#c84d58",
      "#f27b72", "#203e67", "#152b4a", "#ffffff", "#203e67", "#152b4a", "#d8e2f1", "#ffffff", "#f8d9d9",
      "#dae3ee", "#edf1f7", "#f2c8cb", "#f7f9fc", "#e7edf5", "#2b5b80", "#b9c8d9", "#a4433b", "#ffe3db",
      "#e5ebf4", "#dce4ed"},
     "41,93,138", "200,77,88"},
    {DisplayTheme::Ember, "ember", "Ember",
     {"#f1e3da", "#fbf6f2", "#3a241e", "#2a1917", "#8d5646", "#e3cabd", "#e3cdc1", "#f0e0d6", "#b5412a",
      "#e8742a", "#77493b", "#4a2a22", "#fbe4d9", "#6b3a2e", "#46261e", "#eab09a", "#fbf6f2", "#f6c9a8",
      "#f0d3c4", "#f6e4d9", "#e6b9a2", "#fbf1ea", "#f6dccd", "#77493b", "#d9b9a8", "#ffffff", "#d7261e",
      "#f0d3c4", "#f0e0d6"},
     "232,116,42", "90,61,56"},
    {DisplayTheme::Nardo, "nardo", "Nardo",
     {"#dcdfe2", "#f4f5f6", "#1c1f23", "#111316", "#6a7078", "#c3c7cc", "#c3c7cc", "#dfe2e5", "#d1122b",
      "#ff5a36", "#d1122b", "#9d0c20", "#ffffff", "#d1122b", "#9d0c20", "#cfd3d8", "#f4f5f6", "#eceef0",
      "#d3d7db", "#e6e8eb", "#c4c9ce", "#eef0f2", "#e3e6e9", "#d1122b", "#b5bac0", "#a8001a", "#ffc2c9",
      "#d3d7db", "#dfe2e5"},
     "209,18,43", "58,64,72"},
    {DisplayTheme::Brooklands, "brooklands", "Brooklands",
     {"#ece6d2", "#fbf8ed", "#14261b", "#0b3b25", "#6b7a62", "#cfc7a8", "#cfc7a8", "#e6dfc4", "#0b5d3b",
      "#c9a227", "#0b5d3b", "#06402a", "#f0d77a", "#0b5d3b", "#c9a227", "#bfd8c4", "#fbf8ed", "#f2e6b6",
      "#d7e6d4", "#e8f0e4", "#b9d3be", "#f5f2e2", "#e7eed9", "#c9a227", "#b8b08c", "#a30f00", "#ffc2b8",
      "#d7e6d4", "#e6dfc4"},
     "11,93,59", "192,128,16"},
    {DisplayTheme::Gulf, "gulf", "Gulf",
     {"#cfe7f3", "#f7fcff", "#0e2a3a", "#0a2433", "#547489", "#a9d0e4", "#a9d0e4", "#d2e8f3", "#ff6a13",
      "#ff6a13", "#ff6a13", "#d9500a", "#ffffff", "#ff6a13", "#d9500a", "#8ccbea", "#f7fcff", "#ffe3cf",
      "#bfe0f0", "#dcedf7", "#a7d3ea", "#eef7fc", "#d6ecf8", "#ff6a13", "#8fc2dc", "#a8140a", "#ffc2b8",
      "#bfe0f0", "#d2e8f3"},
     "255,106,19", "40,130,180"},
    {DisplayTheme::Obsidian, "obsidian", "Obsidian",
     {"#f8f4ec", "#fffdf9", "#282d32", "#181b22", "#756e68", "#e3d9cb", "#d9cfc2", "#eee7dc", "#a8532d",
      "#eeb171", "#604938", "#493629", "#ffffff", "#604938", "#493629", "#e8d5bd", "#fffdf9", "#f0dfc5",
      "#e8d8c4", "#f3e8d9", "#e2c7a9", "#fbf7f0", "#f3e4d1", "#9d5f35", "#c9bbaa", "#9b382d", "#ffe0d7",
      "#f2e4d2", "#e6dacb"},
     "180,111,60", "80,65,60"},
    {DisplayTheme::Indigo, "indigo", "Indigo",
     {"#f6f3ff", "#fdfcff", "#2b2149", "#19152f", "#756988", "#ded6f1", "#d4c9eb", "#ebe5f8", "#7052b4",
      "#cab4ff", "#4b347f", "#392664", "#ffffff", "#4b347f", "#392664", "#d7c8f6", "#fdfcff", "#e8dcff",
      "#e5dcf8", "#f0eafd", "#d3c0f1", "#faf8ff", "#ede5fb", "#5d4aab", "#cdbfe5", "#9a3260", "#ffe1ec",
      "#e8defa", "#e5dcf7"},
     "112,82,180", "147,88,130"},
    {DisplayTheme::Native, "native", "Native",
     {"#e1e9f8", "#fafcff", "#14213d", "#0f1b4d", "#5a6b8f", "#c5d3ea", "#c5d3ea", "#dbe5f4", "#0fa3a3",
      "#6a5cff", "#2b3a9e", "#1c2a78", "#ffffff", "#2b3a9e", "#1c2a78", "#b4c8fb", "#aee9e3", "#e9f6fb",
      "#cfdcf8", "#eef3fc", "#bfeee9", "#f3f7fd", "#dce9fb", "#0fa3a3", "#a9bbdc", "#ffffff", "#d6336c",
      "#cfdcf8", "#dbe5f4"},
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

// The `:root { --token: value; ... }` block every other rule reads through var().
std::string theme_root_css_(const DisplayOptions& options) {
  const auto& palette = palette_for_(resolve_theme_(options));
  std::string css = ":root {";
  for (std::size_t i = 0; i < kThemeTokens.size(); ++i) {
    css += pluss::tils::OutputEngine::format(" --{}: {};", kThemeTokens[i], palette.v[i]);
  }
  css += " --shadow: none; }";
  return css;
}

std::string html_shell_(std::string_view title,
                        std::string_view subtitle,
                        std::string_view body,
                        const DisplayOptions& options) {
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
      font-family: "Avenir Next", "Inter", "Segoe UI", sans-serif;
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
      background: linear-gradient(90deg, var(--hero-a) 0%, var(--hero-b) 68%, var(--hero-c) 100%);
      display: flex;
      gap: 14px;
      align-items: center;
      justify-content: space-between;
      flex-wrap: wrap;
    }}
    .hero h1 {{
      margin: 0;
      font-family: "Avenir Next", "Inter", "Segoe UI", sans-serif;
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
    .tools input {{
      margin: 0;
      min-width: 220px;
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
    .tools button {{
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
    .table-scroll {{
      overflow: auto;
      max-height: calc(100vh - 300px);
      border-radius: inherit;
    }}
    table {{
      width: 100%;
      height: 400px;
      margin: 0;
      border-collapse: collapse;
      border-radius: 6px;
      overflow: hidden;
      font-size: 0.86rem;
    }}
    thead th {{
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
    .missing {{
      color: var(--miss-ink);
      background: var(--miss-bg);
      font-weight: 700;
    }}
    .nonfinite {{
      /* NaN / inf / -inf are values, not missing data: a neutral tint made from the theme's own ink, never the NULL red. */
      color: var(--title);
      background: color-mix(in srgb, var(--ink) 11%, transparent);
      font-style: italic;
      font-weight: 600;
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
      font-family: "Avenir Next", "Inter", "Segoe UI", sans-serif;
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
      <header class="hero">
        <div>
          <h1>{0}</h1>
          <p>{1}</p>
        </div>
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
          else if (text === 'NAN' || text === 'INF' || text === '-INF') cls += (cls ? ' ' : '') + 'nonfinite';
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
                     theme_root_css_(options));
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

  body += style.virtual_rows ? "<div class=\"table-wrap vwrap\"><div class=\"table-scroll\">"
                              : "<div class=\"table-wrap\"><div class=\"table-scroll\">";
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
        body += numeric ? (missing_cell ? "num missing" : (is_nonfinite ? "num nonfinite" : "num")) : "missing";
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

class DataFrameVirtualTableModel final
    : public pluss::view::VirtualTableModel {
 public:
  explicit DataFrameVirtualTableModel(const DataFrame& frame)
      : cases_(display_shelf(frame.labels().values())),
        columns_(dataframe_columns_(frame)),
        names_(frame.features()),
        rows_(frame.ncases()) {
    numeric_.reserve(columns_.size());
    search_columns_.reserve(columns_.size());
    std::unordered_set<const void*> shared_numeric_payloads;
    for (std::size_t index = 0; index < columns_.size(); ++index) {
      auto& column = columns_[index];
      column = display_shelf(column);
      numeric_.push_back(is_numeric_shelf(column));
      bool searchable = true;
      if (column.kind() == ShelfKind::Series &&
          !detail::has_validity_mask(column)) {
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
    }
    return pluss::view::VirtualTableCell{
        .text = std::move(text),
        .missing = missing,
        .nonfinite = nonfinite,
    };  }

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
  std::vector<std::string> names_{};
  std::vector<bool> numeric_{};
  std::vector<std::size_t> search_columns_{};
  std::int64_t rows_{0};
};

// Body of the data-table panel when the in-house viewer paints the rows from a model: the
// summary cards and an empty table marked as virtual.
std::string virtual_table_body_(const DataFrame& frame) {
  std::string body;
  body += "<div class=\"summary-grid\">";
  body += pluss::tils::OutputEngine::format(
      "<div class=\"summary-card\"><div class=\"label\">Cases</div><div "
      "class=\"value\">{}</div></div>",
      frame.ncases());
  body += pluss::tils::OutputEngine::format(
      "<div class=\"summary-card\"><div class=\"label\">Features</div><div "
      "class=\"value\">{}</div></div>",
      frame.nfeatures());
  body += pluss::tils::OutputEngine::format(
      "<div class=\"summary-card\"><div class=\"label\">Case Axis</div><div "
      "class=\"value\">{}</div></div>",
      escape_html_(logical_dtype(frame.labels().values())));
  body += "</div>";
  body +=
      "<div class=\"table-wrap\"><div class=\"table-scroll\"><table "
      "id=\"plussmic-table\" data-virtual-table=\"true\"></table></div></div>";
  return body;
}

// ---------------------------------------------------------------------------
// Shared helpers for the profiling views (Missing / Outliers). Every view renders as a table so the viewer's table scrolling works.
// ---------------------------------------------------------------------------

// Series of doubles with some entries unavailable (shown blank in the table).
Shelf masked_f64_(const std::vector<double>& values, const std::vector<std::uint8_t>& ok) {
  const auto mask = pluss::make_series(ok).to(pluss::codem::ScalarType::Bool);
  return Shelf(pluss::make_series(values), mask);
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

// Separate absent cells from nonfinite numeric values. The last row counts
// cases where any column has each condition.
std::optional<ViewResult> compute_missing_(const DataFrame& frame) {
  const auto cols = dataframe_columns_(frame);
  const auto names = frame.features();
  const auto n = frame.ncases();
  if (cols.empty() || n == 0) {
    return std::nullopt;
  }
  std::vector<std::string> row_names;
  std::array<std::vector<std::int64_t>, 3> counts;
  std::array<std::vector<double>, 3> percents;
  std::array<std::vector<bool>, 3> any_flags;
  for (auto& flags : any_flags) flags.resize(static_cast<std::size_t>(n), false);
  for (std::size_t c = 0; c < cols.size(); ++c) {
    row_names.push_back(names[c]);
    std::array<std::vector<bool>, 3> flags{missing_flags_(cols[c], n), {}, {}};
    try { flags[1] = shelf_flags_(cols[c].isnan(), n); } catch (const std::exception&) {}
    try { flags[2] = shelf_flags_(cols[c].isinf(), n); } catch (const std::exception&) {}
    for (std::size_t kind = 0; kind < flags.size(); ++kind) {
      std::int64_t count = 0;
      for (std::size_t i = 0; i < static_cast<std::size_t>(n); ++i) {
        if (i < flags[kind].size() && flags[kind][i]) {
          ++count;
          any_flags[kind][i] = true;
        }
      }
      counts[kind].push_back(count);
      percents[kind].push_back(100.0 * static_cast<double>(count) / static_cast<double>(n));
    }
  }
  row_names.push_back("(any column)");
  for (std::size_t kind = 0; kind < counts.size(); ++kind) {
    const auto count = std::count(any_flags[kind].begin(), any_flags[kind].end(), true);
    counts[kind].push_back(count);
    percents[kind].push_back(100.0 * static_cast<double>(count) / static_cast<double>(n));
  }

  std::vector<std::pair<std::string, Shelf>> out;
  const std::array<std::string, 3> labels{"null", "NaN", "infinity"};
  for (std::size_t kind = 0; kind < labels.size(); ++kind) {
    out.emplace_back(labels[kind], Shelf(pluss::make_series(counts[kind])));
    out.emplace_back(labels[kind] + " %", Shelf(pluss::make_series(percents[kind])));
  }
  const DataFrame table(std::move(out), Shelf(pluss::make_note(row_names)));
  return ViewResult{table, 0, false};
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
class StatsViews {
 public:
  explicit StatsViews(DataFrame frame) : frame_(std::move(frame)) {}

  struct Info {
    std::string id;
    std::string label;
  };

  // Views that can apply to this frame. A view listed here may still turn out to be undefined
  // (render() then returns an empty string).
  std::vector<Info> available() const {
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

  std::string render(const std::string& id, const DisplayOptions& options) {
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
std::string render_html_table_(const DataFrame& frame,
                               std::string_view title,
                               std::string_view subtitle,
                               const DisplayOptions& options,
                               StatsViews& views,
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
    struct Panel {
      std::string id;
      std::string label;
      std::string html;
      bool lazy{false};
    };
    std::vector<Panel> panels;
    panels.push_back({"table", "Table", std::move(body), false});
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
  }
  return html_shell_(title, subtitle, body, options);
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
    body += "<div class=\"table-wrap\"><div class=\"table-scroll\"><div style=\"padding:18px;\" class=\"muted\">Atlas is empty.</div></div></div>";
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
      "<div style=\"padding:8px 6px 0;\"><span class=\"pill\">Feature inventory</span></div><div class=\"table-wrap\"><div class=\"table-scroll\">";
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

bool show_html_window_(const std::string& html,
                       const DisplayOptions& options,
                       std::string_view title,
                       const DisplayBackend backend,
                       std::shared_ptr<const pluss::view::VirtualTableModel>
                           virtual_table_model = {},
                       const bool table_viewport_scroll_only = false,
                       std::function<std::string(const DisplayOptions&)> render_with = {},
                       const bool offer_number_format = false,
                       std::function<std::string(const DisplayOptions&, std::string_view)> render_panel_with = {}) {
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
      menu.render = [state, render_with](std::string_view id) {
        for (const auto& entry : kNumberFormats) {
          if (id == entry.id) {
            state->number_format = entry.format;
            return render_with(*state);
          }
        }
        return std::string{};
      };
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

void set_default_theme(DisplayTheme theme) {
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

void set_default_backend(DisplayBackend backend) {
  g_default_backend.store(backend, std::memory_order_relaxed);
}

bool gui_backend_available() {
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
  return render_html_table_(
      frame,
      document_title_("DataFrame", options),
      pluss::tils::OutputEngine::format("{} cases, {} features", frame.ncases(), frame.nfeatures()),
      options, views);
}

std::string render_html(const Shelf& shelf, const DisplayOptions& options) {
  const auto title = document_title_(shelf.named() ? shelf.name() : "Shelf", options);
  const auto subtitle = pluss::tils::OutputEngine::format("{} cases, dtype {}", shelf.size(), shelf.dtype());
  return render_html_pre_(title, subtitle, render(shelf, options), options);
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

std::string render_info_html(const DataFrame& frame, const DisplayOptions& options) {
  return render_html_info_doc_(frame, options);
}

std::string render(const Atlas& atlas, const DisplayOptions& options) {
  const auto term = detect_terminal();
  const auto effective = resolve_effective_options(options, term, false);
  std::vector<std::string> lines;
  lines.push_back(pluss::tils::OutputEngine::format("Atlas(depth={}, rows={}, cols={})", atlas.depth(), atlas.ncases(), atlas.nfeatures()));

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
  std::optional<Shelf> display_axis0;
  if (atlas.axis0_labels().has_value()) {
    display_axis0 = display_shelf(*atlas.axis0_labels());
  }

  for (std::size_t i = 0; i < visible.size(); ++i) {
    if (ellipsis && i == static_cast<std::size_t>(effective.edge_rows)) {
      lines.push_back("...");
    }

    const auto idx = visible[i];
    std::string label = pluss::tils::OutputEngine::format("frame[{}]", idx);
    if (display_axis0.has_value()) {
      label += pluss::tils::OutputEngine::format(" case={}", shelf_value_at(*display_axis0, idx));
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
  const auto backend = resolve_backend_(options);
  if (backend != DisplayBackend::Terminal) {
    auto render_opts = options;
    render_opts.backend = backend;
    const std::int64_t rows = std::max<std::int64_t>(0, frame.ncases());
    const std::int64_t columns = std::max<std::int64_t>(0, frame.nfeatures());
    const bool use_virtual_table =
        backend == DisplayBackend::PlussWebView &&
        rows > 0 && columns > 0 &&
        (rows > 10000 ||
         columns > 256 ||
         (rows <= std::numeric_limits<std::int64_t>::max() /
                      std::max<std::int64_t>(1, columns) &&
          rows * columns > 250000));
    std::shared_ptr<const pluss::view::VirtualTableModel> virtual_model;
    const DataFrame frame_copy = frame;
    // PlussView computes each profiling view when it is first opened and keeps the result, so
    // opening the window is not delayed by views nobody uses and a theme or number-format change
    // only re-renders. (A browser view builds every view up front.)
    const bool in_house = backend == DisplayBackend::PlussWebView;
    const auto views = std::make_shared<StatsViews>(frame_copy);
    const auto render_with = [frame_copy, views, use_virtual_table, in_house](const DisplayOptions& themed) {
      return render_html_table_(
          frame_copy,
          document_title_("DataFrame", themed),
          pluss::tils::OutputEngine::format("{} cases, {} features", frame_copy.ncases(), frame_copy.nfeatures()),
          themed, *views, /*lazy=*/in_house, /*virtual_model=*/use_virtual_table);
    };
    const auto html = render_with(render_opts);
    if (use_virtual_table) {
      virtual_model =
          std::make_shared<DataFrameVirtualTableModel>(frame);
    }
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
                          !use_virtual_table,
                          std::move(render_panel_with))) {
      return;
    }
  }
  print(std::cout, frame, options);
}

void show(const Shelf& shelf, const DisplayOptions& options) {
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

void show(const Atlas& atlas, const DisplayOptions& options) {
  const auto backend = resolve_backend_(options);
  if (backend != DisplayBackend::Terminal) {
    auto render_opts = options;
    render_opts.backend = backend;
    const auto html = render_html(atlas, render_opts);
    if (show_html_window_(html, render_opts, "Pluss Atlas", backend)) {
      return;
    }
  }
  print(std::cout, atlas, options);
}

void show_info(const DataFrame& frame, const DisplayOptions& options) {
  const auto backend = resolve_backend_(options);
  if (backend != DisplayBackend::Terminal) {
    auto render_opts = options;
    render_opts.backend = backend;
    const auto html = render_info_html(frame, render_opts);
    if (show_html_window_(html, render_opts, "Pluss DataFrame", backend)) {
      return;
    }
  }
  print_info(std::cout, frame, options);
}

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
