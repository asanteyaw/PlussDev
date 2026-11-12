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
#include <unordered_set>
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
#include "pluss/tils/view/html_window.h"
#include "pluss/tils/view/window/text_layout.h"
#include "table/Atlas.h"
#include "table/DataFrame.h"
#include "table/Slate.h"

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
    return std::isnan(value) ? std::string("NULL")
                             : fmt::format(fmt::runtime(std::string(spec)), value);
  };
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

bool is_numeric_slate(const Slate& t) {
  if (t.kind() != SlateKind::Series) {
    return false;
  }
  const auto dt = t.as_series().dtype();
  return dt.is_floating() || dt.is_integral() || dt.scalar == pluss::codem::ScalarType::Bool;
}

bool series_all_zero_i64(const pluss::Series& s) {
  const auto values = s.host<std::int64_t>();
  return std::all_of(values.begin(), values.end(), [](const std::int64_t v) { return v == 0; });
}

Slate temporal_slate_to_display_note(const Slate& t) {
  const auto timeline = t.as_timeline();
  const auto has_subsecond = !series_all_zero_i64(timeline.nanosecond());
  const auto has_clock_time = has_subsecond || !series_all_zero_i64(timeline.second())
                              || !series_all_zero_i64(timeline.minute())
                              || !series_all_zero_i64(timeline.hour());
  if (!has_clock_time) {
    return Slate(timeline.strftime("%Y-%m-%d"), detail::validity_mask(t));
  }
  if (!has_subsecond) {
    return Slate(timeline.strftime("%Y-%m-%d %H:%M:%S"), detail::validity_mask(t));
  }
  return Slate(timeline.strftime("%Y-%m-%d %H:%M:%S.%f"), detail::validity_mask(t));
}

Slate display_slate(const Slate& t) {
  if (t.kind() == SlateKind::Timeline) {
    return temporal_slate_to_display_note(t);
  }
  if (t.kind() == SlateKind::Duration) {
    return t.cast().note();
  }
  return t;
}

std::string slate_value_at(const Slate& t, std::int64_t row) {
  if (!detail::valid_at(t, row)) {
    return "NULL";
  }

  switch (t.kind()) {
    case SlateKind::Series:
      return scalar_at(t.as_series(), row);
    case SlateKind::Note:
      return std::string(t.as_note().at(row));
    case SlateKind::Timeline:
      return fmt::format("{}", t.as_timeline().ticks()[static_cast<std::size_t>(row)]);
    case SlateKind::Duration:
      return std::string(t.as_duration().to_note_iso().at(row));
  }
  return "<?>";
}

