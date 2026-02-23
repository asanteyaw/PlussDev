#include "CodeM/core/Printing.h"

#include "CodeM/core/Complex.h"
#include "CodeM/core/Half.h"
#include "CodeM/core/ScalarType.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace pluss::codem {

namespace {

constexpr const char* kAnsiReset = "\x1b[0m";
constexpr const char* kAnsiRed = "\x1b[31m";
constexpr const char* kAnsiYellow = "\x1b[33m";

struct ScalarText final {
  std::string text;
  bool numeric{false};
  bool negative{false};
  bool special{false};
  bool ellipsis{false};
};

struct DecimalParts final {
  std::size_t pre_digits{0};
  std::size_t post_digits{0};
  bool has_sign{false};
};

struct VisiblePlan final {
  std::vector<std::int64_t> head;
  std::vector<std::int64_t> tail;
  bool has_ellipsis{false};
};

struct LegacySummaryConfig final {
  bool summarize{false};
  std::int64_t head{0};
  std::int64_t tail_start{0};
};

static bool is_special_fp_(double v) {
  return std::isnan(v) || std::isinf(v);
}

static std::string trim_float_string_(std::string s) {
  const std::size_t epos = s.find_first_of("eE");
  const std::size_t body_end = (epos == std::string::npos) ? s.size() : epos;
  const std::size_t dot = s.find('.');
  if (dot == std::string::npos || dot >= body_end) {
    return s;
  }

  std::size_t last = body_end;
  while (last > dot + 1 && s[last - 1] == '0') {
    --last;
  }
  if (last == dot + 1) {
    --last;
  }

  std::string out;
  out.reserve(s.size());
  out.append(s.data(), last);
  if (epos != std::string::npos) {
    out.append(s.data() + epos, s.size() - epos);
  }
  return out;
}

static std::string format_float_(double v, const PrintOptions& opt) {
  std::ostringstream oss;
  if (opt.number_mode == PrintNumberMode::Fixed) {
    oss << std::fixed;
  } else if (opt.number_mode == PrintNumberMode::Scientific) {
    oss << std::scientific;
  }

  const std::int64_t p = std::max<std::int64_t>(0, opt.precision);
  oss << std::setprecision(static_cast<int>(p));
  oss << v;
  std::string out = oss.str();
  if (opt.trim_trailing_zeros && !is_special_fp_(v)) {
    out = trim_float_string_(std::move(out));
  }
  return out;
}

static std::string format_complex_(double re, double im, const PrintOptions& opt) {
  const bool imag_neg = std::signbit(im);
  const std::string re_s = format_float_(re, opt);
  const std::string im_s = format_float_(std::fabs(im), opt);

  const char unit =
      (opt.complex_style == PrintComplexStyle::ParenI || opt.complex_style == PrintComplexStyle::CartesianI) ? 'i'
                                                                                                               : 'j';

  if (opt.complex_style == PrintComplexStyle::CartesianI ||
      opt.complex_style == PrintComplexStyle::CartesianJ) {
    return re_s + (imag_neg ? " - " : " + ") + im_s + unit;
  }

  return "(" + re_s + (imag_neg ? "-" : "+") + im_s + unit + ")";
}

static ScalarText scalar_text_at_(const Series& s, std::int64_t off, const PrintOptions& opt) {
  ScalarText out;

  auto set_float = [&](double v) {
    out.numeric = true;
    out.negative = std::signbit(v) && !is_special_fp_(v);
    out.special = is_special_fp_(v);
    out.text = format_float_(v, opt);
  };

  switch (s.scalar_type()) {
    case ScalarType::Float64: set_float(reinterpret_cast<const double*>(s.raw_data())[off]); return out;
    case ScalarType::Float32: set_float(reinterpret_cast<const float*>(s.raw_data())[off]); return out;
    case ScalarType::Float16:
      set_float(float16_to_float(reinterpret_cast<const Float16*>(s.raw_data())[off]));
      return out;
    case ScalarType::BFloat16:
      set_float(bfloat16_to_float(reinterpret_cast<const BFloat16*>(s.raw_data())[off]));
      return out;
    case ScalarType::Int64: {
      const auto v = reinterpret_cast<const std::int64_t*>(s.raw_data())[off];
      out.numeric = true;
      out.negative = v < 0;
      out.text = std::to_string(v);
      return out;
    }
    case ScalarType::Int32: {
      const auto v = reinterpret_cast<const std::int32_t*>(s.raw_data())[off];
      out.numeric = true;
      out.negative = v < 0;
      out.text = std::to_string(v);
      return out;
    }
    case ScalarType::Int16: {
      const auto v = reinterpret_cast<const std::int16_t*>(s.raw_data())[off];
      out.numeric = true;
      out.negative = v < 0;
      out.text = std::to_string(v);
      return out;
    }
    case ScalarType::Int8: {
      const auto v = reinterpret_cast<const std::int8_t*>(s.raw_data())[off];
      out.numeric = true;
      out.negative = v < 0;
      out.text = std::to_string(static_cast<int>(v));
      return out;
    }
    case ScalarType::UInt64:
      out.numeric = true;
      out.text = std::to_string(reinterpret_cast<const std::uint64_t*>(s.raw_data())[off]);
      return out;
    case ScalarType::UInt32:
      out.numeric = true;
      out.text = std::to_string(reinterpret_cast<const std::uint32_t*>(s.raw_data())[off]);
      return out;
    case ScalarType::UInt16:
      out.numeric = true;
      out.text = std::to_string(reinterpret_cast<const std::uint16_t*>(s.raw_data())[off]);
      return out;
    case ScalarType::UInt8:
      out.numeric = true;
      out.text = std::to_string(reinterpret_cast<const std::uint8_t*>(s.raw_data())[off]);
      return out;
    case ScalarType::Bool: {
      const bool v = reinterpret_cast<const bool*>(s.raw_data())[off];
      out.numeric = false;
      out.text = (opt.bool_style == PrintBoolStyle::OneZero) ? (v ? "1" : "0") : (v ? "true" : "false");
      return out;
    }
    case ScalarType::Complex64: {
      const auto v = reinterpret_cast<const Complex64*>(s.raw_data())[off];
      out.text = format_complex_(v.real(), v.imag(), opt);
      out.numeric = false;
      out.negative = (v.real() < 0.0f) || (v.imag() < 0.0f);
      out.special = is_special_fp_(v.real()) || is_special_fp_(v.imag());
      return out;
    }
    case ScalarType::Complex128: {
      const auto v = reinterpret_cast<const Complex128*>(s.raw_data())[off];
      out.text = format_complex_(v.real(), v.imag(), opt);
      out.numeric = false;
      out.negative = (v.real() < 0.0) || (v.imag() < 0.0);
      out.special = is_special_fp_(v.real()) || is_special_fp_(v.imag());
      return out;
    }
    default:
      out.text = "<dtype?>";
      return out;
  }
}

static std::string clip_cell_(std::string s, const PrintOptions& opt) {
  const std::int64_t maxw = opt.pretty_cell_max_width;
  if (maxw <= 0) {
    return s;
  }
  const std::size_t m = static_cast<std::size_t>(maxw);
  if (s.size() <= m) {
    return s;
  }
  if (m <= 3) {
    return s.substr(0, m);
  }
  return s.substr(0, m - 3) + "...";
}

static std::string center_text_(const std::string& s, std::size_t width) {
  if (s.size() >= width) {
    return s;
  }
  const std::size_t pad = width - s.size();
  const std::size_t left = pad / 2;
  const std::size_t right = pad - left;
  return std::string(left, ' ') + s + std::string(right, ' ');
}

static DecimalParts decimal_parts_(const std::string& s) {
  DecimalParts out;
  if (s.empty()) {
    return out;
  }
  const std::size_t start = (s[0] == '-' || s[0] == '+') ? 1u : 0u;
  out.has_sign = (start == 1u);
  const std::size_t epos = s.find_first_of("eE");
  const std::size_t body_end = (epos == std::string::npos) ? s.size() : epos;
  const std::size_t dot = s.find('.', start);

  if (dot != std::string::npos && dot < body_end) {
    out.pre_digits = dot - start;
    out.post_digits = body_end - dot - 1;
  } else {
    out.pre_digits = (body_end >= start) ? (body_end - start) : 0u;
    out.post_digits = 0;
  }
  return out;
}

struct FloatPrintStats final {
  bool has_float_like{false};
  bool has_finite{false};
  double min_abs_nonzero{std::numeric_limits<double>::infinity()};
  double max_abs{0.0};
};

static void update_float_stats_(double v, FloatPrintStats& st) {
  st.has_float_like = true;
  if (!std::isfinite(v)) {
    return;
  }
  st.has_finite = true;
  const double a = std::fabs(v);
  st.max_abs = std::max(st.max_abs, a);
  if (a > 0.0) {
    st.min_abs_nonzero = std::min(st.min_abs_nonzero, a);
  }
}

static FloatPrintStats collect_float_stats_(const Series& s) {
  FloatPrintStats st;
  const std::int64_t n = s.numel();
  switch (s.scalar_type()) {
    case ScalarType::Float64: {
      const auto* p = reinterpret_cast<const double*>(s.raw_data());
      for (std::int64_t i = 0; i < n; ++i) update_float_stats_(p[i], st);
      break;
    }
    case ScalarType::Float32: {
      const auto* p = reinterpret_cast<const float*>(s.raw_data());
      for (std::int64_t i = 0; i < n; ++i) update_float_stats_(static_cast<double>(p[i]), st);
      break;
    }
    case ScalarType::Float16: {
      const auto* p = reinterpret_cast<const Float16*>(s.raw_data());
      for (std::int64_t i = 0; i < n; ++i) update_float_stats_(float16_to_float(p[i]), st);
      break;
    }
    case ScalarType::BFloat16: {
      const auto* p = reinterpret_cast<const BFloat16*>(s.raw_data());
      for (std::int64_t i = 0; i < n; ++i) update_float_stats_(bfloat16_to_float(p[i]), st);
      break;
    }
    case ScalarType::Complex64: {
      const auto* p = reinterpret_cast<const Complex64*>(s.raw_data());
      for (std::int64_t i = 0; i < n; ++i) {
        update_float_stats_(static_cast<double>(p[i].real()), st);
        update_float_stats_(static_cast<double>(p[i].imag()), st);
      }
      break;
    }
    case ScalarType::Complex128: {
      const auto* p = reinterpret_cast<const Complex128*>(s.raw_data());
      for (std::int64_t i = 0; i < n; ++i) {
        update_float_stats_(p[i].real(), st);
        update_float_stats_(p[i].imag(), st);
      }
      break;
    }
    default:
      break;
  }
  return st;
}

static PrintOptions resolve_pretty_default_policy_(const Series& s, const PrintOptions& opt) {
  PrintOptions out = opt;
  const PrintOptions defaults{};
  const bool user_precision_set = (opt.precision != defaults.precision);

  // print_series default: torch-like readability baseline.
  if (!user_precision_set) {
    out.precision = 4;
  }
  if (out.pretty_align == defaults.pretty_align) {
    out.pretty_align = PrintAlignMode::Decimal;
  }

  const bool summary_defaults_unchanged =
      opt.pretty_matrix_max_rows == defaults.pretty_matrix_max_rows &&
      opt.pretty_matrix_max_cols == defaults.pretty_matrix_max_cols &&
      opt.pretty_row_head_items == defaults.pretty_row_head_items &&
      opt.pretty_row_tail_items == defaults.pretty_row_tail_items &&
      opt.pretty_col_head_items == defaults.pretty_col_head_items &&
      opt.pretty_col_tail_items == defaults.pretty_col_tail_items;
  const bool user_requested_wider_precision = user_precision_set && (opt.precision > 4);

  // Default summary policy (when user didn't ask for wider precision):
  // rows: >10 => keep first 5, last 1; cols: >7 => keep first 5, last 1.
  if (summary_defaults_unchanged && !user_requested_wider_precision) {
    out.pretty_matrix_max_rows = 10;
    out.pretty_matrix_max_cols = 7;
    out.pretty_row_head_items = 5;
    out.pretty_row_tail_items = 1;
    out.pretty_col_head_items = 5;
    out.pretty_col_tail_items = 1;
  }

  // In default General mode, choose fixed vs scientific from data scale.
  if (out.number_mode == PrintNumberMode::General) {
    const FloatPrintStats st = collect_float_stats_(s);
    if (st.has_float_like && st.has_finite) {
      const std::int64_t p = std::max<std::int64_t>(0, out.precision);
      // Switch to scientific only when fixed formatting would hide nonzero values as 0.000...,
      // or when magnitudes get very large for the chosen precision.
      const double zero_round_threshold = 0.5 * std::pow(10.0, -static_cast<double>(p));
      const double large_threshold = std::pow(10.0, static_cast<double>(p + 2));
      const bool too_small_for_fixed =
          std::isfinite(st.min_abs_nonzero) && st.min_abs_nonzero < zero_round_threshold;
      const bool too_large_for_fixed = st.max_abs >= large_threshold;
      out.number_mode =
          (too_small_for_fixed || too_large_for_fixed) ? PrintNumberMode::Scientific : PrintNumberMode::Fixed;
    }
  }

  return out;
}

static std::string maybe_colorize_(const std::string& text, const ScalarText& c, const PrintOptions& opt) {
  if (!opt.enable_color) {
    return text;
  }

  const char* code = nullptr;
  if (opt.color_special && c.special) {
    code = kAnsiYellow;
  } else if (opt.color_negative && c.negative) {
    code = kAnsiRed;
  }

  if (!code) {
    return text;
  }
  return std::string(code) + text + kAnsiReset;
}

static VisiblePlan make_visible_plan_(std::int64_t n,
                                      std::int64_t max_visible,
                                      std::int64_t head_items,
                                      std::int64_t tail_items) {
  VisiblePlan plan;
  if (n <= 0) {
    return plan;
  }

  const std::int64_t safe_head = std::max<std::int64_t>(0, head_items);
  const std::int64_t safe_tail = std::max<std::int64_t>(0, tail_items);
  const bool summarize = (max_visible > 0 && n > max_visible && n > (safe_head + safe_tail));

  if (!summarize) {
    plan.head.reserve(static_cast<std::size_t>(n));
    for (std::int64_t i = 0; i < n; ++i) {
      plan.head.push_back(i);
    }
    return plan;
  }

  const std::int64_t h = std::min<std::int64_t>(safe_head, n);
  const std::int64_t t = std::min<std::int64_t>(safe_tail, n - h);

  plan.head.reserve(static_cast<std::size_t>(h));
  for (std::int64_t i = 0; i < h; ++i) {
    plan.head.push_back(i);
  }

  plan.tail.reserve(static_cast<std::size_t>(t));
  for (std::int64_t i = n - t; i < n; ++i) {
    plan.tail.push_back(i);
  }
  plan.has_ellipsis = true;
  return plan;
}

static std::vector<std::int64_t> make_display_tokens_(const VisiblePlan& plan) {
  std::vector<std::int64_t> out;
  out.reserve(plan.head.size() + plan.tail.size() + (plan.has_ellipsis ? 1u : 0u));
  out.insert(out.end(), plan.head.begin(), plan.head.end());
  if (plan.has_ellipsis) {
    out.push_back(-1);
  }
  out.insert(out.end(), plan.tail.begin(), plan.tail.end());
  return out;
}

static std::vector<std::int64_t> make_uniform_slice_tokens_(std::int64_t n, std::int64_t max_slices) {
  std::vector<std::int64_t> out;
  if (n <= 0) {
    return out;
  }
  if (max_slices <= 0 || n <= max_slices) {
    out.reserve(static_cast<std::size_t>(n));
    for (std::int64_t i = 0; i < n; ++i) out.push_back(i);
    return out;
  }

  const std::int64_t k = std::max<std::int64_t>(2, max_slices);
  std::vector<std::int64_t> picked;
  picked.reserve(static_cast<std::size_t>(k));
  for (std::int64_t i = 0; i < k; ++i) {
    const double t = static_cast<double>(i) / static_cast<double>(k - 1);
    const std::int64_t idx = static_cast<std::int64_t>(std::llround(t * static_cast<double>(n - 1)));
    if (picked.empty() || picked.back() != idx) {
      picked.push_back(idx);
    }
  }

  if (picked.front() != 0) {
    picked.insert(picked.begin(), 0);
  }
  if (picked.back() != n - 1) {
    picked.push_back(n - 1);
  }

  out.reserve(picked.size() * 2);
  for (std::size_t i = 0; i < picked.size(); ++i) {
    if (i > 0 && picked[i] != picked[i - 1] + 1) {
      out.push_back(-1);
    }
    out.push_back(picked[i]);
  }
  return out;
}

static std::vector<std::int64_t> make_slice_tokens_(std::int64_t n, const PrintOptions& opt) {
  if (opt.pretty_slice_sampling == PrintSliceSampling::Uniform) {
    return make_uniform_slice_tokens_(n, opt.pretty_max_slices);
  }

  const VisiblePlan slice_plan =
      make_visible_plan_(n, opt.pretty_max_slices, opt.pretty_slice_head_items, opt.pretty_slice_tail_items);
  return make_display_tokens_(slice_plan);
}

static LegacySummaryConfig legacy_summary_config_(const Series& s,
                                                  const std::vector<std::int64_t>& sizes,
                                                  std::int64_t dim,
                                                  const PrintOptions& opt) {
  LegacySummaryConfig cfg;
  const std::int64_t n = sizes[static_cast<std::size_t>(dim)];
  const std::int64_t rank = static_cast<std::int64_t>(sizes.size());

  if (rank >= 2 && dim == rank - 2) {
    const bool summarize = n > opt.legacy_rank2_max_rows &&
                           n > (opt.legacy_rank2_head_items + opt.legacy_rank2_tail_items);
    cfg.summarize = summarize;
    cfg.head = summarize ? opt.legacy_rank2_head_items : n;
    cfg.tail_start = summarize ? (n - opt.legacy_rank2_tail_items) : n;
    return cfg;
  }

  if (rank >= 2 && dim == rank - 1) {
    const bool summarize = n > opt.legacy_rank2_max_cols &&
                           n > (opt.legacy_rank2_head_items + opt.legacy_rank2_tail_items);
    cfg.summarize = summarize;
    cfg.head = summarize ? opt.legacy_rank2_head_items : n;
    cfg.tail_start = summarize ? (n - opt.legacy_rank2_tail_items) : n;
    return cfg;
  }

  cfg.summarize = (s.numel() > opt.max_elements && n > 2 * opt.edge_items);
  cfg.head = cfg.summarize ? opt.edge_items : n;
  cfg.tail_start = cfg.summarize ? (n - opt.edge_items) : n;
  return cfg;
}

static std::string scalar_text_for_legacy_(const Series& s, std::int64_t off, const PrintOptions& opt) {
  return scalar_text_at_(s, off, opt).text;
}

static void format_recursive_(std::ostringstream& oss,
                              const Series& s,
                              const std::vector<std::int64_t>& sizes,
                              const std::vector<std::int64_t>& strides,
                              std::int64_t dim,
                              std::int64_t base_off,
                              const PrintOptions& opt) {
  if (dim == static_cast<std::int64_t>(sizes.size())) {
    oss << scalar_text_for_legacy_(s, base_off, opt);
    return;
  }

  const auto n = sizes[static_cast<std::size_t>(dim)];
  const auto st = strides[static_cast<std::size_t>(dim)];

  oss << "[";
  const LegacySummaryConfig summary = legacy_summary_config_(s, sizes, dim, opt);
  for (std::int64_t i = 0; i < summary.head; ++i) {
    if (i > 0) {
      oss << (dim == static_cast<std::int64_t>(sizes.size()) - 1 ? ", " : ",\n  ");
    }
    format_recursive_(oss, s, sizes, strides, dim + 1, base_off + i * st, opt);
  }

  if (summary.summarize) {
    oss << (dim == static_cast<std::int64_t>(sizes.size()) - 1 ? ", ... , " : ",\n  ...\n ");
    for (std::int64_t i = summary.tail_start; i < n; ++i) {
      if (i > summary.tail_start) {
        oss << (dim == static_cast<std::int64_t>(sizes.size()) - 1 ? ", " : ",\n  ");
      }
      format_recursive_(oss, s, sizes, strides, dim + 1, base_off + i * st, opt);
    }
  }

  oss << "]";
}

static const char* left_bracket_(PrintBracketStyle style, std::size_t row, std::size_t rows) {
  if (style == PrintBracketStyle::None) return "";
  if (style == PrintBracketStyle::Ascii) {
    if (rows <= 1) return "[";
    if (row == 0) return "/";
    if (row + 1 == rows) return "\\";
    return "|";
  }

  if (rows <= 1) return "[";
  if (row == 0) return "⎡";
  if (row + 1 == rows) return "⎣";
  return "⎢";
}

static const char* right_bracket_(PrintBracketStyle style, std::size_t row, std::size_t rows) {
  if (style == PrintBracketStyle::None) return "";
  if (style == PrintBracketStyle::Ascii) {
    if (rows <= 1) return "]";
    if (row == 0) return "\\";
    if (row + 1 == rows) return "/";
    return "|";
  }

  if (rows <= 1) return "]";
  if (row == 0) return "⎤";
  if (row + 1 == rows) return "⎦";
  return "⎥";
}

static std::vector<std::int64_t> decode_index_(std::int64_t linear,
                                                const std::vector<std::int64_t>& dims) {
  std::vector<std::int64_t> out(dims.size(), 0);
  for (std::size_t rev = 0; rev < dims.size(); ++rev) {
    const std::size_t d = dims.size() - 1 - rev;
    const std::int64_t dim = dims[d];
    if (dim > 0) {
      out[d] = linear % dim;
      linear /= dim;
    }
  }
  return out;
}

static std::int64_t estimate_fit_cols_(const Series& s,
                                       std::int64_t base_off,
                                       std::int64_t cols,
                                       std::int64_t col_stride,
                                       const PrintOptions& opt) {
  if (opt.pretty_max_line_width <= 0 || cols <= 0) {
    return 0;
  }

  const std::int64_t sample_cols = std::min<std::int64_t>(cols, 8);
  std::size_t total = 0;
  for (std::int64_t c = 0; c < sample_cols; ++c) {
    ScalarText txt = scalar_text_at_(s, base_off + c * col_stride, opt);
    txt.text = clip_cell_(std::move(txt.text), opt);
    total += txt.text.size();
  }

  const std::size_t avg = std::max<std::size_t>(1, total / static_cast<std::size_t>(sample_cols));
  const std::size_t sep = opt.pretty_values_with_commas ? 2u : 1u;
  const std::size_t overhead = 6u; // brackets + spaces
  const std::size_t linew = static_cast<std::size_t>(opt.pretty_max_line_width);
  if (linew <= overhead) {
    return 1;
  }

  const std::size_t fit = (linew - overhead + sep) / (avg + sep);
  return static_cast<std::int64_t>(std::max<std::size_t>(1, fit));
}

static void format_matrix_block_(std::ostringstream& oss,
                                 const Series& s,
                                 std::int64_t base_off,
                                 std::int64_t rows,
                                 std::int64_t cols,
                                 std::int64_t row_stride,
                                 std::int64_t col_stride,
                                 const PrintOptions& opt,
                                 std::int64_t chunk_begin,
                                 std::int64_t chunk_cols,
                                 bool force_all_cols) {
  const VisiblePlan row_plan = make_visible_plan_(
      rows, opt.pretty_matrix_max_rows, opt.pretty_row_head_items, opt.pretty_row_tail_items);
  const auto row_tokens = make_display_tokens_(row_plan);

  std::int64_t effective_max_cols = force_all_cols ? 0 : opt.pretty_matrix_max_cols;
  if (opt.pretty_max_line_width > 0) {
    const std::int64_t fit = estimate_fit_cols_(s, base_off, cols, col_stride, opt);
    if (fit > 0) {
      effective_max_cols = (effective_max_cols > 0) ? std::min(effective_max_cols, fit) : fit;
    }
  }

  std::vector<std::int64_t> col_tokens;
  if (force_all_cols) {
    col_tokens.reserve(static_cast<std::size_t>(chunk_cols));
    for (std::int64_t c = 0; c < chunk_cols; ++c) {
      col_tokens.push_back(chunk_begin + c);
    }
  } else {
    const VisiblePlan col_plan = make_visible_plan_(
        cols, effective_max_cols, opt.pretty_col_head_items, opt.pretty_col_tail_items);
    col_tokens = make_display_tokens_(col_plan);
  }

  const std::size_t out_rows = row_tokens.size();
  const std::size_t out_cols = col_tokens.size();

  std::vector<std::vector<ScalarText>> cells(out_rows, std::vector<ScalarText>(out_cols));
  std::vector<std::size_t> col_widths(out_cols, 3u);
  std::vector<std::size_t> col_pre(out_cols, 0u);
  std::vector<std::size_t> col_post(out_cols, 0u);
  std::vector<std::size_t> col_sign(out_cols, 0u);

  for (std::size_t r = 0; r < out_rows; ++r) {
    for (std::size_t c = 0; c < out_cols; ++c) {
      ScalarText cell;
      if (row_tokens[r] < 0 || col_tokens[c] < 0) {
        cell.text = "...";
        cell.ellipsis = true;
      } else {
        const std::int64_t off = base_off + row_tokens[r] * row_stride + col_tokens[c] * col_stride;
        cell = scalar_text_at_(s, off, opt);
      }
      cell.text = clip_cell_(std::move(cell.text), opt);

      const std::size_t len = cell.text.size();
      col_widths[c] = std::max(col_widths[c], len);
      if (cell.numeric &&
          (opt.pretty_align == PrintAlignMode::Decimal || opt.pretty_align == PrintAlignMode::Left)) {
        const DecimalParts dp = decimal_parts_(cell.text);
        col_pre[c] = std::max(col_pre[c], dp.pre_digits);
        col_post[c] = std::max(col_post[c], dp.post_digits);
        col_sign[c] = std::max(col_sign[c], static_cast<std::size_t>(dp.has_sign ? 1 : 0));
      }
      cells[r][c] = std::move(cell);
    }
  }

  for (std::size_t c = 0; c < out_cols; ++c) {
    if (opt.pretty_align == PrintAlignMode::Decimal || opt.pretty_align == PrintAlignMode::Left) {
      const std::size_t w = col_sign[c] + col_pre[c] + (col_post[c] > 0 ? (1 + col_post[c]) : 0);
      col_widths[c] = std::max(col_widths[c], w);
    }
  }

  const std::string sep = opt.pretty_values_with_commas ? ", " : " ";
  for (std::size_t r = 0; r < out_rows; ++r) {
    const char* lbr = left_bracket_(opt.pretty_bracket_style, r, out_rows);
    const char* rbr = right_bracket_(opt.pretty_bracket_style, r, out_rows);

    if (opt.pretty_bracket_style == PrintBracketStyle::None) {
      oss << "  ";
    } else {
      oss << lbr << " ";
    }

    for (std::size_t c = 0; c < out_cols; ++c) {
      const ScalarText& cell = cells[r][c];
      std::string padded;

      if (cell.ellipsis) {
        padded = center_text_(cell.text, col_widths[c]);
      } else if (opt.pretty_align == PrintAlignMode::Left && cell.numeric) {
        const DecimalParts dp = decimal_parts_(cell.text);
        const std::size_t sign_width = dp.has_sign ? 1u : 0u;
        const std::size_t lead =
            (col_sign[c] - std::min(col_sign[c], sign_width)) + (col_pre[c] - dp.pre_digits);
        padded.assign(lead, ' ');
        padded += cell.text;
        if (col_post[c] > dp.post_digits) {
          padded.append(col_post[c] - dp.post_digits, ' ');
        }
        if (padded.size() < col_widths[c]) {
          padded.append(col_widths[c] - padded.size(), ' ');
        }
      } else if (opt.pretty_align == PrintAlignMode::Left) {
        padded = cell.text;
        if (padded.size() < col_widths[c]) {
          padded.append(col_widths[c] - padded.size(), ' ');
        }
      } else if (opt.pretty_align == PrintAlignMode::Decimal && cell.numeric) {
        const DecimalParts dp = decimal_parts_(cell.text);
        const std::size_t sign_width = dp.has_sign ? 1u : 0u;
        const std::size_t lead =
            (col_sign[c] - std::min(col_sign[c], sign_width)) + (col_pre[c] - dp.pre_digits);
        padded.assign(lead, ' ');
        padded += cell.text;
        if (col_post[c] > dp.post_digits) {
          padded.append(col_post[c] - dp.post_digits, ' ');
        }
      } else {
        std::ostringstream ss;
        ss << std::setw(static_cast<int>(col_widths[c])) << std::right << cell.text;
        padded = ss.str();
      }

      oss << maybe_colorize_(padded, cell, opt);
      if (c + 1 < out_cols) {
        oss << sep;
      }
    }

    if (opt.pretty_bracket_style != PrintBracketStyle::None) {
      oss << " " << rbr;
    }

    if (r + 1 < out_rows) {
      oss << "\n";
    }
  }
}

static void format_matrix_with_wrap_(std::ostringstream& oss,
                                     const Series& s,
                                     std::int64_t base_off,
                                     std::int64_t rows,
                                     std::int64_t cols,
                                     std::int64_t row_stride,
                                     std::int64_t col_stride,
                                     const PrintOptions& opt) {
  if (!(opt.pretty_auto_wrap && opt.pretty_max_line_width > 0 && opt.pretty_matrix_max_cols <= 0)) {
    format_matrix_block_(oss, s, base_off, rows, cols, row_stride, col_stride, opt, 0, 0, false);
    return;
  }

  const std::int64_t fit = estimate_fit_cols_(s, base_off, cols, col_stride, opt);
  if (fit <= 0 || cols <= fit) {
    format_matrix_block_(oss, s, base_off, rows, cols, row_stride, col_stride, opt, 0, 0, false);
    return;
  }

  for (std::int64_t start = 0; start < cols; start += fit) {
    const std::int64_t width = std::min<std::int64_t>(fit, cols - start);
    if (start > 0) {
      oss << "\n\n";
    }
    oss << "cols[" << start << ":" << (start + width - 1) << "]\n";
    format_matrix_block_(oss, s, base_off, rows, cols, row_stride, col_stride, opt, start, width, true);
  }
}

static void format_pretty_values_(std::ostringstream& oss,
                                  const Series& s,
                                  const std::vector<std::int64_t>& sizes,
                                  const std::vector<std::int64_t>& strides,
                                  const PrintOptions& opt) {
  const std::size_t rank = sizes.size();
  if (rank == 2) {
    oss << "values=\n";
    format_matrix_with_wrap_(oss, s, 0, sizes[0], sizes[1], strides[0], strides[1], opt);
    return;
  }

  std::vector<std::int64_t> prefix_dims(sizes.begin(), sizes.end() - 2);
  std::int64_t total_slices = 1;
  for (const auto d : prefix_dims) {
    total_slices *= d;
  }

  const auto slice_tokens = make_slice_tokens_(total_slices, opt);

  for (std::size_t si = 0; si < slice_tokens.size(); ++si) {
    const std::int64_t token = slice_tokens[si];
    if (token < 0) {
      oss << "...\n";
      continue;
    }

    const auto prefix = decode_index_(token, prefix_dims);
    if (opt.pretty_show_slice_indices) {
      oss << opt.pretty_slice_label_prefix << "[";
      for (std::size_t i = 0; i < prefix.size(); ++i) {
        if (i) oss << ", ";
        oss << prefix[i];
      }
      if (!prefix.empty()) oss << ", ";
      oss << ":, :]" << opt.pretty_slice_label_suffix << "\n";
    }

    std::int64_t base_off = 0;
    for (std::size_t i = 0; i < prefix.size(); ++i) {
      base_off += prefix[i] * strides[i];
    }

    format_matrix_with_wrap_(
        oss, s, base_off, sizes[rank - 2], sizes[rank - 1], strides[rank - 2], strides[rank - 1], opt);
    if (si + 1 < slice_tokens.size()) {
      oss << "\n\n";
    }
  }
}

static const char* platform_name_(Platform p) {
  switch (p) {
    case Platform::CPU: return "cpu";
    case Platform::CUDA: return "cuda";
    case Platform::MPS: return "mps";
    default: return "unknown";
  }
}

static bool show_header_(const PrintOptions& opt) {
  return opt.content_mode != PrintContentMode::ValuesOnly;
}

static bool show_values_(const PrintOptions& opt) {
  return opt.content_mode != PrintContentMode::HeaderOnly;
}

static void append_metadata_(std::ostringstream& oss, const Series& s, const PrintOptions& opt) {
  if (opt.metadata_mode == PrintMetadataMode::None) {
    return;
  }

  oss << "Series(";
  bool first_field = true;
  auto append_field = [&](const char* k, const std::string& v) {
    if (!first_field) oss << ", ";
    first_field = false;
    oss << k << "=" << v;
  };

  const bool compact = opt.metadata_mode == PrintMetadataMode::Compact;

  auto maybe_shape = [&]() {
    if (!(opt.show_shape || compact)) return;
    std::ostringstream v;
    v << "[";
    for (std::size_t i = 0; i < s.sizes().size(); ++i) {
      if (i) v << ", ";
      v << s.sizes()[i];
    }
    v << "]";
    append_field("shape", v.str());
  };

  auto maybe_dtype = [&]() {
    if (!(opt.show_dtype || compact)) return;
    append_field("dtype", std::string(dtype_of(s.scalar_type()).name));
  };

  if (compact) {
    maybe_shape();
    maybe_dtype();
    if (opt.show_numel) append_field("numel", std::to_string(s.numel()));
  } else {
    maybe_shape();
    if (opt.show_strides) {
      std::ostringstream v;
      v << "[";
      for (std::size_t i = 0; i < s.strides().size(); ++i) {
        if (i) v << ", ";
        v << s.strides()[i];
      }
      v << "]";
      append_field("strides", v.str());
    }
    if (opt.show_dtype) maybe_dtype();
    if (opt.show_platform) {
      const auto p = s.platform();
      append_field("platform", platform_name_(p));
    }
    if (opt.show_numel) append_field("numel", std::to_string(s.numel()));
    if (opt.show_contiguous) append_field("contiguous", s.is_contiguous() ? "true" : "false");
  }

  oss << ")";
}

static std::string render_series_string_(const Series& s,
                                         const PrintOptions& opt,
                                         bool use_pretty_values) {
  if (!s.defined()) return "Series(undefined)";

  std::ostringstream oss;
  if (show_header_(opt)) {
    append_metadata_(oss, s, opt);
    if (show_values_(opt)) {
      oss << "\n";
    }
  }

  if (!show_values_(opt)) {
    return oss.str();
  }

  if (s.numel() == 0) {
    oss << "[]";
    return oss.str();
  }

  auto c = s.contiguous();
  const std::vector<std::int64_t> sizes(c.sizes().begin(), c.sizes().end());
  const std::vector<std::int64_t> strides(c.strides().begin(), c.strides().end());

  if (use_pretty_values && sizes.size() >= 2) {
    format_pretty_values_(oss, c, sizes, strides, opt);
  } else {
    oss << "values=";
    format_recursive_(oss, c, sizes, strides, 0, 0, opt);
  }
  return oss.str();
}

} // namespace

std::string to_string(const Series& s, const PrintOptions& opt) {
  return render_series_string_(s, opt, /*use_pretty_values=*/false);
}

std::string print_series(const Series& s, const PrintOptions& opt) {
  const PrintOptions effective = resolve_pretty_default_policy_(s, opt);
  return render_series_string_(s, effective, /*use_pretty_values=*/true);
}

std::ostream& print_series(std::ostream& os, const Series& s, const PrintOptions& opt) {
  os << print_series(s, opt);
  return os;
}

std::ostream& operator<<(std::ostream& os, const Series& s) {
  os << to_string(s);
  return os;
}

} // namespace pluss::codem