std::int64_t non_null_count(const Slate& t) {
  if (t.kind() == SlateKind::Duration) {
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

std::uint64_t estimate_bytes(const Slate& t) {
  std::uint64_t bytes = 0;
  if (t.kind() == SlateKind::Series) {
    const auto item = t.as_series().dtype().itemsize;
    bytes += static_cast<std::uint64_t>(item) * static_cast<std::uint64_t>(t.size());
  } else if (t.kind() == SlateKind::Timeline || t.kind() == SlateKind::Duration) {
    bytes += static_cast<std::uint64_t>(8) * static_cast<std::uint64_t>(t.size());
  } else if (t.kind() == SlateKind::Note) {
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

std::vector<Slate> dataframe_columns_(const DataFrame& frame) {
  std::vector<Slate> columns;
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
  const auto display_cases = display_slate(frame.labels().values());
  std::vector<Slate> display_cols;
  display_cols.reserve(cols.size());
  for (const auto& col : cols) {
    display_cols.push_back(display_slate(col));
  }

  auto has_negative_visible_value = [&](std::int64_t cidx) {
    const auto& col = cols[static_cast<std::size_t>(cidx)];
    if (!is_numeric_slate(col)) {
      return false;
    }

    auto row_is_negative = [&](std::int64_t ridx) {
      const auto v = slate_value_at(display_cols[static_cast<std::size_t>(cidx)], ridx);
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
      row.push_back(clip_cell(slate_value_at(display_cases, ridx), options.max_cell_width));
    }

    auto render_data_cell = [&](std::int64_t cidx) {
      std::string value = slate_value_at(display_cols[static_cast<std::size_t>(cidx)], ridx);
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

std::string logical_dtype(const Slate& t) {
  switch (t.kind()) {
    case SlateKind::Series:
      return "Series";
    case SlateKind::Note:
      return "Note";
    case SlateKind::Timeline:
      return "Timeline";
    case SlateKind::Duration:
      return "Duration";
  }
  return "Unknown";
}

std::string render_slate_series_like_(const Slate& slate,
                                       const EffectiveDisplayOptions& options,
                                       std::int64_t terminal_width) {
  const auto dtype_footer = logical_dtype(slate);

  const auto values = display_slate(slate);
  std::optional<Slate> labels = std::nullopt;
  if (detail::has_display_labels(slate)) {
    labels = display_slate(detail::display_labels(slate));
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
      auto parts = split_display_token_(slate_value_at(*labels, ridx));
      parts.resize(structured_headers.size());
      parts.push_back(slate_value_at(values, ridx));
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
    if (slate.named()) {
      out += fmt::format("Name: {}, ptype: {}", slate.name(), dtype_footer);
    } else {
      out += fmt::format("ptype: {}", dtype_footer);
    }
    return out;
  }

  std::vector<std::pair<std::string, std::string>> rows;
  rows.reserve(row_plan.head.size() + row_plan.tail.size() + (row_plan.ellipsis ? 1 : 0));

  auto push_row = [&](std::int64_t ridx) {
    const auto idx = labels.has_value() ? slate_value_at(*labels, ridx) : fmt::format("{}", ridx);
    const auto val = slate_value_at(values, ridx);
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
    const auto line = fmt::format("{}  {}",
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
  if (slate.named()) {
    out += fmt::format("Name: {}, ptype: {}", slate.name(), dtype_footer);
  } else {
    out += fmt::format("ptype: {}", dtype_footer);
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

  return fmt::format(R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>{0}</title>
  <style>
    :root {{
      --paper: #f4f7fb;
      --ink: #102033;
      --muted: #607086;
      --grid: #dbe3ec;
      --line: #c8d3df;
      --accent: #007f86;
      --accent-2: #ef5b3f;
      --accent-3: #6757d9;
      --success: #4f8994;
      --panel: #ffffff;
      --shadow: none;
    }}
    * {{ box-sizing: border-box; }}
    body {{
      margin: 0;
      color: var(--ink);
      font-family: "Avenir Next", "Inter", "Segoe UI", sans-serif;
      background: #edf4f7;
    }}
    .shell {{
      min-height: 100vh;
      padding: 14px;
    }}
    .frame {{
      max-width: 1560px;
      margin: 0 auto;
      background: var(--panel);
      border: 1px solid #cbd7e3;
      border-radius: 8px;
      overflow: hidden;
      box-shadow: none;
    }}
    .hero {{
      padding: 14px 16px;
      border-bottom: 1px solid #cbd7e3;
      border-top: 5px solid var(--accent);
      background: linear-gradient(90deg, #dff5f4 0%, #ffffff 68%, #f5f9fa 100%);
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
      color: #102f3b;
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
      border: 1px solid #b9c7d5;
      background: #ffffff;
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
      border: 1px solid #075d68;
      background: #0b7285;
      color: white;
      font: 600 0.82rem/1 "Avenir Next", "Inter", "Segoe UI", sans-serif;
      line-height: 38px;
      appearance: none;
      -webkit-appearance: none;
      cursor: pointer;
    }}
    .tools button.reset {{
      background: #0b7285;
      border-color: #075d68;
      color: #ffffff;
    }}
    .tools button {{
      text-align: center;
    }}
    .tools input {{
      text-align: left;
    }}
    .content {{
      padding: 0 12px 12px;
      background: #ffffff;
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
      border: 1px solid #cfdae5;
      border-left: 4px solid var(--accent-3);
      background: #ffffff;
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
      border: 1px solid #c8d4df;
      border-radius: 7px;
      background: #ffffff;
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
      background: #dcefee;
      border-bottom: 1px solid var(--line);
      text-align: center;
      white-space: nowrap;
    }}
    th, td {{
      padding: 8px 10px;
      border-bottom: 1px solid #dde5ed;
      vertical-align: top;
    }}
    tbody tr:nth-child(even) {{
      background: #f7f9fc;
    }}
    tbody tr:hover {{
      background: #e8f7f5;
    }}
    th.sortable {{
      cursor: pointer;
      user-select: none;
    }}
    th.sortable::after {{
      content: "  ↕";
      color: #65758a;
      font-size: 0.85em;
    }}
    .sticky-left {{
      position: sticky;
      left: 0;
      z-index: 1;
      background: #edf6f7;
      border-right: 2px solid var(--success);
    }}
    thead .sticky-left {{
      z-index: 3;
      background: #cce8e6;
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
      color: #c9362b;
      background: #fff0ee;
      font-weight: 700;
    }}
    .hidden {{
      display: none;
    }}
    .pill {{
      display: inline-block;
      padding: 4px 7px;
      border-radius: 6px;
      background: #dff3f1;
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
      border-bottom: 1px solid #d8e1e8;
      padding-bottom: 12px;
    }}
    .atlas-current-frame {{
      min-width: 150px;
      padding: 8px 12px;
      border: 1px solid #9eb2c3;
      border-radius: 7px;
      background: #edf6f7;
      color: #163b47;
      text-align: center;
      font-weight: 700;
      white-space: nowrap;
    }}
    .atlas-tab {{
      padding: 7px 10px;
      border: 1px solid #bac8d6;
      border-radius: 7px;
      background: #ffffff;
      color: var(--ink);
      cursor: pointer;
      white-space: nowrap;
      font: inherit;
    }}
    .atlas-tab.active {{
      background: var(--accent-3);
      border-color: #5141bd;
      color: #ffffff;
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

    resetButton.addEventListener('click', () => {{
      searchInput.value = '';
      filterTables('');
      detailTables.forEach((tbl) => {{
        const tbody = tbl.tBodies[0];
        if (!tbody) return;
        const rows = Array.from(tbody.rows);
        rows.sort((a, b) => Number(a.dataset.rowIndex || '0') - Number(b.dataset.rowIndex || '0'));
        rows.forEach((row) => tbody.appendChild(row));
        delete tbl.dataset.sortDir;
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
                     responsive_css);
}

std::string render_html_pre_(std::string_view title,
                             std::string_view subtitle,
                             const std::string& content,
                             const DisplayOptions& options) {
  const auto body = fmt::format(
      R"HTML(<div class="table-wrap"><div class="table-scroll"><pre style="margin:0; padding:16px; font-family:'IBM Plex Mono', 'SFMono-Regular', monospace; white-space:pre-wrap;">{}</pre></div></div>)HTML",
      escape_html_(content));
  return html_shell_(title, subtitle, body, options);
}

std::string render_html_table_body_(const DataFrame& frame,
                                    const DisplayOptions& options,
                                    std::string_view table_id) {
  const auto cols = dataframe_columns_(frame);
  const auto display_cases = display_slate(frame.labels().values());
  std::vector<Slate> display_cols;
  display_cols.reserve(cols.size());
  for (const auto& col : cols) {
    display_cols.push_back(display_slate(col));
  }
  const auto names = frame.features();

  std::string body;
  body += "<div class=\"summary-grid\">";
  body += fmt::format("<div class=\"summary-card\"><div class=\"label\">Cases</div><div class=\"value\">{}</div></div>",
                      frame.ncases());
  body +=
      fmt::format("<div class=\"summary-card\"><div class=\"label\">Features</div><div class=\"value\">{}</div></div>",
                  frame.nfeatures());
  body += fmt::format(
      "<div class=\"summary-card\"><div class=\"label\">Case Axis</div><div class=\"value\">{}</div></div>",
      escape_html_(logical_dtype(frame.labels().values())));
  body += "</div>";

  body += "<div class=\"table-wrap\"><div class=\"table-scroll\">";
  int case_width_px = 0;
  if (options.show_case_index) {
    case_width_px = pluss::view::window::layout_math_text("case", 14).width;
    for (std::int64_t row = 0; row < frame.ncases(); ++row) {
      case_width_px = std::max(
          case_width_px,
          pluss::view::window::layout_math_text(slate_value_at(display_cases, row), 14).width);
    }
    case_width_px += 5;
  }

  body += fmt::format("<table id=\"{}\" data-filterable=\"true\">", escape_html_(table_id));
  if (options.show_case_index) {
    body += "<colgroup>";
    body += fmt::format(
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
    const auto case_numeric = is_numeric_slate(frame.labels().values());
    body += fmt::format(
        "<th class=\"sortable sticky-left{}\" data-column-index=\"{}\" data-kind=\"{}\">case</th>",
        case_numeric ? " num" : "",
        column_index++,
        case_numeric ? "numeric" : "text");
  }

  for (std::size_t i = 0; i < names.size(); ++i) {
    const auto numeric = is_numeric_slate(cols[i]);
    body += fmt::format("<th class=\"sortable{}\" data-column-index=\"{}\" data-kind=\"{}\">{}</th>",
                        numeric ? " num" : "",
                        column_index++,
                        numeric ? "numeric" : "text",
                        escape_html_(names[i]));
  }
  body += "</tr></thead><tbody>";

  for (std::int64_t row = 0; row < frame.ncases(); ++row) {
    body += fmt::format("<tr data-row-index=\"{}\">", row);
    if (options.show_case_index) {
      const auto case_value = slate_value_at(display_cases, row);
      const auto case_numeric = is_numeric_slate(frame.labels().values());
      body += fmt::format("<td class=\"sticky-left{}{}\" data-sort-value=\"{}\">{}</td>",
                          case_numeric ? " num" : "",
                          case_value == "NULL" ? " missing" : "",
                          escape_html_(case_value),
                          escape_html_(case_value));
    }
    for (std::size_t col = 0; col < cols.size(); ++col) {
      const auto value = slate_value_at(display_cols[col], row);
      const auto numeric = is_numeric_slate(cols[col]);
      body += fmt::format("<td class=\"{}{}\" data-sort-value=\"{}\">{}</td>",
                          numeric ? "num" : "",
                          value == "NULL" ? " missing" : "",
                          escape_html_(value),
                          escape_html_(value));
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
      : cases_(display_slate(frame.labels().values())),
        columns_(dataframe_columns_(frame)),
        names_(frame.features()),
        rows_(frame.ncases()) {
    numeric_.reserve(columns_.size());
    search_columns_.reserve(columns_.size());
    std::unordered_set<const void*> shared_numeric_payloads;
    for (std::size_t index = 0; index < columns_.size(); ++index) {
      auto& column = columns_[index];
      column = display_slate(column);
      numeric_.push_back(is_numeric_slate(column));
      bool searchable = true;
      if (column.kind() == SlateKind::Series &&
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
    return slate_value_at(cases_, row);
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
    return pluss::view::VirtualTableCell{
        .text = missing ? std::string("NULL") : slate_value_at(source, row),
        .missing = missing,
    };
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
    if (contains_query(slate_value_at(cases_, row))) {
      return true;
    }
    for (const std::size_t index : search_columns_) {
      if (contains_query(slate_value_at(columns_[index], row))) {
        return true;
      }
    }
    return false;
  }

 private:
  Slate cases_{};
  std::vector<Slate> columns_{};
  std::vector<std::string> names_{};
  std::vector<bool> numeric_{};
  std::vector<std::size_t> search_columns_{};
  std::int64_t rows_{0};
};

std::string render_virtual_html_table_(const DataFrame& frame,
                                       std::string_view title,
                                       std::string_view subtitle,
                                       const DisplayOptions& options) {
  std::string body;
  body += "<div class=\"summary-grid\">";
  body += fmt::format(
      "<div class=\"summary-card\"><div class=\"label\">Cases</div><div "
      "class=\"value\">{}</div></div>",
      frame.ncases());
  body += fmt::format(
      "<div class=\"summary-card\"><div class=\"label\">Features</div><div "
      "class=\"value\">{}</div></div>",
      frame.nfeatures());
  body += fmt::format(
      "<div class=\"summary-card\"><div class=\"label\">Case Axis</div><div "
      "class=\"value\">{}</div></div>",
      escape_html_(logical_dtype(frame.labels().values())));
  body += "</div>";
  body +=
      "<div class=\"table-wrap\"><div class=\"table-scroll\"><table "
      "id=\"plussmic-table\" data-virtual-table=\"true\"></table></div></div>";
  return html_shell_(title, subtitle, body, options);
}

std::string render_html_table_(const DataFrame& frame,
                               std::string_view title,
                               std::string_view subtitle,
                               const DisplayOptions& options) {
  const auto body = render_html_table_body_(frame, options, "plussmic-table");
  return html_shell_(title, subtitle, body, options);
}

std::string render_html_atlas_(const Atlas& atlas, const DisplayOptions& options) {
  std::optional<Slate> display_axis0;
  if (atlas.axis0_labels().has_value()) {
    display_axis0 = display_slate(*atlas.axis0_labels());
  }

  std::string body;
  body += "<div class=\"summary-grid\">";
  body += fmt::format("<div class=\"summary-card\"><div class=\"label\">Depth</div><div class=\"value\">{}</div></div>",
                      atlas.depth());
  body += fmt::format("<div class=\"summary-card\"><div class=\"label\">Cases</div><div class=\"value\">{}</div></div>",
                      atlas.ncases());
  body += fmt::format("<div class=\"summary-card\"><div class=\"label\">Features</div><div class=\"value\">{}</div></div>",
                      atlas.nfeatures());
  body += fmt::format("<div class=\"summary-card\"><div class=\"label\">Depth Labels</div><div class=\"value\">{}</div></div>",
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
    first_label = slate_value_at(*display_axis0, 0);
  }
  body += fmt::format(
      "<span id=\"atlas-current-frame\" class=\"atlas-current-frame\" role=\"status\">{} (1 of {})</span>",
      escape_html_(first_label), atlas.depth());
  body += "<button id=\"atlas-next\" type=\"button\" class=\"atlas-tab\">Next</button>";
  body += "<button id=\"atlas-last\" type=\"button\" class=\"atlas-tab\">Last</button>";
  body += "<button id=\"atlas-show-all\" type=\"button\" class=\"atlas-tab\" aria-pressed=\"false\">All frames</button>";
  body += "</div>";
  body += "<div class=\"atlas-nav atlas-frame-tabs\" role=\"group\" aria-label=\"Select a frame\">";

  for (std::int64_t i = 0; i < atlas.depth(); ++i) {
    std::string label = fmt::format("Frame {}", i);
    if (display_axis0.has_value()) {
      label = slate_value_at(*display_axis0, i);
    }
    body += fmt::format(
        "<button id=\"atlas-tab-{}\" type=\"button\" class=\"atlas-tab{}\" data-frame-index=\"{}\" aria-pressed=\"{}\">{}</button>",
        i, i == 0 ? " active" : "", i, i == 0 ? "true" : "false", escape_html_(label));
  }
  body += "</div>";

  for (std::int64_t i = 0; i < atlas.depth(); ++i) {
    std::string label = fmt::format("Frame {}", i);
    if (display_axis0.has_value()) {
      label = slate_value_at(*display_axis0, i);
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
                     fmt::format("{} frames, {} cases, {} features", atlas.depth(), atlas.ncases(), atlas.nfeatures()),
                     body,
                     options);
}

std::string render_html_info_doc_(const DataFrame& frame, const DisplayOptions& options) {
  const auto cols = dataframe_columns_(frame);
  const auto display_cases = display_slate(frame.labels().values());
  const auto names = frame.features();

  std::unordered_map<std::string, std::int64_t> dtype_counts;
  std::uint64_t memory_bytes = estimate_bytes(frame.labels().values());
  for (std::size_t i = 0; i < names.size(); ++i) {
    const auto dtype = logical_dtype(cols[i]);
    dtype_counts[dtype] += 1;
    memory_bytes += estimate_bytes(cols[i]);
  }

  const auto first_case = frame.ncases() > 0 ? slate_value_at(display_cases, 0) : std::string("NULL");
  const auto last_case = frame.ncases() > 0 ? slate_value_at(display_cases, frame.ncases() - 1) : std::string("NULL");

  std::string body;
  body += "<div class=\"summary-grid\">";
  body += fmt::format("<div class=\"summary-card\"><div class=\"label\">Class</div><div class=\"value\">{}</div></div>",
                      "pluss::table::DataFrame");
  body += fmt::format("<div class=\"summary-card\"><div class=\"label\">Cases</div><div class=\"value\">{}</div></div>",
                      frame.ncases());
  body +=
      fmt::format("<div class=\"summary-card\"><div class=\"label\">Features</div><div class=\"value\">{}</div></div>",
                  frame.nfeatures());
  body += fmt::format(
      "<div class=\"summary-card\"><div class=\"label\">Case Range</div><div class=\"value\">{} <span class=\"muted\">to</span> {}</div></div>",
      escape_html_(first_case),
      escape_html_(last_case));
  const auto null_case_rows = frame.nfeatures() == 0
                                  ? std::int64_t{0}
                                  : frame.isnull().any(Axis::Case).sum().as_series().host<std::int64_t>().front();
  body += fmt::format(
      "<div class=\"summary-card\"><div class=\"label\">Null Case Rows</div><div class=\"value\">{}</div></div>",
      null_case_rows);
  body += fmt::format("<div class=\"summary-card\"><div class=\"label\">PTypes</div><div class=\"value\">{}</div></div>",
                      escape_html_(dtype_summary_line(dtype_counts)));
  body += fmt::format("<div class=\"summary-card\"><div class=\"label\">Memory</div><div class=\"value\">{}</div></div>",
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
    body += fmt::format(
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
                     fmt::format("{} cases, {} features", frame.ncases(), frame.nfeatures()),
                     body,
                     options);
}

bool show_html_window_(const std::string& html,
                       const DisplayOptions& options,
                       std::string_view title,
                       const DisplayBackend backend,
                       std::shared_ptr<const pluss::view::VirtualTableModel>
                           virtual_table_model = {},
                       const bool table_viewport_scroll_only = false) {
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
    out += fmt::format("\n[{} cs x {} feat]", frame.ncases(), frame.nfeatures());
  }
  return out;
}

std::string render(const Slate& slate, const DisplayOptions& options) {
  const auto term = detect_terminal();
  const auto effective = resolve_effective_options(options, term, false);
  const auto terminal_width = effective.terminal_width > 0 ? effective.terminal_width : term.columns;
  return render_slate_series_like_(slate, effective, terminal_width);
}

std::string render_html(const DataFrame& frame, const DisplayOptions& options) {
  return render_html_table_(
      frame,
      document_title_("DataFrame", options),
      fmt::format("{} cases, {} features", frame.ncases(), frame.nfeatures()),
      options);
}

std::string render_html(const Slate& slate, const DisplayOptions& options) {
  const auto title = document_title_(slate.named() ? slate.name() : "Slate", options);
  const auto subtitle = fmt::format("{} cases, dtype {}", slate.size(), slate.dtype());
  return render_html_pre_(title, subtitle, render(slate, options), options);
}

std::string render_html(const Atlas& atlas, const DisplayOptions& options) {
  return render_html_atlas_(atlas, options);
}

std::string render_info(const DataFrame& frame, const DisplayOptions& options) {
  const auto term = detect_terminal();
  const auto effective = resolve_effective_options(options, term, true);
  const auto terminal_width = effective.terminal_width > 0 ? effective.terminal_width : term.columns;

  const auto cols = dataframe_columns_(frame);
  const auto display_cases = display_slate(frame.labels().values());
  const auto names = frame.features();

  std::unordered_map<std::string, std::int64_t> dtype_counts;
  std::uint64_t memory_bytes = estimate_bytes(frame.labels().values());

  for (std::size_t i = 0; i < names.size(); ++i) {
    const auto dtype = logical_dtype(cols[i]);
    dtype_counts[dtype] += 1;
    memory_bytes += estimate_bytes(cols[i]);
  }

  const auto first_case = frame.ncases() > 0 ? slate_value_at(display_cases, 0) : std::string("NULL");
  const auto last_case = frame.ncases() > 0 ? slate_value_at(display_cases, frame.ncases() - 1) : std::string("NULL");

  TableMatrix summary;
  summary.headers = {"Field", "Value"};
  summary.right_align = {false, false};
  summary.min_widths = {5, 8};
  const auto null_case_rows = frame.nfeatures() == 0
                                  ? std::int64_t{0}
                                  : frame.isnull().any(Axis::Case).sum().as_series().host<std::int64_t>().front();
  summary.rows = {
      {"Class", "pluss::table::DataFrame"},
      {"Cases", fmt::format("{}", frame.ncases())},
      {"Features", fmt::format("{}", frame.nfeatures())},
      {"Case Range", fmt::format("{} -> {}", first_case, last_case)},
      {"Null Case Rows", fmt::format("{}", null_case_rows)},
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
        fmt::format("{}", i),
        names[i],
        fmt::format("{}", nn),
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
  lines.push_back(fmt::format("Atlas(depth={}, rows={}, cols={})", atlas.depth(), atlas.ncases(), atlas.nfeatures()));

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
  std::optional<Slate> display_axis0;
  if (atlas.axis0_labels().has_value()) {
    display_axis0 = display_slate(*atlas.axis0_labels());
  }

  for (std::size_t i = 0; i < visible.size(); ++i) {
    if (ellipsis && i == static_cast<std::size_t>(effective.edge_rows)) {
      lines.push_back("...");
    }

    const auto idx = visible[i];
    std::string label = fmt::format("frame[{}]", idx);
    if (display_axis0.has_value()) {
      label += fmt::format(" case={}", slate_value_at(*display_axis0, idx));
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
    const auto html =
        use_virtual_table
            ? render_virtual_html_table_(
                  frame,
                  document_title_("DataFrame", render_opts),
                  fmt::format("{} cases, {} features", frame.ncases(),
                              frame.nfeatures()),
                  render_opts)
            : render_html(frame, render_opts);
    if (use_virtual_table) {
      virtual_model =
          std::make_shared<DataFrameVirtualTableModel>(frame);
    }
    if (show_html_window_(html,
                          render_opts,
                          "Pluss DataFrame",
                          backend,
                          std::move(virtual_model),
                          backend == DisplayBackend::PlussWebView)) {
      return;
    }
  }
  print(std::cout, frame, options);
}

void show(const Slate& slate, const DisplayOptions& options) {
  const auto backend = resolve_backend_(options);
  if (backend != DisplayBackend::Terminal) {
    auto render_opts = options;
    render_opts.backend = backend;
    const auto html = render_html(slate, render_opts);
    if (show_html_window_(html, render_opts, "PlussMic Slate", backend)) {
      return;
    }
  }
  print(std::cout, slate, options);
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

void print(std::ostream& os, const Slate& slate, const DisplayOptions& options) {
  os << render(slate, options) << '\n';
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
