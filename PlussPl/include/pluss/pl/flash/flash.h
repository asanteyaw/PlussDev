#pragma once

#include "pluss/coden/note_factory.h"
#include "pluss/codeh/codeh.h"
#include "pluss/pl/studio.h"

#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <iterator>
#include <limits>
#include <map>
#include <numeric>
#include <optional>
#include <random>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace pluss::pl::flash {
namespace detail {

template <class T>
concept ArithmeticValue = std::is_arithmetic_v<std::remove_cvref_t<T>>;

template <class Range>
concept ArithmeticRange =
    std::ranges::input_range<Range> && ArithmeticValue<std::ranges::range_value_t<Range>>;

[[nodiscard]] inline Series to_series(const Series& values) {
  return values;
}

template <ArithmeticRange Range>
[[nodiscard]] inline Series to_series(Range&& values) {
  return ::pluss::pl::detail::to_series(std::forward<Range>(values));
}

[[nodiscard]] inline Series to_series(const Timeline& timeline) {
  return timeline.epoch_nanos().to(ScalarType::Float64).reshape({timeline.numel()});
}

[[nodiscard]] inline Series to_series(const TimeIndex& index) {
  return index.timeline().epoch_nanos().to(ScalarType::Float64).reshape({index.size()});
}

template <class T>
concept StringLike = std::is_convertible_v<T, std::string_view>;

template <class T>
concept HueSeriesLike =
    requires(T&& value) {
      { to_series(std::forward<T>(value)) } -> std::same_as<Series>;
    } && !StringLike<T>;

[[nodiscard]] inline std::vector<std::string> note_values(const Note& note) {
  const auto values = note.vector();
  std::vector<std::string> out;
  out.reserve(values.size());
  for (const std::string_view value : values) {
    out.emplace_back(value);
  }
  return out;
}

[[nodiscard]] inline std::vector<double> to_f64_vector(const Series& values) {
  return values.to(ScalarType::Float64).to_vec<double>();
}

[[nodiscard]] inline Series flatten_f64(const Series& values) {
  return values.to(ScalarType::Float64).reshape({values.numel()});
}

[[nodiscard]] inline std::pair<std::vector<double>, std::vector<std::string>> encode_categories(
    const Note& categories) {
  const std::vector<std::string> labels = note_values(categories);
  std::vector<double> encoded;
  encoded.reserve(labels.size());
  std::vector<std::string> uniques;
  std::map<std::string, std::int64_t> index_of;
  for (const std::string& label : labels) {
    const auto [it, inserted] = index_of.emplace(label, static_cast<std::int64_t>(index_of.size()));
    if (inserted) {
      uniques.push_back(label);
    }
    encoded.push_back(static_cast<double>(it->second + 1));
  }
  return {std::move(encoded), std::move(uniques)};
}

inline void require_equal_xy_hue_lengths(
    std::string_view name,
    const Series& x,
    const Series& y,
    const Series& hue) {
  if (x.numel() != y.numel() || x.numel() != hue.numel()) {
    throw std::invalid_argument(
        std::string(name) + " requires x, y, and hue to have identical lengths");
  }
}

[[nodiscard]] inline Rgba hsl_to_rgb(double h, double s, double l) noexcept;

[[nodiscard]] inline std::vector<Rgba> categorical_colors(std::int64_t count) {
  if (count <= 0) {
    return {};
  }
  std::vector<Rgba> colors;
  colors.reserve(static_cast<std::size_t>(count));
  for (const auto i : std::views::iota(std::int64_t{0}, count)) {
    const double h = static_cast<double>(i) / static_cast<double>(std::max<std::int64_t>(count, 1));
    colors.push_back(hsl_to_rgb(h, 0.68, 0.50));
  }
  return colors;
}

[[nodiscard]] inline std::string compact_number_label(double value) {
  const double rounded = std::round(value);
  if (std::abs(value - rounded) < 1e-9) {
    return std::to_string(static_cast<std::int64_t>(rounded));
  }
  std::string text = std::to_string(value);
  while (!text.empty() && text.back() == '0') {
    text.pop_back();
  }
  if (!text.empty() && text.back() == '.') {
    text.pop_back();
  }
  return text;
}

[[nodiscard]] inline std::vector<double> ordinal_levels(std::int64_t count, double first = 0.0) {
  if (count <= 0) {
    return {};
  }
  std::vector<double> levels;
  levels.reserve(static_cast<std::size_t>(count));
  for (const auto i : std::views::iota(std::int64_t{0}, count)) {
    levels.push_back(first + static_cast<double>(i));
  }
  return levels;
}

[[nodiscard]] inline std::vector<double> linspace(double low, double high, std::int64_t count);

[[nodiscard]] inline std::pair<Series, std::vector<std::string>> numeric_hue_bins(
    const Series& hue_values,
    std::int64_t bin_count) {
  const Series hue = flatten_f64(hue_values);
  if (hue.numel() == 0) {
    return {hue, {}};
  }
  const std::int64_t bins = std::max<std::int64_t>(bin_count, 2);
  const double low = to_f64_vector(hue.min())[0];
  const double high = to_f64_vector(hue.max())[0];
  if (!(high > low)) {
    return {pluss::full({hue.numel()}, 0.0, ScalarType::Float64), {"hue=" + compact_number_label(low)}};
  }

  const Series codes = hue.sub(low)
                           .div(high - low)
                           .mul(static_cast<double>(bins))
                           .floor()
                           .clamp(0.0, static_cast<double>(bins - 1))
                           .to(ScalarType::Float64);
  const std::vector<double> edges = linspace(low, high, bins + 1);
  std::vector<std::string> labels;
  labels.reserve(static_cast<std::size_t>(bins));
  for (const auto i : std::views::iota(std::int64_t{0}, bins)) {
    const double start = edges[static_cast<std::size_t>(i)];
    const double stop = edges[static_cast<std::size_t>(i + 1)];
    const std::string left = compact_number_label(start);
    const std::string right = compact_number_label(stop);
    if (i == bins - 1) {
      labels.push_back("[" + left + ", " + right + "]");
    } else {
      labels.push_back("[" + left + ", " + right + ")");
    }
  }
  return {codes, std::move(labels)};
}

enum class HueRenderKind : std::uint8_t { Scatter, Line };

inline Trace& draw_grouped_hue(
    Pane& pane,
    HueRenderKind render_kind,
    const Series& x,
    const Series& y,
    const Series& hue_codes,
    const std::vector<double>& levels,
    const std::vector<std::string>& labels) {
  if (levels.empty() || levels.size() != labels.size()) {
    throw std::invalid_argument("flash::draw_grouped_hue requires aligned non-empty levels/labels");
  }
  const auto colors = categorical_colors(static_cast<std::int64_t>(levels.size()));
  Trace* last = nullptr;
  for (const auto i : std::views::iota(std::size_t{0}, levels.size())) {
    const Series mask = hue_codes.eq(levels[i]);
    const Series grouped_x = x.masked_select(mask);
    if (grouped_x.numel() == 0) {
      continue;
    }
    const Series grouped_y = y.masked_select(mask);
    Trace* trace = nullptr;
    if (render_kind == HueRenderKind::Line) {
      trace = &pane.trend(grouped_x, grouped_y);
    } else {
      trace = &pane.cloud(grouped_x, grouped_y);
    }
    auto& out = *trace;
    out.stroke(colors[i]).label(labels[i]);
    if (render_kind == HueRenderKind::Line) {
      out.width(1.6F);
    } else {
      out.width(0.55F);
    }
    last = &out;
  }
  if (last == nullptr) {
    throw std::invalid_argument("flash::draw_grouped_hue produced no traces");
  }
  return *last;
}

[[nodiscard]] inline std::vector<double> linspace(double low, double high, std::int64_t count) {
  if (count <= 0) {
    return {};
  }
  if (count == 1) {
    return {low};
  }
  std::vector<double> out(static_cast<std::size_t>(count), low);
  const double step = (high - low) / static_cast<double>(count - 1);
  for (std::int64_t i = 0; i < count; ++i) {
    out[static_cast<std::size_t>(i)] = low + step * static_cast<double>(i);
  }
  return out;
}

[[nodiscard]] inline std::pair<double, double> linear_fit(const Series& x, const Series& y) {
  const std::vector<double> vx = to_f64_vector(x);
  const std::vector<double> vy = to_f64_vector(y);
  if (vx.size() != vy.size() || vx.empty()) {
    throw std::invalid_argument("flash::linear_fit requires equal non-empty x/y vectors");
  }

  const double n = static_cast<double>(vx.size());
  double sum_x = 0.0;
  double sum_y = 0.0;
  double sum_xx = 0.0;
  double sum_xy = 0.0;
  for (std::size_t i = 0; i < vx.size(); ++i) {
    sum_x += vx[i];
    sum_y += vy[i];
    sum_xx += vx[i] * vx[i];
    sum_xy += vx[i] * vy[i];
  }
  const double denom = n * sum_xx - sum_x * sum_x;
  if (std::abs(denom) < 1e-12) {
    return {0.0, sum_y / n};
  }
  const double slope = (n * sum_xy - sum_x * sum_y) / denom;
  const double intercept = (sum_y - slope * sum_x) / n;
  return {slope, intercept};
}

[[nodiscard]] inline std::pair<Series, Series> finite_xy(const Series& x, const Series& y) {
  const Series sx = flatten_f64(x);
  const Series sy = flatten_f64(y);
  if (sx.numel() != sy.numel()) {
    throw std::invalid_argument("flash::finite_xy requires equal length x/y");
  }
  const Series mask = sx.isfinite().logical_and(sy.isfinite());
  return {sx.masked_select(mask), sy.masked_select(mask)};
}

[[nodiscard]] inline std::vector<double> solve_linear_system(
    std::vector<double> a,
    std::vector<double> b,
    std::int64_t n) {
  if (n <= 0) {
    return {};
  }
  for (const auto k : std::views::iota(std::int64_t{0}, n)) {
    std::int64_t pivot = k;
    double pivot_abs = std::abs(a[static_cast<std::size_t>(k * n + k)]);
    for (const auto row : std::views::iota(k + 1, n)) {
      const double candidate = std::abs(a[static_cast<std::size_t>(row * n + k)]);
      if (candidate > pivot_abs) {
        pivot = row;
        pivot_abs = candidate;
      }
    }
    if (pivot_abs < 1e-12) {
      return std::vector<double>(static_cast<std::size_t>(n), 0.0);
    }
    if (pivot != k) {
      for (const auto col : std::views::iota(std::int64_t{0}, n)) {
        std::swap(a[static_cast<std::size_t>(k * n + col)], a[static_cast<std::size_t>(pivot * n + col)]);
      }
      std::swap(b[static_cast<std::size_t>(k)], b[static_cast<std::size_t>(pivot)]);
    }
    const double diag = a[static_cast<std::size_t>(k * n + k)];
    for (const auto row : std::views::iota(k + 1, n)) {
      const double factor = a[static_cast<std::size_t>(row * n + k)] / diag;
      if (std::abs(factor) < 1e-18) {
        continue;
      }
      for (const auto col : std::views::iota(k, n)) {
        a[static_cast<std::size_t>(row * n + col)] -= factor * a[static_cast<std::size_t>(k * n + col)];
      }
      b[static_cast<std::size_t>(row)] -= factor * b[static_cast<std::size_t>(k)];
    }
  }
  std::vector<double> x(static_cast<std::size_t>(n), 0.0);
  for (auto row = n; row-- > 0;) {
    double acc = b[static_cast<std::size_t>(row)];
    for (const auto col : std::views::iota(row + 1, n)) {
      acc -= a[static_cast<std::size_t>(row * n + col)] * x[static_cast<std::size_t>(col)];
    }
    const double diag = a[static_cast<std::size_t>(row * n + row)];
    x[static_cast<std::size_t>(row)] = std::abs(diag) < 1e-12 ? 0.0 : (acc / diag);
  }
  return x;
}

[[nodiscard]] inline std::vector<double> polyfit_coeffs(
    const std::vector<double>& x,
    const std::vector<double>& y,
    std::int64_t degree,
    const std::vector<double>* weights = nullptr) {
  if (x.size() != y.size() || x.empty()) {
    throw std::invalid_argument("flash::polyfit_coeffs requires equal non-empty x/y");
  }
  const auto d = std::max<std::int64_t>(1, degree);
  const auto p = d + 1;
  std::vector<double> a(static_cast<std::size_t>(p * p), 0.0);
  std::vector<double> b(static_cast<std::size_t>(p), 0.0);
  std::vector<double> powers(static_cast<std::size_t>(2 * d + 1), 0.0);
  for (const auto i : std::views::iota(std::size_t{0}, x.size())) {
    const double w = (weights == nullptr || i >= weights->size()) ? 1.0 : std::max(0.0, (*weights)[i]);
    if (!(w > 0.0)) {
      continue;
    }
    powers[0] = 1.0;
    for (const auto j : std::views::iota(std::int64_t{1}, 2 * d + 1)) {
      powers[static_cast<std::size_t>(j)] = powers[static_cast<std::size_t>(j - 1)] * x[i];
    }
    for (const auto row : std::views::iota(std::int64_t{0}, p)) {
      for (const auto col : std::views::iota(std::int64_t{0}, p)) {
        a[static_cast<std::size_t>(row * p + col)] += w * powers[static_cast<std::size_t>(row + col)];
      }
      b[static_cast<std::size_t>(row)] += w * y[i] * powers[static_cast<std::size_t>(row)];
    }
  }
  return solve_linear_system(std::move(a), std::move(b), p);
}

[[nodiscard]] inline std::vector<double> poly_eval(
    const std::vector<double>& coeffs,
    const std::vector<double>& x) {
  std::vector<double> out(x.size(), 0.0);
  std::transform(x.begin(), x.end(), out.begin(), [&](const double x_i) {
    double y_i = 0.0;
    for (auto idx = coeffs.size(); idx-- > 0;) {
      y_i = y_i * x_i + coeffs[idx];
    }
    return y_i;
  });
  return out;
}

[[nodiscard]] inline std::vector<double> median_absolute_deviation_weights(
    const std::vector<double>& residuals) {
  std::vector<double> abs_res(residuals.size(), 0.0);
  std::transform(residuals.begin(), residuals.end(), abs_res.begin(), [](const double r) { return std::abs(r); });
  std::vector<double> sorted = abs_res;
  std::sort(sorted.begin(), sorted.end());
  double mad = 0.0;
  if (!sorted.empty()) {
    const auto mid = sorted.size() / 2;
    mad = (sorted.size() % 2 == 0)
              ? (0.5 * (sorted[mid - 1] + sorted[mid]))
              : sorted[mid];
  }
  if (!(mad > 1e-12)) {
    return std::vector<double>(residuals.size(), 1.0);
  }
  std::vector<double> w(residuals.size(), 0.0);
  std::transform(residuals.begin(), residuals.end(), w.begin(), [&](const double r) {
    const double u = std::abs(r) / (6.0 * mad);
    if (u >= 1.0) {
      return 0.0;
    }
    const double t = 1.0 - u * u;
    return t * t;
  });
  return w;
}

[[nodiscard]] inline std::vector<double> fit_poly_predict(
    const std::vector<double>& x,
    const std::vector<double>& y,
    const std::vector<double>& query,
    std::int64_t degree,
    bool robust) {
  if (!robust) {
    return poly_eval(polyfit_coeffs(x, y, degree), query);
  }
  std::vector<double> weights(x.size(), 1.0);
  std::vector<double> coeffs = polyfit_coeffs(x, y, degree, &weights);
  for (const auto _ : std::views::iota(0, 3)) {
    const auto pred_train = poly_eval(coeffs, x);
    std::vector<double> residuals(y.size(), 0.0);
    std::transform(y.begin(), y.end(), pred_train.begin(), residuals.begin(), [](const double a, const double b) {
      return a - b;
    });
    weights = median_absolute_deviation_weights(residuals);
    coeffs = polyfit_coeffs(x, y, degree, &weights);
  }
  return poly_eval(coeffs, query);
}

[[nodiscard]] inline std::vector<double> lowess_predict(
    const std::vector<double>& x,
    const std::vector<double>& y,
    const std::vector<double>& query,
    double frac,
    const std::vector<double>* robust_weights = nullptr) {
  if (x.size() != y.size() || x.empty()) {
    throw std::invalid_argument("flash::lowess_predict requires equal non-empty x/y");
  }
  const std::int64_t n = static_cast<std::int64_t>(x.size());
  const std::int64_t span = std::clamp<std::int64_t>(
      static_cast<std::int64_t>(std::ceil(std::clamp(frac, 0.05, 1.0) * static_cast<double>(n))),
      2,
      n);
  std::vector<double> out(query.size(), 0.0);
  std::vector<double> distances(x.size(), 0.0);
  for (const auto qi : std::views::iota(std::size_t{0}, query.size())) {
    std::transform(x.begin(), x.end(), distances.begin(), [&](const double x_i) {
      return std::abs(x_i - query[qi]);
    });
    std::vector<double> dist_copy = distances;
    std::nth_element(
        dist_copy.begin(),
        dist_copy.begin() + static_cast<std::ptrdiff_t>(span - 1),
        dist_copy.end());
    double h = dist_copy[static_cast<std::size_t>(span - 1)];
    if (!(h > 0.0)) {
      h = *std::max_element(distances.begin(), distances.end());
    }
    h = std::max(h, 1e-9);
    double sw = 0.0;
    double swx = 0.0;
    double swy = 0.0;
    double swxx = 0.0;
    double swxy = 0.0;
    for (const auto i : std::views::iota(std::size_t{0}, x.size())) {
      const double u = std::clamp(distances[i] / h, 0.0, 1.0);
      const double tri = std::pow(1.0 - u * u * u, 3.0);
      const double rw =
          (robust_weights == nullptr || i >= robust_weights->size()) ? 1.0 : std::max(0.0, (*robust_weights)[i]);
      const double w = tri * rw;
      if (!(w > 0.0)) {
        continue;
      }
      sw += w;
      swx += w * x[i];
      swy += w * y[i];
      swxx += w * x[i] * x[i];
      swxy += w * x[i] * y[i];
    }
    if (!(sw > 0.0)) {
      out[qi] = 0.0;
      continue;
    }
    const double denom = sw * swxx - swx * swx;
    if (std::abs(denom) < 1e-12) {
      out[qi] = swy / sw;
      continue;
    }
    const double slope = (sw * swxy - swx * swy) / denom;
    const double intercept = (swy - slope * swx) / sw;
    out[qi] = intercept + slope * query[qi];
  }
  return out;
}

[[nodiscard]] inline std::vector<double> regression_predict(
    const std::vector<double>& x,
    const std::vector<double>& y,
    const std::vector<double>& query,
    std::int64_t degree,
    bool robust,
    bool lowess,
    double lowess_frac) {
  if (lowess) {
    if (!robust) {
      return lowess_predict(x, y, query, lowess_frac, nullptr);
    }
    const auto pred_train = lowess_predict(x, y, x, lowess_frac, nullptr);
    std::vector<double> residuals(y.size(), 0.0);
    std::transform(y.begin(), y.end(), pred_train.begin(), residuals.begin(), [](const double a, const double b) {
      return a - b;
    });
    auto robust_w = median_absolute_deviation_weights(residuals);
    return lowess_predict(x, y, query, lowess_frac, &robust_w);
  }
  return fit_poly_predict(x, y, query, degree, robust);
}

[[nodiscard]] inline std::pair<std::vector<double>, std::vector<double>> ecdf_xy(const Series& values) {
  std::vector<double> x = to_f64_vector(values);
  std::sort(x.begin(), x.end());
  std::vector<double> y(x.size(), 0.0);
  const double n = static_cast<double>(x.size());
  for (std::size_t i = 0; i < x.size(); ++i) {
    y[i] = static_cast<double>(i + 1) / n;
  }
  return {std::move(x), std::move(y)};
}

[[nodiscard]] inline std::pair<std::vector<double>, std::vector<double>> gaussian_kde(
    const Series& values,
    std::int64_t grid_points) {
  std::vector<double> samples = to_f64_vector(values);
  if (samples.empty()) {
    return {{}, {}};
  }
  std::sort(samples.begin(), samples.end());
  const std::int64_t count = std::max<std::int64_t>(grid_points, 16);
  const double low = samples.front();
  const double high = samples.back();
  std::vector<double> x = linspace(low, high, count);
  std::vector<double> y(x.size(), 0.0);

  double mean = 0.0;
  double m2 = 0.0;
  std::size_t seen = 0;
  for (const double value : samples) {
    ++seen;
    const double delta = value - mean;
    mean += delta / static_cast<double>(seen);
    const double delta2 = value - mean;
    m2 += delta * delta2;
  }
  const double variance = seen > 0 ? (m2 / static_cast<double>(seen)) : 0.0;
  const double sigma = std::sqrt(std::max(variance, 1e-18));
  const double span = std::max(high - low, 1e-6);
  const double bandwidth =
      sigma > 0.0 ? (1.06 * sigma * std::pow(static_cast<double>(samples.size()), -0.2)) : (span / 24.0);
  const double h = std::max(bandwidth, 1e-6);
  const double inv_norm = 1.0 / (std::sqrt(2.0 * 3.14159265358979323846) * h * static_cast<double>(samples.size()));

  for (std::size_t i = 0; i < x.size(); ++i) {
    const double x_i = x[i];
    double acc = 0.0;
    for (const double sample : samples) {
      const double u = (x_i - sample) / h;
      acc += std::exp(-0.5 * u * u);
    }
    y[i] = acc * inv_norm;
  }

  return {std::move(x), std::move(y)};
}

[[nodiscard]] inline double inv_norm_cdf(double p) {
  const double clamped = std::clamp(p, 1e-12, 1.0 - 1e-12);
  constexpr double a1 = -3.969683028665376e+01;
  constexpr double a2 = 2.209460984245205e+02;
  constexpr double a3 = -2.759285104469687e+02;
  constexpr double a4 = 1.383577518672690e+02;
  constexpr double a5 = -3.066479806614716e+01;
  constexpr double a6 = 2.506628277459239e+00;
  constexpr double b1 = -5.447609879822406e+01;
  constexpr double b2 = 1.615858368580409e+02;
  constexpr double b3 = -1.556989798598866e+02;
  constexpr double b4 = 6.680131188771972e+01;
  constexpr double b5 = -1.328068155288572e+01;
  constexpr double c1 = -7.784894002430293e-03;
  constexpr double c2 = -3.223964580411365e-01;
  constexpr double c3 = -2.400758277161838e+00;
  constexpr double c4 = -2.549732539343734e+00;
  constexpr double c5 = 4.374664141464968e+00;
  constexpr double c6 = 2.938163982698783e+00;
  constexpr double d1 = 7.784695709041462e-03;
  constexpr double d2 = 3.224671290700398e-01;
  constexpr double d3 = 2.445134137142996e+00;
  constexpr double d4 = 3.754408661907416e+00;
  constexpr double plow = 0.02425;
  constexpr double phigh = 1.0 - plow;
  if (clamped < plow) {
    const double q = std::sqrt(-2.0 * std::log(clamped));
    return (((((c1 * q + c2) * q + c3) * q + c4) * q + c5) * q + c6)
           / ((((d1 * q + d2) * q + d3) * q + d4) * q + 1.0);
  }
  if (clamped > phigh) {
    const double q = std::sqrt(-2.0 * std::log(1.0 - clamped));
    return -(((((c1 * q + c2) * q + c3) * q + c4) * q + c5) * q + c6)
           / ((((d1 * q + d2) * q + d3) * q + d4) * q + 1.0);
  }
  const double q = clamped - 0.5;
  const double r = q * q;
  return (((((a1 * r + a2) * r + a3) * r + a4) * r + a5) * r + a6) * q
         / (((((b1 * r + b2) * r + b3) * r + b4) * r + b5) * r + 1.0);
}

[[nodiscard]] inline Rgba mix(Rgba a, Rgba b, double t) noexcept {
  const double s = std::clamp(t, 0.0, 1.0);
  return Rgba{
      (1.0 - s) * a.red + s * b.red,
      (1.0 - s) * a.green + s * b.green,
      (1.0 - s) * a.blue + s * b.blue,
      (1.0 - s) * a.alpha + s * b.alpha,
  };
}

[[nodiscard]] inline Rgba hsl_to_rgb(double h, double s, double l) noexcept {
  const auto hue_to_rgb = [](double p, double q, double t) {
    if (t < 0.0) t += 1.0;
    if (t > 1.0) t -= 1.0;
    if (t < 1.0 / 6.0) return p + (q - p) * 6.0 * t;
    if (t < 1.0 / 2.0) return q;
    if (t < 2.0 / 3.0) return p + (q - p) * (2.0 / 3.0 - t) * 6.0;
    return p;
  };

  const double hh = std::fmod(std::max(h, 0.0), 1.0);
  if (s <= 0.0) {
    return Rgba{l, l, l, 1.0};
  }
  const double q = l < 0.5 ? (l * (1.0 + s)) : (l + s - l * s);
  const double p = 2.0 * l - q;
  return Rgba{
      hue_to_rgb(p, q, hh + 1.0 / 3.0),
      hue_to_rgb(p, q, hh),
      hue_to_rgb(p, q, hh - 1.0 / 3.0),
      1.0,
  };
}

inline void apply_tick_font_scale(Pane& pane, double tick_scale, double axis_scale) {
  pane.tick_font_scale(tick_scale).axis_label_font_scale(axis_scale);
}

[[nodiscard]] inline double scalar_value(const Series& value) {
  const Series scalar = value.to(ScalarType::Float64).reshape({value.numel()});
  if (scalar.numel() == 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return scalar.item<double>();
}

[[nodiscard]] inline double z_for_confidence_level(double level) {
  if (level >= 0.995) {
    return 2.807;
  }
  if (level >= 0.99) {
    return 2.576;
  }
  if (level >= 0.975) {
    return 2.241;
  }
  if (level >= 0.95) {
    return 1.96;
  }
  if (level >= 0.90) {
    return 1.645;
  }
  return 1.282;
}

[[nodiscard]] inline Series normalize_unit(const Series& values) {
  const Series flat = flatten_f64(values);
  if (flat.numel() == 0) {
    return flat;
  }
  const double low = scalar_value(flat.min());
  const double high = scalar_value(flat.max());
  if (!std::isfinite(low) || !std::isfinite(high) || !(high > low)) {
    return pluss::full({flat.numel()}, 0.0, ScalarType::Float64);
  }
  return flat.sub(low).div(high - low);
}

[[nodiscard]] inline std::int64_t require_equal_lengths(
    const std::vector<Series>& columns,
    std::string_view where) {
  if (columns.empty()) {
    return 0;
  }
  const std::int64_t nrows = columns.front().numel();
  for (const auto& column : columns) {
    if (column.numel() != nrows) {
      throw std::invalid_argument(std::string(where) + " requires equal-length columns");
    }
  }
  return nrows;
}

[[nodiscard]] inline std::vector<Series> prepare_numeric_columns(
    const std::vector<Series>& columns,
    bool normalize) {
  std::vector<Series> prepared;
  prepared.reserve(columns.size());
  std::transform(columns.begin(), columns.end(), std::back_inserter(prepared), [&](const Series& column) {
    return normalize ? normalize_unit(column) : flatten_f64(column);
  });
  return prepared;
}

inline void downsample_columns(
    std::vector<Series>& columns,
    std::int64_t max_points,
    std::vector<std::string>* labels = nullptr) {
  if (columns.empty() || max_points <= 0) {
    return;
  }
  const std::int64_t nrows = columns.front().numel();
  if (nrows <= max_points) {
    return;
  }
  const auto step = std::max<std::int64_t>(1, (nrows + max_points - 1) / max_points);
  const auto idx = Series::arange(0, nrows, step, ScalarType::Int64);
  if (labels != nullptr) {
    const auto sampled_idx = idx.to_vec<std::int64_t>();
    std::vector<std::string> sampled_labels;
    sampled_labels.reserve(sampled_idx.size());
    std::transform(
        sampled_idx.begin(),
        sampled_idx.end(),
        std::back_inserter(sampled_labels),
        [&](const std::int64_t row_index) { return (*labels)[static_cast<std::size_t>(row_index)]; });
    *labels = std::move(sampled_labels);
  }
  for (auto& column : columns) {
    column = column.take(idx);
  }
}

[[nodiscard]] inline std::vector<std::string> default_feature_labels(
    const std::vector<std::string>& labels,
    std::int64_t ncols) {
  if (labels.size() == static_cast<std::size_t>(ncols)) {
    return labels;
  }
  std::vector<std::string> out;
  out.reserve(static_cast<std::size_t>(ncols));
  for (const auto i : std::views::iota(std::int64_t{0}, ncols)) {
    out.push_back("x" + std::to_string(i + 1));
  }
  return out;
}

[[nodiscard]] inline Series matrix_from_columns(const std::vector<Series>& columns, std::int64_t rows) {
  if (columns.empty()) {
    return {};
  }
  std::vector<Series> flat;
  flat.reserve(columns.size());
  std::transform(columns.begin(), columns.end(), std::back_inserter(flat), [&](const Series& column) {
    return flatten_f64(column).reshape({rows});
  });
  auto linear = flat.front();
  if (flat.size() > 1) {
    std::vector<Series> tail;
    tail.reserve(flat.size() - 1);
    std::copy(flat.begin() + 1, flat.end(), std::back_inserter(tail));
    linear = linear.unite(tail, 0);
  }
  return linear
      .reshape({static_cast<std::int64_t>(flat.size()), rows})
      .transpose(0, 1)
      .contiguous();
}

[[nodiscard]] inline std::vector<double> row_major_values(
    const std::vector<Series>& columns,
    std::int64_t rows) {
  return matrix_from_columns(columns, rows).to(ScalarType::Float64).to_vec<double>();
}

[[nodiscard]] inline Series andrews_basis(const std::vector<double>& t_values, std::int64_t ncols) {
  const auto points = static_cast<std::int64_t>(t_values.size());
  std::vector<double> basis(static_cast<std::size_t>(std::max<std::int64_t>(0, ncols * points)), 0.0);
  if (ncols <= 0 || points <= 0) {
    return pluss::make_series(basis, {ncols, points});
  }
  const double inv_sqrt2 = 1.0 / std::sqrt(2.0);
  for (const auto col : std::views::iota(std::int64_t{0}, ncols)) {
    auto* row_begin = basis.data() + static_cast<std::size_t>(col * points);
    if (col == 0) {
      std::fill(row_begin, row_begin + points, inv_sqrt2);
      continue;
    }
    const auto harmonic = static_cast<std::int64_t>((col + 1) / 2);
    const bool use_sin = (col % 2) == 1;
    std::transform(
        t_values.begin(),
        t_values.end(),
        row_begin,
        [&](const double t) {
          const double arg = static_cast<double>(harmonic) * t;
          return use_sin ? std::sin(arg) : std::cos(arg);
        });
  }
  return pluss::make_series(basis, {ncols, points});
}

[[nodiscard]] inline double pearson_corr(const Series& lhs, const Series& rhs) {
  const Series x = flatten_f64(lhs);
  const Series y = flatten_f64(rhs);
  if (x.numel() != y.numel() || x.numel() < 2) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  const double x_mean = scalar_value(x.mean());
  const double y_mean = scalar_value(y.mean());
  const Series x_centered = x.sub(x_mean);
  const Series y_centered = y.sub(y_mean);
  const double numerator = scalar_value(x_centered.mul(y_centered).sum());
  const double denom_x = scalar_value(x_centered.mul(x_centered).sum());
  const double denom_y = scalar_value(y_centered.mul(y_centered).sum());
  const double denom = std::sqrt(std::max(0.0, denom_x) * std::max(0.0, denom_y));
  if (!(denom > 0.0) || !std::isfinite(denom)) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return numerator / denom;
}

[[nodiscard]] inline double quantile_sorted(const std::vector<double>& sorted, double q) {
  if (sorted.empty()) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  if (sorted.size() == 1) {
    return sorted.front();
  }
  const double clamped = std::clamp(q, 0.0, 1.0);
  const double position = clamped * static_cast<double>(sorted.size() - 1);
  const auto lo = static_cast<std::size_t>(std::floor(position));
  const auto hi = static_cast<std::size_t>(std::ceil(position));
  if (lo == hi) {
    return sorted[lo];
  }
  const double weight = position - static_cast<double>(lo);
  return sorted[lo] * (1.0 - weight) + sorted[hi] * weight;
}

struct FieldmapGrid {
  std::vector<double> x{};
  std::vector<double> y{};
  Series z{};
};

[[nodiscard]] inline Series apply_heatmap_limits_and_mask(
    const Series& z_values,
    const std::optional<double>& vmin,
    const std::optional<double>& vmax,
    const std::optional<Series>& mask) {
  Series out = z_values.to(ScalarType::Float64);
  if (vmin.has_value() || vmax.has_value()) {
    const double lo = vmin.value_or(-std::numeric_limits<double>::infinity());
    const double hi = vmax.value_or(std::numeric_limits<double>::infinity());
    if (std::isfinite(lo) && std::isfinite(hi) && lo <= hi) {
      out = out.clamp(lo, hi);
    }
  }
  if (!mask.has_value()) {
    return out;
  }

  const Series flat_mask = flatten_f64(*mask).reshape({out.numel()});
  if (flat_mask.numel() != out.numel()) {
    throw std::invalid_argument("flash::heatmap mask must match z element count");
  }
  const Series valid = flat_mask.ne(0.0).reshape(out.sizes());
  const Series fill =
      Series::full({out.numel()}, std::numeric_limits<double>::quiet_NaN(), ScalarType::Float64).reshape(out.sizes());
  return out.where(valid, fill);
}

[[nodiscard]] inline FieldmapGrid hist2d_grid(
    const Series& x_values,
    const Series& y_values,
    std::int64_t bins_x,
    std::int64_t bins_y) {
  const Series sx = flatten_f64(x_values);
  const Series sy = flatten_f64(y_values);
  if (sx.numel() != sy.numel() || sx.numel() <= 0) {
    throw std::invalid_argument("flash::hist2d_grid requires equal non-empty x/y lengths");
  }

  const std::int64_t bx = std::max<std::int64_t>(4, bins_x);
  const std::int64_t by = std::max<std::int64_t>(4, bins_y);
  const std::vector<double> vx = to_f64_vector(sx);
  const std::vector<double> vy = to_f64_vector(sy);
  if (vx.size() != vy.size()) {
    throw std::invalid_argument("flash::hist2d_grid x/y size mismatch");
  }

  double x_min = std::numeric_limits<double>::infinity();
  double x_max = -std::numeric_limits<double>::infinity();
  double y_min = std::numeric_limits<double>::infinity();
  double y_max = -std::numeric_limits<double>::infinity();
  std::size_t valid_points = 0;
  for (const auto i : std::views::iota(std::size_t{0}, vx.size())) {
    const double x = vx[i];
    const double y = vy[i];
    if (!std::isfinite(x) || !std::isfinite(y)) {
      continue;
    }
    x_min = std::min(x_min, x);
    x_max = std::max(x_max, x);
    y_min = std::min(y_min, y);
    y_max = std::max(y_max, y);
    ++valid_points;
  }
  if (valid_points == 0) {
    throw std::invalid_argument("flash::hist2d_grid has no finite x/y pairs");
  }
  const double x_span = (x_max > x_min) ? (x_max - x_min) : 1.0;
  const double y_span = (y_max > y_min) ? (y_max - y_min) : 1.0;

  std::vector<double> counts(static_cast<std::size_t>(bx * by), 0.0);
  for (const auto i : std::views::iota(std::size_t{0}, vx.size())) {
    const double x = vx[i];
    const double y = vy[i];
    if (!std::isfinite(x) || !std::isfinite(y)) {
      continue;
    }
    const auto ix = static_cast<std::int64_t>(std::floor((x - x_min) / x_span * static_cast<double>(bx)));
    const auto iy = static_cast<std::int64_t>(std::floor((y - y_min) / y_span * static_cast<double>(by)));
    const auto cx = std::clamp<std::int64_t>(ix, 0, bx - 1);
    const auto cy = std::clamp<std::int64_t>(iy, 0, by - 1);
    counts[static_cast<std::size_t>(cy * bx + cx)] += 1.0;
  }

  FieldmapGrid out;
  out.x = linspace(x_min, x_max, bx);
  out.y = linspace(y_min, y_max, by);
  out.z = pluss::make_series(std::move(counts)).reshape({by, bx});
  return out;
}

[[nodiscard]] inline Series smooth_grid(const Series& grid, std::int64_t passes) {
  Series out = grid.to(ScalarType::Float64);
  if (out.dim() != 2 || out.numel() == 0 || passes <= 0) {
    return out;
  }
  const auto rows = out.sizes()[0];
  const auto cols = out.sizes()[1];
  if (rows <= 2 || cols <= 2) {
    return out;
  }

  std::vector<double> curr = out.to_vec<double>();
  std::vector<double> next(curr.size(), 0.0);
  constexpr std::array<double, 9> k{
      1.0, 2.0, 1.0,
      2.0, 4.0, 2.0,
      1.0, 2.0, 1.0,
  };
  constexpr std::array<std::int64_t, 9> dr{-1, -1, -1, 0, 0, 0, 1, 1, 1};
  constexpr std::array<std::int64_t, 9> dc{-1, 0, 1, -1, 0, 1, -1, 0, 1};

  const auto total = static_cast<std::size_t>(rows * cols);
  for (const auto _ : std::views::iota(std::int64_t{0}, passes)) {
    for (const auto idx : std::views::iota(std::size_t{0}, total)) {
      const auto r = static_cast<std::int64_t>(idx / static_cast<std::size_t>(cols));
      const auto c = static_cast<std::int64_t>(idx % static_cast<std::size_t>(cols));
      double weighted = 0.0;
      double weight_sum = 0.0;
      for (const auto kidx : std::views::iota(std::size_t{0}, k.size())) {
        const auto rr = std::clamp<std::int64_t>(r + dr[kidx], 0, rows - 1);
        const auto cc = std::clamp<std::int64_t>(c + dc[kidx], 0, cols - 1);
        const double value = curr[static_cast<std::size_t>(rr * cols + cc)];
        if (!std::isfinite(value)) {
          continue;
        }
        weighted += value * k[kidx];
        weight_sum += k[kidx];
      }
      next[idx] = (weight_sum > 0.0) ? (weighted / weight_sum) : curr[idx];
    }
    curr.swap(next);
  }
  return pluss::make_series(std::move(curr)).reshape({rows, cols});
}

[[nodiscard]] inline std::vector<std::size_t> argsort_indices(const std::vector<double>& values) {
  std::vector<std::size_t> order(values.size(), 0);
  std::iota(order.begin(), order.end(), std::size_t{0});
  std::stable_sort(order.begin(), order.end(), [&](const std::size_t a, const std::size_t b) {
    return values[a] < values[b];
  });
  return order;
}

[[nodiscard]] inline FieldmapGrid cluster_grid_by_mean(
    const Series& x_values,
    const Series& y_values,
    const Series& z_values,
    bool cluster_rows,
    bool cluster_cols) {
  const Series sx = flatten_f64(x_values);
  const Series sy = flatten_f64(y_values);
  const Series sz = z_values.to(ScalarType::Float64);
  if (sz.dim() != 2) {
    throw std::invalid_argument("flash::clustermap requires rank-2 z");
  }

  const auto rows = sz.sizes()[0];
  const auto cols = sz.sizes()[1];
  if (sx.numel() != cols || sy.numel() != rows) {
    throw std::invalid_argument("flash::clustermap x/y extents must match z cols/rows");
  }

  const std::vector<double> x = to_f64_vector(sx);
  const std::vector<double> y = to_f64_vector(sy);
  const std::vector<double> flat = sz.to_vec<double>();

  std::vector<double> row_sum(static_cast<std::size_t>(rows), 0.0);
  std::vector<double> row_cnt(static_cast<std::size_t>(rows), 0.0);
  std::vector<double> col_sum(static_cast<std::size_t>(cols), 0.0);
  std::vector<double> col_cnt(static_cast<std::size_t>(cols), 0.0);
  for (const auto idx : std::views::iota(std::size_t{0}, flat.size())) {
    const double value = flat[idx];
    if (!std::isfinite(value)) {
      continue;
    }
    const auto r = idx / static_cast<std::size_t>(cols);
    const auto c = idx % static_cast<std::size_t>(cols);
    row_sum[r] += value;
    row_cnt[r] += 1.0;
    col_sum[c] += value;
    col_cnt[c] += 1.0;
  }

  std::vector<double> row_mean(row_sum.size(), 0.0);
  std::vector<double> col_mean(col_sum.size(), 0.0);
  std::transform(
      row_sum.begin(),
      row_sum.end(),
      row_cnt.begin(),
      row_mean.begin(),
      [](const double sum, const double cnt) { return (cnt > 0.0) ? (sum / cnt) : std::numeric_limits<double>::infinity(); });
  std::transform(
      col_sum.begin(),
      col_sum.end(),
      col_cnt.begin(),
      col_mean.begin(),
      [](const double sum, const double cnt) { return (cnt > 0.0) ? (sum / cnt) : std::numeric_limits<double>::infinity(); });

  const std::vector<std::size_t> row_order =
      cluster_rows ? argsort_indices(row_mean) : argsort_indices(std::vector<double>(row_mean.size(), 0.0));
  const std::vector<std::size_t> col_order =
      cluster_cols ? argsort_indices(col_mean) : argsort_indices(std::vector<double>(col_mean.size(), 0.0));

  std::vector<double> x_sorted(static_cast<std::size_t>(cols), 0.0);
  std::vector<double> y_sorted(static_cast<std::size_t>(rows), 0.0);
  std::transform(col_order.begin(), col_order.end(), x_sorted.begin(), [&](const std::size_t idx) { return x[idx]; });
  std::transform(row_order.begin(), row_order.end(), y_sorted.begin(), [&](const std::size_t idx) { return y[idx]; });

  std::vector<double> z_sorted(flat.size(), 0.0);
  for (const auto out_idx : std::views::iota(std::size_t{0}, z_sorted.size())) {
    const auto out_r = out_idx / static_cast<std::size_t>(cols);
    const auto out_c = out_idx % static_cast<std::size_t>(cols);
    const auto src_r = row_order[out_r];
    const auto src_c = col_order[out_c];
    z_sorted[out_idx] = flat[src_r * static_cast<std::size_t>(cols) + src_c];
  }

  FieldmapGrid out;
  out.x = std::move(x_sorted);
  out.y = std::move(y_sorted);
  out.z = pluss::make_series(std::move(z_sorted)).reshape({rows, cols});
  return out;
}

} // namespace detail

struct Dataset {
  std::optional<Note> numeric_names{};
  std::vector<Series> numeric_values{};
  std::optional<Note> note_names{};
  std::vector<Note> note_values{};

  [[nodiscard]] std::optional<Series> numeric(std::string_view name) const {
    if (!numeric_names.has_value()) {
      return std::nullopt;
    }
    const auto n = std::min<std::int64_t>(numeric_names->numel(), static_cast<std::int64_t>(numeric_values.size()));
    for (std::int64_t i = 0; i < n; ++i) {
      if (numeric_names->at(i) == name) {
        return numeric_values[static_cast<std::size_t>(i)];
      }
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<Note> note(std::string_view name) const {
    if (!note_names.has_value()) {
      return std::nullopt;
    }
    const auto n = std::min<std::int64_t>(note_names->numel(), static_cast<std::int64_t>(note_values.size()));
    for (std::int64_t i = 0; i < n; ++i) {
      if (note_names->at(i) == name) {
        return note_values[static_cast<std::size_t>(i)];
      }
    }
    return std::nullopt;
  }
};

#include "pluss/pl/flash/detail/flash_style.h"
#include "pluss/pl/flash/detail/scene3d.h"

template <class X, class Y, class Z>
Trace& path3(Pane& pane, X&& x, Y&& y, Z&& z) {
  return pane.path3(detail::to_series(std::forward<X>(x)),
                    detail::to_series(std::forward<Y>(y)),
                    detail::to_series(std::forward<Z>(z)));
}

template <class X, class Y, class Z>
Trace& path3(Studio& studio, X&& x, Y&& y, Z&& z, std::string title = {}) {
  Pane& pane = studio.pane3(std::move(title));
  return path3(pane, std::forward<X>(x), std::forward<Y>(y), std::forward<Z>(z));
}

template <class X, class Y, class Z>
Trace& cloud3(Pane& pane, X&& x, Y&& y, Z&& z) {
  return pane.cloud3(detail::to_series(std::forward<X>(x)),
                     detail::to_series(std::forward<Y>(y)),
                     detail::to_series(std::forward<Z>(z)));
}

template <class X, class Y, class Z, class Size>
Trace& cloud3(Pane& pane, X&& x, Y&& y, Z&& z, Size&& size) {
  return pane.cloud3(detail::to_series(std::forward<X>(x)),
                     detail::to_series(std::forward<Y>(y)),
                     detail::to_series(std::forward<Z>(z)),
                     detail::to_series(std::forward<Size>(size)));
}

template <class X, class Y, class Z, class Size, class Color>
Trace& cloud3(Pane& pane, X&& x, Y&& y, Z&& z, Size&& size, Color&& color) {
  return pane.cloud3(detail::to_series(std::forward<X>(x)),
                     detail::to_series(std::forward<Y>(y)),
                     detail::to_series(std::forward<Z>(z)),
                     detail::to_series(std::forward<Size>(size)),
                     detail::to_series(std::forward<Color>(color)));
}

template <class X, class Y, class Z>
Trace& cloud3(Studio& studio, X&& x, Y&& y, Z&& z, std::string title = {}) {
  Pane& pane = studio.pane3(std::move(title));
  return cloud3(pane, std::forward<X>(x), std::forward<Y>(y), std::forward<Z>(z));
}

template <class X, class Y, class Z>
Trace& sheet(Pane& pane, X&& x, Y&& y, Z&& z) {
  return pane.sheet(detail::to_series(std::forward<X>(x)),
                    detail::to_series(std::forward<Y>(y)),
                    detail::to_series(std::forward<Z>(z)));
}

template <class X, class Y, class Z, class C>
Trace& sheet(Pane& pane, X&& x, Y&& y, Z&& z, C&& c) {
  return pane.sheet(detail::to_series(std::forward<X>(x)),
                    detail::to_series(std::forward<Y>(y)),
                    detail::to_series(std::forward<Z>(z)),
                    detail::to_series(std::forward<C>(c)));
}

template <class X, class Y, class Z>
Trace& sheet(Studio& studio, X&& x, Y&& y, Z&& z, std::string title = {}) {
  Pane& pane = studio.pane3(std::move(title));
  return sheet(pane, std::forward<X>(x), std::forward<Y>(y), std::forward<Z>(z));
}

template <class X, class Y, class Z>
Trace& lattice(Pane& pane, X&& x, Y&& y, Z&& z) {
  return pane.lattice(detail::to_series(std::forward<X>(x)),
                      detail::to_series(std::forward<Y>(y)),
                      detail::to_series(std::forward<Z>(z)));
}

template <class X, class Y, class Z, class C>
Trace& lattice(Pane& pane, X&& x, Y&& y, Z&& z, C&& c) {
  return pane.lattice(detail::to_series(std::forward<X>(x)),
                      detail::to_series(std::forward<Y>(y)),
                      detail::to_series(std::forward<Z>(z)),
                      detail::to_series(std::forward<C>(c)));
}

template <class X, class Y, class Z>
Trace& lattice(Studio& studio, X&& x, Y&& y, Z&& z, std::string title = {}) {
  Pane& pane = studio.pane3(std::move(title));
  return lattice(pane, std::forward<X>(x), std::forward<Y>(y), std::forward<Z>(z));
}

template <class Z>
Trace& spire3(Pane& pane, Z&& z) {
  return pane.spire3(detail::to_series(std::forward<Z>(z)));
}

template <class Y, class Z>
Trace& spire3(Pane& pane, Y&& y, Z&& z) {
  return pane.spire3(detail::to_series(std::forward<Y>(y)),
                     detail::to_series(std::forward<Z>(z)));
}

template <class X, class Y, class Z>
Trace& spire3(Pane& pane, X&& x, Y&& y, Z&& z) {
  return pane.spire3(detail::to_series(std::forward<X>(x)),
                     detail::to_series(std::forward<Y>(y)),
                     detail::to_series(std::forward<Z>(z)));
}

template <class X, class Y, class Z, class C>
Trace& spire3(Pane& pane, X&& x, Y&& y, Z&& z, C&& c) {
  return pane.spire3(detail::to_series(std::forward<X>(x)),
                     detail::to_series(std::forward<Y>(y)),
                     detail::to_series(std::forward<Z>(z)),
                     detail::to_series(std::forward<C>(c)));
}

template <class X, class Y, class Z>
Trace& ridge3(Pane& pane, X&& x, Y&& y, Z&& z) {
  return pane.ridge3(detail::to_series(std::forward<X>(x)),
                     detail::to_series(std::forward<Y>(y)),
                     detail::to_series(std::forward<Z>(z)));
}

template <class X, class Y, class Z, class Levels>
  requires(!std::integral<std::remove_cvref_t<Levels>>)
Trace& ridge3(Pane& pane, X&& x, Y&& y, Z&& z, Levels&& levels) {
  return pane.ridge3(detail::to_series(std::forward<X>(x)),
                     detail::to_series(std::forward<Y>(y)),
                     detail::to_series(std::forward<Z>(z)),
                     detail::to_series(std::forward<Levels>(levels)));
}

template <class X, class Y, class Z>
Trace& ridge3(Pane& pane, X&& x, Y&& y, Z&& z, int level_count) {
  return pane.ridge3(detail::to_series(std::forward<X>(x)),
                     detail::to_series(std::forward<Y>(y)),
                     detail::to_series(std::forward<Z>(z)),
                     level_count);
}

template <class Values>
Trace& isoshell(Pane& pane, Values&& values, int level_count = 4) {
  return pane.isoshell(detail::to_series(std::forward<Values>(values)), level_count);
}

template <class X, class Y, class Z, class Values>
Trace& isoshell(Pane& pane, X&& x, Y&& y, Z&& z, Values&& values, int level_count = 4) {
  return pane.isoshell(detail::to_series(std::forward<X>(x)),
                       detail::to_series(std::forward<Y>(y)),
                       detail::to_series(std::forward<Z>(z)),
                       detail::to_series(std::forward<Values>(values)),
                       level_count);
}

template <class X, class Y, class Z, class Values, class Levels>
  requires(!std::integral<std::remove_cvref_t<Levels>>)
Trace& isoshell(Pane& pane, X&& x, Y&& y, Z&& z, Values&& values, Levels&& levels) {
  return pane.isoshell(detail::to_series(std::forward<X>(x)),
                       detail::to_series(std::forward<Y>(y)),
                       detail::to_series(std::forward<Z>(z)),
                       detail::to_series(std::forward<Values>(values)),
                       detail::to_series(std::forward<Levels>(levels)));
}

template <class X, class Y, class Z>
Trace& plot3D(Pane& pane, X&& x, Y&& y, Z&& z) {
  return path3(pane, std::forward<X>(x), std::forward<Y>(y), std::forward<Z>(z));
}

template <class X, class Y, class Z>
Trace& scatter3D(Pane& pane, X&& x, Y&& y, Z&& z) {
  return cloud3(pane, std::forward<X>(x), std::forward<Y>(y), std::forward<Z>(z));
}

template <class X, class Y, class Z>
Trace& plot_surface(Pane& pane, X&& x, Y&& y, Z&& z) {
  return sheet(pane, std::forward<X>(x), std::forward<Y>(y), std::forward<Z>(z));
}

template <class X, class Y, class Z>
Trace& plot_wireframe(Pane& pane, X&& x, Y&& y, Z&& z) {
  return lattice(pane, std::forward<X>(x), std::forward<Y>(y), std::forward<Z>(z));
}

template <class X, class Y, class Z>
Trace& contour3D(Pane& pane, X&& x, Y&& y, Z&& z, int level_count = 8) {
  return ridge3(pane, std::forward<X>(x), std::forward<Y>(y), std::forward<Z>(z), level_count);
}

template <class X, class Y, class Z>
Trace& bar3d(Pane& pane, X&& x, Y&& y, Z&& z) {
  return spire3(pane, std::forward<X>(x), std::forward<Y>(y), std::forward<Z>(z));
}

struct Hist3DOptions {
  std::int64_t bins_x{26};
  std::int64_t bins_y{20};
  bool normalize{false};
  bool smooth{false};
  std::int64_t smooth_passes{1};
  bool as_surface{false};
  bool as_wireframe{false};
};

template <class X, class Y>
Trace& hist3d(Pane& pane, X&& x, Y&& y, const Hist3DOptions& options = {}) {
  auto grid = detail::hist2d_grid(
      detail::to_series(std::forward<X>(x)),
      detail::to_series(std::forward<Y>(y)),
      std::max<std::int64_t>(4, options.bins_x),
      std::max<std::int64_t>(4, options.bins_y));
  if (options.smooth) {
    grid.z = detail::smooth_grid(grid.z, std::max<std::int64_t>(1, options.smooth_passes));
  }
  if (options.normalize) {
    const double total = detail::to_f64_vector(grid.z.sum())[0];
    if (total > 0.0 && std::isfinite(total)) {
      grid.z = grid.z.div(total);
    }
  }
  if (options.as_wireframe) {
    return pane.lattice(grid.x, grid.y, grid.z);
  }
  if (options.as_surface) {
    return pane.sheet(grid.x, grid.y, grid.z);
  }
  return pane.spire3(grid.x, grid.y, grid.z);
}

template <class X, class Y>
Trace& hist3d(Studio& studio, X&& x, Y&& y, const Hist3DOptions& options = {}, std::string title = {}) {
  Pane& pane = studio.pane3(std::move(title));
  return hist3d(pane, std::forward<X>(x), std::forward<Y>(y), options);
}

template <class X, class Y>
Trace& scatterplot(Pane& pane, X&& x, Y&& y) {
  return pane.cloud(detail::to_series(std::forward<X>(x)), detail::to_series(std::forward<Y>(y)));
}

template <class X, class Y>
Trace& scatterplot(Pane& pane, X&& x, Y&& y, const Note& hue) {
  const Series sx = detail::flatten_f64(detail::to_series(std::forward<X>(x)));
  const Series sy = detail::flatten_f64(detail::to_series(std::forward<Y>(y)));
  const auto [encoded, labels] = detail::encode_categories(hue);
  const Series hue_codes = pluss::make_series(encoded, {static_cast<std::int64_t>(encoded.size())});
  detail::require_equal_xy_hue_lengths("flash::scatterplot(..., hue)", sx, sy, hue_codes);
  return detail::draw_grouped_hue(
      pane,
      detail::HueRenderKind::Scatter,
      sx,
      sy,
      hue_codes,
      detail::ordinal_levels(static_cast<std::int64_t>(labels.size()), 1.0),
      labels);
}

template <class X, class Y, class Hue>
  requires(detail::HueSeriesLike<Hue> && !std::same_as<std::remove_cvref_t<Hue>, Note>)
Trace& scatterplot(Pane& pane, X&& x, Y&& y, Hue&& hue) {
  const Series sx = detail::flatten_f64(detail::to_series(std::forward<X>(x)));
  const Series sy = detail::flatten_f64(detail::to_series(std::forward<Y>(y)));
  const Series hue_values = detail::flatten_f64(detail::to_series(std::forward<Hue>(hue)));
  detail::require_equal_xy_hue_lengths("flash::scatterplot(..., hue)", sx, sy, hue_values);

  const Series unique = hue_values.unique(true).reshape({hue_values.unique(true).numel()});
  if (unique.numel() <= 10) {
    const std::vector<double> levels = detail::to_f64_vector(unique);
    std::vector<std::string> labels;
    labels.reserve(levels.size());
    for (const double level : levels) {
      labels.push_back("hue=" + detail::compact_number_label(level));
    }
    return detail::draw_grouped_hue(
        pane,
        detail::HueRenderKind::Scatter,
        sx,
        sy,
        hue_values,
        levels,
        labels);
  }

  constexpr double marker_area = 18.0;
  return pane.cloud(sx, sy, std::array<double, 1>{marker_area}, hue_values).palette(PaletteRamp::lagoon());
}

template <class X, class Y>
Trace& lineplot(Pane& pane, X&& x, Y&& y) {
  return pane.trend(detail::to_series(std::forward<X>(x)), detail::to_series(std::forward<Y>(y)));
}

template <class X, class Y>
Trace& lineplot(Pane& pane, X&& x, Y&& y, const Note& hue) {
  const Series sx = detail::flatten_f64(detail::to_series(std::forward<X>(x)));
  const Series sy = detail::flatten_f64(detail::to_series(std::forward<Y>(y)));
  const auto [encoded, labels] = detail::encode_categories(hue);
  const Series hue_codes = pluss::make_series(encoded, {static_cast<std::int64_t>(encoded.size())});
  detail::require_equal_xy_hue_lengths("flash::lineplot(..., hue)", sx, sy, hue_codes);
  return detail::draw_grouped_hue(
      pane,
      detail::HueRenderKind::Line,
      sx,
      sy,
      hue_codes,
      detail::ordinal_levels(static_cast<std::int64_t>(labels.size()), 1.0),
      labels);
}

template <class X, class Y, class Hue>
  requires(detail::HueSeriesLike<Hue> && !std::same_as<std::remove_cvref_t<Hue>, Note>)
Trace& lineplot(Pane& pane, X&& x, Y&& y, Hue&& hue) {
  const Series sx = detail::flatten_f64(detail::to_series(std::forward<X>(x)));
  const Series sy = detail::flatten_f64(detail::to_series(std::forward<Y>(y)));
  const Series hue_values = detail::flatten_f64(detail::to_series(std::forward<Hue>(hue)));
  detail::require_equal_xy_hue_lengths("flash::lineplot(..., hue)", sx, sy, hue_values);

  const Series unique = hue_values.unique(true).reshape({hue_values.unique(true).numel()});
  if (unique.numel() <= 8) {
    const std::vector<double> levels = detail::to_f64_vector(unique);
    std::vector<std::string> labels;
    labels.reserve(levels.size());
    for (const double level : levels) {
      labels.push_back("hue=" + detail::compact_number_label(level));
    }
    return detail::draw_grouped_hue(
        pane,
        detail::HueRenderKind::Line,
        sx,
        sy,
        hue_values,
        levels,
        labels);
  }

  const auto [bins, bin_labels] = detail::numeric_hue_bins(hue_values, 6);
  return detail::draw_grouped_hue(
      pane,
      detail::HueRenderKind::Line,
      sx,
      sy,
      bins,
      detail::ordinal_levels(static_cast<std::int64_t>(bin_labels.size()), 0.0),
      bin_labels);
}

template <class X, class Y>
Trace& relplot(Pane& pane, X&& x, Y&& y, std::string_view kind = "scatter") {
  if (kind == "line") {
    return lineplot(pane, std::forward<X>(x), std::forward<Y>(y));
  }
  return scatterplot(pane, std::forward<X>(x), std::forward<Y>(y));
}

template <class X, class Y>
Trace& relplot(Pane& pane, X&& x, Y&& y, const Note& hue, std::string_view kind = "scatter") {
  if (kind == "line") {
    return lineplot(pane, std::forward<X>(x), std::forward<Y>(y), hue);
  }
  return scatterplot(pane, std::forward<X>(x), std::forward<Y>(y), hue);
}

template <class X, class Y, class Hue>
  requires(detail::HueSeriesLike<Hue> && !std::same_as<std::remove_cvref_t<Hue>, Note>)
Trace& relplot(Pane& pane, X&& x, Y&& y, Hue&& hue, std::string_view kind = "scatter") {
  if (kind == "line") {
    return lineplot(pane, std::forward<X>(x), std::forward<Y>(y), std::forward<Hue>(hue));
  }
  return scatterplot(pane, std::forward<X>(x), std::forward<Y>(y), std::forward<Hue>(hue));
}

template <class X, class Y>
Trace& relplot(Studio& studio, X&& x, Y&& y, std::string_view kind = "scatter", std::string title = {}) {
  Pane& pane = studio.pane2(std::move(title));
  return relplot(pane, std::forward<X>(x), std::forward<Y>(y), kind);
}

template <class X, class Y>
Trace& relplot(Studio& studio, X&& x, Y&& y, const Note& hue, std::string_view kind = "scatter", std::string title = {}) {
  Pane& pane = studio.pane2(std::move(title));
  return relplot(pane, std::forward<X>(x), std::forward<Y>(y), hue, kind);
}

template <class X, class Y, class Hue>
  requires(detail::HueSeriesLike<Hue> && !std::same_as<std::remove_cvref_t<Hue>, Note>)
Trace& relplot(
    Studio& studio,
    X&& x,
    Y&& y,
    Hue&& hue,
    std::string_view kind = "scatter",
    std::string title = {}) {
  Pane& pane = studio.pane2(std::move(title));
  return relplot(pane, std::forward<X>(x), std::forward<Y>(y), std::forward<Hue>(hue), kind);
}

template <class Values>
Trace& histplot(Pane& pane, Values&& values, int bins = 12) {
  return pane.bucket(detail::to_series(std::forward<Values>(values)), bins);
}

template <class Values>
Trace& displot(Pane& pane, Values&& values, int bins = 12) {
  return histplot(pane, std::forward<Values>(values), bins);
}

template <class X, class Density>
  requires(!std::integral<std::remove_cvref_t<Density>>)
Trace& kdeplot(Pane& pane, X&& x, Density&& density) {
  return lineplot(pane, std::forward<X>(x), std::forward<Density>(density));
}

template <class Values>
Trace& kdeplot(Pane& pane, Values&& values, std::int64_t points = 128) {
  const auto [x, y] = detail::gaussian_kde(detail::to_series(std::forward<Values>(values)), points);
  return lineplot(pane, x, y);
}

template <class Values>
Trace& ecdfplot(Pane& pane, Values&& values) {
  const auto [x, y] = detail::ecdf_xy(detail::to_series(std::forward<Values>(values)));
  return lineplot(pane, x, y);
}

template <class Values>
Trace& rugplot(Pane& pane, Values&& values, double height = 0.08) {
  const Series x = detail::to_series(std::forward<Values>(values));
  const std::vector<double> y(static_cast<std::size_t>(x.numel()), std::max(height, 1e-4));
  return pane.spike(x, pluss::make_series(y, {x.numel()}), 0.0);
}

inline Trace& stripplot(Pane& pane, const Note& categories, const Series& values) {
  const auto [encoded, labels] = detail::encode_categories(categories);
  pane.x_categories(labels);
  return pane.cloud(pluss::make_series(encoded, {static_cast<std::int64_t>(encoded.size())}), values);
}

template <class Group, class Values>
  requires(!std::same_as<std::remove_cvref_t<Group>, Note>)
Trace& stripplot(Pane& pane, Group&& group, Values&& values) {
  return pane.cloud(detail::to_series(std::forward<Group>(group)),
                    detail::to_series(std::forward<Values>(values)));
}

template <class Group, class Values>
Trace& swarmplot(Pane& pane, Group&& group, Values&& values) {
  return stripplot(pane, std::forward<Group>(group), std::forward<Values>(values));
}

template <class Values>
Trace& boxplot(Pane& pane, Values&& values) {
  return pane.quartet(detail::to_series(std::forward<Values>(values)));
}

inline Trace& boxplot(Pane& pane, const Note& categories, const Series& values) {
  const auto [encoded, labels] = detail::encode_categories(categories);
  pane.x_categories(labels);
  return pane.quartet(pluss::make_series(encoded, {static_cast<std::int64_t>(encoded.size())}), values);
}

template <class Group, class Values>
  requires(!std::same_as<std::remove_cvref_t<Group>, Note>)
Trace& boxplot(Pane& pane, Group&& group, Values&& values) {
  return pane.quartet(detail::to_series(std::forward<Group>(group)),
                      detail::to_series(std::forward<Values>(values)));
}

template <class Values>
Trace& violinplot(Pane& pane, Values&& values) {
  return pane.violin(detail::to_series(std::forward<Values>(values)));
}

template <class Values>
Trace& violinplot(Pane& pane, Values&& values, const ViolinOptions& options) {
  return pane.violin(detail::to_series(std::forward<Values>(values)), options);
}

inline Trace& violinplot(Pane& pane, const Note& categories, const Series& values) {
  const auto [encoded, labels] = detail::encode_categories(categories);
  pane.x_categories(labels);
  return pane.violin(pluss::make_series(encoded, {static_cast<std::int64_t>(encoded.size())}), values);
}

inline Trace& violinplot(Pane& pane, const Note& categories, const Series& values, const ViolinOptions& options) {
  const auto [encoded, labels] = detail::encode_categories(categories);
  pane.x_categories(labels);
  return pane.violin(pluss::make_series(encoded, {static_cast<std::int64_t>(encoded.size())}), values, options);
}

template <class Group, class Values>
  requires(!std::same_as<std::remove_cvref_t<Group>, Note>)
Trace& violinplot(Pane& pane, Group&& group, Values&& values) {
  return pane.violin(detail::to_series(std::forward<Group>(group)),
                     detail::to_series(std::forward<Values>(values)));
}

template <class Group, class Values>
  requires(!std::same_as<std::remove_cvref_t<Group>, Note>)
Trace& violinplot(Pane& pane, Group&& group, Values&& values, const ViolinOptions& options) {
  return pane.violin(detail::to_series(std::forward<Group>(group)),
                     detail::to_series(std::forward<Values>(values)),
                     options);
}

template <class Values>
Trace& boxenplot(Pane& pane, Values&& values) {
  return boxplot(pane, std::forward<Values>(values));
}

template <class X, class Y, class Err>
Trace& pointplot(Pane& pane, X&& x, Y&& y, Err&& err) {
  lineplot(pane, x, y);
  return pane.whisk(detail::to_series(std::forward<X>(x)),
                    detail::to_series(std::forward<Y>(y)),
                    detail::to_series(std::forward<Err>(err)));
}

struct RegPlotOptions {
  bool show_scatter{true};
  bool show_fit{true};
  bool show_ci{false};
  double confidence_level{0.95};
  std::int64_t n_boot{0};
  std::int64_t order{1};
  bool robust{false};
  bool lowess{false};
  double lowess_frac{0.35};
  std::int64_t fit_points{96};
  double marker_area{18.0};
  float scatter_trace_width{0.55F};
  float fit_trace_width{1.25F};
  float ci_trace_width{0.35F};
};

struct QqPlotOptions {
  bool standardize{true};
  bool show_reference_line{true};
  double marker_area{14.0};
  float trace_width{0.75F};
  float reference_width{1.0F};
};

struct ResidHistPlotOptions {
  int bins{24};
  float trace_width{0.85F};
  RegPlotOptions reg{};
};

enum class JointPlotKind : std::uint8_t {
  Scatter,
  Reg,
  Hex,
  Kde,
};

enum class JointMarginalKind : std::uint8_t {
  Hist,
  Kde,
};

struct JointPlotOptions {
  JointPlotKind kind{JointPlotKind::Scatter};
  JointMarginalKind marginal{JointMarginalKind::Hist};
  std::int64_t marginal_bins{18};
  std::int64_t marginal_kde_points{128};
  std::int64_t hex_bins_x{28};
  std::int64_t hex_bins_y{22};
  std::int64_t kde_bins_x{30};
  std::int64_t kde_bins_y{24};
  std::int64_t kde_smooth_passes{2};
  std::int64_t kde_levels{10};
  bool kde_fill{true};
  bool show_joint_grid{true};
  bool show_marginal_grid{false};
  double grid_gap_x{8.0};
  double grid_gap_y{8.0};
  RegPlotOptions reg{};
};

struct HeatmapOptions {
  bool annotate_levels{false};
  bool scale_guide{true};
  bool borderless{true};
  std::optional<double> vmin{std::nullopt};
  std::optional<double> vmax{std::nullopt};
  std::optional<Series> mask{std::nullopt};
};

struct ClusterMapOptions {
  bool cluster_rows{true};
  bool cluster_cols{true};
  HeatmapOptions heatmap{};
};

template <class X, class Y>
  requires(!std::same_as<std::remove_cvref_t<X>, Note>)
Trace& barplot(Pane& pane, X&& x, Y&& y) {
  return pane.columns(detail::to_series(std::forward<X>(x)),
                      detail::to_series(std::forward<Y>(y)));
}

inline Trace& barplot(Pane& pane, const Note& categories, const Series& values) {
  const auto [encoded, labels] = detail::encode_categories(categories);
  pane.x_categories(labels);
  return pane.columns(pluss::make_series(encoded, {static_cast<std::int64_t>(encoded.size())}), values);
}

template <class Values>
  requires(!std::same_as<std::remove_cvref_t<Values>, Note>)
Trace& countplot(Pane& pane, Values&& values, int bins = 12) {
  return histplot(pane, std::forward<Values>(values), bins);
}

inline Trace& countplot(Pane& pane, const Note& categories) {
  std::map<std::string, double> counts;
  for (const std::string& label : detail::note_values(categories)) {
    counts[label] += 1.0;
  }
  std::vector<double> y;
  std::vector<std::string> labels;
  y.reserve(counts.size());
  labels.reserve(counts.size());
  for (const auto& [label, count] : counts) {
    labels.push_back(label);
    y.push_back(count);
  }
  pane.x_categories(labels);
  return pane.columns(pluss::make_series(y, {static_cast<std::int64_t>(y.size())}));
}

template <class X, class Y>
Trace& regplot(Pane& pane, X&& x, Y&& y, const RegPlotOptions& options = {}) {
  const auto [sx, sy] = detail::finite_xy(
      detail::to_series(std::forward<X>(x)),
      detail::to_series(std::forward<Y>(y)));
  if (sx.numel() < 2) {
    throw std::invalid_argument("flash::regplot requires at least two finite x/y pairs");
  }
  const std::vector<double> vx = detail::to_f64_vector(sx);
  const std::vector<double> vy = detail::to_f64_vector(sy);

  Trace* scatter_trace = nullptr;
  if (options.show_scatter) {
    auto& scatter = pane.cloud(sx, sy, std::array<double, 1>{std::max(1.0, options.marker_area)});
    scatter.width(options.scatter_trace_width).label("data");
    scatter_trace = &scatter;
  }
  const auto [xmin, xmax] = std::minmax_element(vx.begin(), vx.end());
  const std::int64_t points = std::max<std::int64_t>(2, options.fit_points);
  std::vector<double> fit_x = detail::linspace(*xmin, *xmax, points);
  std::vector<double> fit_y = detail::regression_predict(
      vx,
      vy,
      fit_x,
      std::max<std::int64_t>(1, options.order),
      options.robust,
      options.lowess,
      options.lowess_frac);

  Trace* fit_trace = nullptr;
  if (options.show_fit) {
    auto& trend = pane.trend(fit_x, fit_y);
    trend.width(options.fit_trace_width).label("fit");
    fit_trace = &trend;
  }

  if (options.show_ci && sx.numel() > 2) {
    std::vector<double> y_err(static_cast<std::size_t>(points), 0.0);
    if (options.n_boot > 0) {
      const auto boot_count = std::max<std::int64_t>(32, options.n_boot);
      std::vector<std::vector<double>> boot_preds(static_cast<std::size_t>(points));
      std::for_each(boot_preds.begin(), boot_preds.end(), [&](auto& v) { v.reserve(static_cast<std::size_t>(boot_count)); });
      std::mt19937_64 rng(81371ULL);
      std::uniform_int_distribution<std::size_t> pick(0, vx.size() - 1);
      std::vector<double> bx(vx.size(), 0.0);
      std::vector<double> by(vy.size(), 0.0);
      for (const auto _ : std::views::iota(std::int64_t{0}, boot_count)) {
        for (const auto i : std::views::iota(std::size_t{0}, vx.size())) {
          const auto idx = pick(rng);
          bx[i] = vx[idx];
          by[i] = vy[idx];
        }
        const auto pred = detail::regression_predict(
            bx,
            by,
            fit_x,
            std::max<std::int64_t>(1, options.order),
            options.robust,
            options.lowess,
            options.lowess_frac);
        for (const auto i : std::views::iota(std::size_t{0}, pred.size())) {
          boot_preds[i].push_back(pred[i]);
        }
      }
      const double alpha = 0.5 * (1.0 - std::clamp(options.confidence_level, 0.01, 0.999));
      for (const auto i : std::views::iota(std::size_t{0}, fit_y.size())) {
        auto& samples = boot_preds[i];
        std::sort(samples.begin(), samples.end());
        const double lo = detail::quantile_sorted(samples, alpha);
        const double hi = detail::quantile_sorted(samples, 1.0 - alpha);
        y_err[i] = std::max(std::abs(fit_y[i] - lo), std::abs(hi - fit_y[i]));
      }
    } else {
      const auto train_pred = detail::regression_predict(
          vx,
          vy,
          vx,
          std::max<std::int64_t>(1, options.order),
          options.robust,
          options.lowess,
          options.lowess_frac);
      std::vector<double> residuals(vy.size(), 0.0);
      std::transform(vy.begin(), vy.end(), train_pred.begin(), residuals.begin(), [](const double a, const double b) {
        return a - b;
      });
      double sse = 0.0;
      std::for_each(residuals.begin(), residuals.end(), [&](const double r) { sse += r * r; });
      const double dof = std::max<double>(1.0, static_cast<double>(vy.size()) - static_cast<double>(std::max<std::int64_t>(2, options.order + 1)));
      const double sigma = std::sqrt(std::max(0.0, sse / dof));
      const double z = detail::z_for_confidence_level(options.confidence_level);
      std::fill(y_err.begin(), y_err.end(), z * sigma);
    }
    auto& ci = pane.whisk(fit_x, fit_y, y_err);
    ci.width(options.ci_trace_width).label("ci");
  }

  if (fit_trace != nullptr) {
    return *fit_trace;
  }
  if (scatter_trace != nullptr) {
    return *scatter_trace;
  }
  auto& fallback = pane.trend(fit_x, fit_y);
  fallback.width(options.fit_trace_width).label("fit");
  return fallback;
}

template <class X, class Y>
Trace& lmplot(Studio& studio, X&& x, Y&& y, std::string title = {}) {
  Pane& pane = studio.pane2(std::move(title));
  return regplot(pane, std::forward<X>(x), std::forward<Y>(y), RegPlotOptions{});
}

template <class X, class Y>
Trace& lmplot(Studio& studio, X&& x, Y&& y, const RegPlotOptions& options, std::string title = {}) {
  Pane& pane = studio.pane2(std::move(title));
  return regplot(pane, std::forward<X>(x), std::forward<Y>(y), options);
}

template <class X, class Y>
Trace& residplot(Pane& pane, X&& x, Y&& y, const RegPlotOptions& options) {
  const auto [sx, sy] = detail::finite_xy(
      detail::to_series(std::forward<X>(x)),
      detail::to_series(std::forward<Y>(y)));
  if (sx.numel() < 2) {
    throw std::invalid_argument("flash::residplot requires at least two finite x/y pairs");
  }
  std::vector<double> vx = detail::to_f64_vector(sx);
  std::vector<double> vy = detail::to_f64_vector(sy);
  const auto pred = detail::regression_predict(
      vx,
      vy,
      vx,
      std::max<std::int64_t>(1, options.order),
      options.robust,
      options.lowess,
      options.lowess_frac);
  std::transform(vy.begin(), vy.end(), pred.begin(), vy.begin(), [](const double y_i, const double p_i) {
    return y_i - p_i;
  });
  return pane.cloud(pluss::make_series(vx, {static_cast<std::int64_t>(vx.size())}),
                    pluss::make_series(vy, {static_cast<std::int64_t>(vy.size())}));
}

template <class X, class Y>
Trace& residplot(Pane& pane, X&& x, Y&& y) {
  return residplot(pane, std::forward<X>(x), std::forward<Y>(y), RegPlotOptions{});
}

template <class Values>
Trace& qqplot(Pane& pane, Values&& values, const QqPlotOptions& options = {}) {
  const Series base = detail::flatten_f64(detail::to_series(std::forward<Values>(values)));
  const Series finite = base.masked_select(base.isfinite());
  if (finite.numel() < 2) {
    throw std::invalid_argument("flash::qqplot requires at least two finite values");
  }
  std::vector<double> sample = detail::to_f64_vector(finite);
  std::sort(sample.begin(), sample.end());
  if (options.standardize) {
    const double mean =
        std::accumulate(sample.begin(), sample.end(), 0.0) / static_cast<double>(std::max<std::size_t>(1, sample.size()));
    double variance = 0.0;
    std::for_each(sample.begin(), sample.end(), [&](const double v) {
      const double d = v - mean;
      variance += d * d;
    });
    const double sigma = std::sqrt(variance / static_cast<double>(std::max<std::size_t>(1, sample.size())));
    if (sigma > 1e-12) {
      std::for_each(sample.begin(), sample.end(), [&](double& v) { v = (v - mean) / sigma; });
    }
  }

  std::vector<double> q(sample.size(), 0.0);
  for (const auto i : std::views::iota(std::size_t{0}, sample.size())) {
    const double p = (static_cast<double>(i) + 0.5) / static_cast<double>(sample.size());
    q[i] = detail::inv_norm_cdf(p);
  }

  auto& trace = pane.cloud(q, sample, std::array<double, 1>{std::max(2.0, options.marker_area)});
  trace.width(options.trace_width).label("qq");
  if (options.show_reference_line) {
    const double q25 = detail::inv_norm_cdf(0.25);
    const double q75 = detail::inv_norm_cdf(0.75);
    const double y25 = detail::quantile_sorted(sample, 0.25);
    const double y75 = detail::quantile_sorted(sample, 0.75);
    const double slope = std::abs(q75 - q25) > 1e-12 ? ((y75 - y25) / (q75 - q25)) : 1.0;
    const double intercept = y25 - slope * q25;
    const std::array<double, 2> lx{q.front(), q.back()};
    const std::array<double, 2> ly{intercept + slope * lx[0], intercept + slope * lx[1]};
    pane.trend(lx, ly).width(options.reference_width).label("reference");
  }
  pane.grid(GridStyle::Major).x_label("theoretical quantiles").y_label("sample quantiles");
  return trace;
}

template <class Values>
Trace& qqplot(Studio& studio, Values&& values, const QqPlotOptions& options = {}, std::string title = {}) {
  Pane& pane = studio.pane2(std::move(title));
  return qqplot(pane, std::forward<Values>(values), options);
}

template <class X, class Y>
Trace& resid_histplot(Pane& pane, X&& x, Y&& y, const ResidHistPlotOptions& options = {}) {
  const auto [sx, sy] = detail::finite_xy(
      detail::to_series(std::forward<X>(x)),
      detail::to_series(std::forward<Y>(y)));
  if (sx.numel() < 2) {
    throw std::invalid_argument("flash::resid_histplot requires at least two finite x/y pairs");
  }
  const std::vector<double> vx = detail::to_f64_vector(sx);
  std::vector<double> vy = detail::to_f64_vector(sy);
  const auto pred = detail::regression_predict(
      vx,
      vy,
      vx,
      std::max<std::int64_t>(1, options.reg.order),
      options.reg.robust,
      options.reg.lowess,
      options.reg.lowess_frac);
  std::transform(vy.begin(), vy.end(), pred.begin(), vy.begin(), [](const double y_i, const double p_i) {
    return y_i - p_i;
  });
  auto& hist = pane.bucket(
      pluss::make_series(vy, {static_cast<std::int64_t>(vy.size())}),
      std::max(4, options.bins));
  hist.width(options.trace_width).label("residuals");
  pane.grid(GridStyle::Major).x_label("residual").y_label("count").rule_x(0.0, "zero");
  return hist;
}

template <class X, class Y>
Trace& resid_histplot(
    Studio& studio,
    X&& x,
    Y&& y,
    const ResidHistPlotOptions& options = {},
    std::string title = {}) {
  Pane& pane = studio.pane2(std::move(title));
  return resid_histplot(pane, std::forward<X>(x), std::forward<Y>(y), options);
}

template <class X, class Y, class Z>
Trace& heatmap(Pane& pane, X&& x, Y&& y, Z&& z) {
  return heatmap(pane, std::forward<X>(x), std::forward<Y>(y), std::forward<Z>(z), HeatmapOptions{});
}

template <class X, class Y, class Z>
Trace& heatmap(Pane& pane, X&& x, Y&& y, Z&& z, const HeatmapOptions& options) {
  const Series sx = detail::to_series(std::forward<X>(x));
  const Series sy = detail::to_series(std::forward<Y>(y));
  const Series sz = detail::apply_heatmap_limits_and_mask(
      detail::to_series(std::forward<Z>(z)),
      options.vmin,
      options.vmax,
      options.mask);
  auto& trace = pane.fieldmap(sx, sy, sz);
  trace.annotate_levels(options.annotate_levels);
  pane.scale_guide(options.scale_guide);
  if (options.borderless) {
    pane.grid(GridStyle::None)
        .x_major_grid(false)
        .y_major_grid(false)
        .x_minor_grid(false)
        .y_minor_grid(false)
        .axes_visible(true)
        .x_tick_labels(true)
        .y_tick_labels(true);
  }
  return trace;
}

template <class X, class Y, class Z, class C>
  requires(!std::same_as<std::remove_cvref_t<C>, HeatmapOptions>)
Trace& heatmap(Pane& pane, X&& x, Y&& y, Z&& z, C&& c) {
  return heatmap(
      pane,
      std::forward<X>(x),
      std::forward<Y>(y),
      std::forward<Z>(z),
      std::forward<C>(c),
      HeatmapOptions{});
}

template <class X, class Y, class Z, class C>
  requires(!std::same_as<std::remove_cvref_t<C>, HeatmapOptions>)
Trace& heatmap(Pane& pane, X&& x, Y&& y, Z&& z, C&& c, const HeatmapOptions& options) {
  const Series sx = detail::to_series(std::forward<X>(x));
  const Series sy = detail::to_series(std::forward<Y>(y));
  const Series sz = detail::apply_heatmap_limits_and_mask(
      detail::to_series(std::forward<Z>(z)),
      options.vmin,
      options.vmax,
      options.mask);
  auto& trace = pane.fieldmap(sx, sy, sz, detail::to_series(std::forward<C>(c)));
  trace.annotate_levels(options.annotate_levels);
  pane.scale_guide(options.scale_guide);
  if (options.borderless) {
    pane.grid(GridStyle::None)
        .x_major_grid(false)
        .y_major_grid(false)
        .x_minor_grid(false)
        .y_minor_grid(false)
        .axes_visible(true)
        .x_tick_labels(true)
        .y_tick_labels(true);
  }
  return trace;
}

template <class X, class Y, class Z>
Trace& clustermap(Pane& pane, X&& x, Y&& y, Z&& z) {
  return clustermap(pane, std::forward<X>(x), std::forward<Y>(y), std::forward<Z>(z), ClusterMapOptions{});
}

template <class X, class Y, class Z>
Trace& clustermap(Pane& pane, X&& x, Y&& y, Z&& z, const ClusterMapOptions& options) {
  const auto clustered = detail::cluster_grid_by_mean(
      detail::to_series(std::forward<X>(x)),
      detail::to_series(std::forward<Y>(y)),
      detail::to_series(std::forward<Z>(z)),
      options.cluster_rows,
      options.cluster_cols);
  return heatmap(pane, clustered.x, clustered.y, clustered.z, options.heatmap);
}

enum class CatPlotKind : std::uint8_t {
  Strip,
  Swarm,
  Box,
  Violin,
  Bar,
  Count,
};

enum class CatEstimator : std::uint8_t {
  Mean,
  Median,
};

enum class CatErrorBar : std::uint8_t {
  None,
  CI,
  PI,
  Std,
  SE,
};

struct CatPlotOptions {
  CatPlotKind kind{CatPlotKind::Strip};
  CatEstimator estimator{CatEstimator::Mean};
  CatErrorBar errorbar{CatErrorBar::None};
  double confidence_level{0.95};
  bool dodge{false};
  bool split{false};
  std::vector<std::string> order{};
  std::vector<std::string> hue_order{};
  ViolinOptions violin{};
  float trace_width{0.9F};
};

[[nodiscard]] inline CatPlotKind catplot_kind(std::string_view kind) {
  if (kind == "swarm") return CatPlotKind::Swarm;
  if (kind == "box") return CatPlotKind::Box;
  if (kind == "violin") return CatPlotKind::Violin;
  if (kind == "bar") return CatPlotKind::Bar;
  if (kind == "count") return CatPlotKind::Count;
  return CatPlotKind::Strip;
}

[[nodiscard]] inline std::vector<std::string> ordered_levels(
    const std::vector<std::string>& labels,
    const std::vector<std::string>& requested) {
  std::unordered_set<std::string> present(labels.begin(), labels.end());
  std::unordered_set<std::string> seen;
  std::vector<std::string> out;
  out.reserve(std::max(labels.size(), requested.size()));
  if (requested.empty()) {
    for (const auto& label : labels) {
      if (seen.insert(label).second) {
        out.push_back(label);
      }
    }
    return out;
  }
  for (const auto& label : requested) {
    if (present.contains(label) && seen.insert(label).second) {
      out.push_back(label);
    }
  }
  for (const auto& label : labels) {
    if (seen.insert(label).second) {
      out.push_back(label);
    }
  }
  return out;
}

struct CatBucketStats {
  std::vector<double> values{};
  double sum{0.0};
  double sumsq{0.0};
};

struct CatSummary {
  double value{std::numeric_limits<double>::quiet_NaN()};
  double err_neg{0.0};
  double err_pos{0.0};
  bool has_error{false};
  bool valid{false};
};

[[nodiscard]] inline CatSummary summarize_bucket(
    const CatBucketStats& bucket,
    CatEstimator estimator,
    CatErrorBar errorbar,
    double confidence_level) {
  CatSummary out;
  const auto n = bucket.values.size();
  if (n == 0) {
    return out;
  }

  const double n_d = static_cast<double>(n);
  const double mean = bucket.sum / n_d;
  const double variance =
      (n > 1) ? std::max(0.0, (bucket.sumsq - n_d * mean * mean) / (n_d - 1.0)) : 0.0;
  const double stddev = std::sqrt(variance);

  if (estimator == CatEstimator::Median) {
    std::vector<double> sorted = bucket.values;
    std::sort(sorted.begin(), sorted.end());
    out.value = detail::quantile_sorted(sorted, 0.5);
    if (errorbar == CatErrorBar::PI) {
      const double q = std::clamp((1.0 - confidence_level) * 0.5, 0.0, 0.5);
      const double low = detail::quantile_sorted(sorted, q);
      const double high = detail::quantile_sorted(sorted, 1.0 - q);
      out.err_neg = std::max(0.0, out.value - low);
      out.err_pos = std::max(0.0, high - out.value);
      out.has_error = true;
    }
  } else {
    out.value = mean;
  }

  if (errorbar == CatErrorBar::None) {
    out.valid = true;
    return out;
  }
  if (errorbar == CatErrorBar::PI && estimator == CatEstimator::Median) {
    out.valid = true;
    return out;
  }
  if (errorbar == CatErrorBar::PI) {
    std::vector<double> sorted = bucket.values;
    std::sort(sorted.begin(), sorted.end());
    const double q = std::clamp((1.0 - confidence_level) * 0.5, 0.0, 0.5);
    const double low = detail::quantile_sorted(sorted, q);
    const double high = detail::quantile_sorted(sorted, 1.0 - q);
    out.err_neg = std::max(0.0, out.value - low);
    out.err_pos = std::max(0.0, high - out.value);
    out.has_error = true;
    out.valid = true;
    return out;
  }

  const double se = (n > 0) ? (stddev / std::sqrt(n_d)) : 0.0;
  const double z = detail::z_for_confidence_level(confidence_level);
  double span = 0.0;
  switch (errorbar) {
    case CatErrorBar::Std:
      span = stddev;
      break;
    case CatErrorBar::SE:
      span = se;
      break;
    case CatErrorBar::CI:
      span = z * se;
      break;
    case CatErrorBar::None:
    case CatErrorBar::PI:
      span = 0.0;
      break;
  }
  out.err_neg = std::max(0.0, span);
  out.err_pos = std::max(0.0, span);
  out.has_error = span > 0.0;
  out.valid = true;
  return out;
}

inline Trace& catplot(
    Pane& pane,
    const Note& group,
    const Series& values,
    const Note& hue,
    const CatPlotOptions& options);

inline Trace& catplot(
    Pane& pane,
    const Note& group,
    const Series& values,
    const CatPlotOptions& options = {}) {
  const auto empty_hue = pluss::make_note(std::vector<std::string>{});
  return catplot(pane, group, values, empty_hue, options);
}

inline Trace& catplot(
    Pane& pane,
    const Note& group,
    const Series& values,
    const Note& hue,
    const CatPlotOptions& options = {}) {
  const auto groups = detail::note_values(group);
  const auto vals = detail::to_f64_vector(detail::flatten_f64(values));
  if (groups.size() != vals.size()) {
    throw std::invalid_argument("flash::catplot group/value length mismatch");
  }
  std::vector<std::string> hues;
  if (hue.numel() > 0) {
    hues = detail::note_values(hue);
    if (hues.size() != vals.size()) {
      throw std::invalid_argument("flash::catplot hue length must match group/value");
    }
  }

  const auto group_levels = ordered_levels(groups, options.order);
  if (group_levels.empty()) {
    throw std::invalid_argument("flash::catplot requires at least one category");
  }
  std::unordered_map<std::string, std::size_t> group_idx;
  group_idx.reserve(group_levels.size());
  for (const auto i : std::views::iota(std::size_t{0}, group_levels.size())) {
    group_idx.emplace(group_levels[i], i);
  }

  const bool has_hue = !hues.empty();
  const auto hue_levels = has_hue ? ordered_levels(hues, options.hue_order) : std::vector<std::string>{};
  std::unordered_map<std::string, std::size_t> hue_idx;
  hue_idx.reserve(hue_levels.size());
  for (const auto i : std::views::iota(std::size_t{0}, hue_levels.size())) {
    hue_idx.emplace(hue_levels[i], i);
  }

  const std::size_t ng = group_levels.size();
  const std::size_t nh = has_hue ? hue_levels.size() : std::size_t{1};
  const bool split_violin = options.kind == CatPlotKind::Violin && has_hue && options.split;
  if (split_violin && nh != 2) {
    throw std::invalid_argument("flash::catplot split=true currently requires exactly two hue levels");
  }
  const bool use_dodge = has_hue && options.dodge && !split_violin;
  const bool bar_like = options.kind == CatPlotKind::Bar || options.kind == CatPlotKind::Count;
  const bool needs_finite_values = options.kind != CatPlotKind::Count;

  std::vector<double> group_centers;
  group_centers.reserve(ng);
  const double spacing = use_dodge ? static_cast<double>(nh + 1) : 1.0;
  for (const auto g : std::views::iota(std::size_t{0}, ng)) {
    const double left = 1.0 + static_cast<double>(g) * spacing;
    const double center = use_dodge ? (left + 0.5 * static_cast<double>(nh - 1)) : left;
    group_centers.push_back(center);
  }

  if (bar_like) {
    std::vector<CatBucketStats> buckets(ng * nh);
    for (const auto i : std::views::iota(std::size_t{0}, vals.size())) {
      const auto g_it = group_idx.find(groups[i]);
      if (g_it == group_idx.end()) {
        continue;
      }
      const std::size_t g = g_it->second;
      std::size_t h = 0;
      if (has_hue) {
        const auto h_it = hue_idx.find(hues[i]);
        if (h_it == hue_idx.end()) {
          continue;
        }
        h = h_it->second;
      }
      const std::size_t idx = g * nh + h;
      if (options.kind == CatPlotKind::Count) {
        buckets[idx].values.push_back(1.0);
        buckets[idx].sum += 1.0;
        buckets[idx].sumsq += 1.0;
        continue;
      }
      const double v = vals[i];
      if (!std::isfinite(v)) {
        continue;
      }
      buckets[idx].values.push_back(v);
      buckets[idx].sum += v;
      buckets[idx].sumsq += v * v;
    }

    pane.x_ticks(group_centers, group_levels);
    Trace* last = nullptr;
    for (const auto h : std::views::iota(std::size_t{0}, nh)) {
      std::vector<double> x_vals;
      std::vector<double> y_vals;
      std::vector<double> y_neg;
      std::vector<double> y_pos;
      x_vals.reserve(ng);
      y_vals.reserve(ng);
      y_neg.reserve(ng);
      y_pos.reserve(ng);
      bool any_err = false;
      for (const auto g : std::views::iota(std::size_t{0}, ng)) {
        const auto& bucket = buckets[g * nh + h];
        const double x_center =
            use_dodge ? (1.0 + static_cast<double>(g) * spacing + static_cast<double>(h)) : group_centers[g];
        x_vals.push_back(x_center);
        if (options.kind == CatPlotKind::Count) {
          y_vals.push_back(static_cast<double>(bucket.values.size()));
          y_neg.push_back(0.0);
          y_pos.push_back(0.0);
          continue;
        }
        const auto summary =
            summarize_bucket(bucket, options.estimator, options.errorbar, options.confidence_level);
        y_vals.push_back(summary.valid ? summary.value : std::numeric_limits<double>::quiet_NaN());
        y_neg.push_back(summary.has_error ? summary.err_neg : 0.0);
        y_pos.push_back(summary.has_error ? summary.err_pos : 0.0);
        any_err = any_err || summary.has_error;
      }
      auto& trace = pane.columns(pluss::make_series(x_vals), pluss::make_series(y_vals));
      trace.width(options.trace_width);
      if (has_hue) {
        trace.label(hue_levels[h]);
      }
      if (options.kind != CatPlotKind::Count && options.errorbar != CatErrorBar::None && any_err) {
        pane.whisk(
            pluss::make_series(x_vals),
            pluss::make_series(y_vals),
            pluss::make_series(y_neg),
            pluss::make_series(y_pos));
      }
      last = &trace;
    }
    if (last == nullptr) {
      throw std::invalid_argument("flash::catplot produced no bar/count traces");
    }
    return *last;
  }

  std::vector<std::vector<double>> per_hue_x(nh);
  std::vector<std::vector<double>> per_hue_y(nh);
  for (const auto i : std::views::iota(std::size_t{0}, vals.size())) {
    const auto g_it = group_idx.find(groups[i]);
    if (g_it == group_idx.end()) {
      continue;
    }
    const double v = vals[i];
    if (needs_finite_values && !std::isfinite(v)) {
      continue;
    }
    std::size_t h = 0;
    if (has_hue) {
      const auto h_it = hue_idx.find(hues[i]);
      if (h_it == hue_idx.end()) {
        continue;
      }
      h = h_it->second;
    }
    const double x_center =
        use_dodge ? (1.0 + static_cast<double>(g_it->second) * spacing + static_cast<double>(h))
                  : group_centers[g_it->second];
    per_hue_x[h].push_back(x_center);
    per_hue_y[h].push_back(v);
  }

  pane.x_ticks(group_centers, group_levels);
  Trace* last = nullptr;
  for (const auto h : std::views::iota(std::size_t{0}, nh)) {
    if (per_hue_x[h].empty()) {
      continue;
    }
    const Series sx = pluss::make_series(per_hue_x[h]);
    const Series sy = pluss::make_series(per_hue_y[h]);
    Trace* trace = nullptr;
    switch (options.kind) {
      case CatPlotKind::Swarm:
      case CatPlotKind::Strip: {
        trace = &pane.cloud(sx, sy);
        break;
      }
      case CatPlotKind::Box: {
        trace = &pane.quartet(sx, sy);
        break;
      }
      case CatPlotKind::Violin: {
        auto violin_options = options.violin;
        if (split_violin) {
          violin_options.side = h == 0 ? ViolinSide::Left : ViolinSide::Right;
        }
        trace = &pane.violin(sx, sy, violin_options);
        break;
      }
      case CatPlotKind::Bar:
      case CatPlotKind::Count:
      default: {
        trace = &pane.cloud(sx, sy);
        break;
      }
    }
    auto& out = *trace;
    out.width(options.trace_width);
    if (has_hue) {
      out.label(hue_levels[h]);
    }
    last = &out;
  }
  if (last == nullptr) {
    throw std::invalid_argument("flash::catplot produced no traces");
  }
  return *last;
}

inline Trace& catplot(Pane& pane, const Note& group, const Series& values, std::string_view kind = "strip") {
  CatPlotOptions options;
  options.kind = catplot_kind(kind);
  return catplot(pane, group, values, options);
}

inline Trace& catplot(
    Pane& pane,
    const Note& group,
    const Series& values,
    const Note& hue,
    std::string_view kind = "strip") {
  CatPlotOptions options;
  options.kind = catplot_kind(kind);
  return catplot(pane, group, values, hue, options);
}

template <class Group, class Values>
Trace& catplot(Pane& pane, Group&& group, Values&& values, std::string_view kind = "strip") {
  if (kind == "box") {
    return boxplot(pane, std::forward<Group>(group), std::forward<Values>(values));
  }
  if (kind == "violin") {
    return violinplot(pane, std::forward<Group>(group), std::forward<Values>(values));
  }
  if (kind == "bar") {
    return barplot(pane, std::forward<Group>(group), std::forward<Values>(values));
  }
  if (kind == "count") {
    return countplot(pane, std::forward<Group>(group));
  }
  return stripplot(pane, std::forward<Group>(group), std::forward<Values>(values));
}

inline Studio& FacetGrid(Studio& studio, std::int64_t rows, std::int64_t cols, double gx = 18.0, double gy = 18.0) {
  return studio.mosaic(rows, cols, gx, gy);
}

inline Studio& PairGrid(Studio& studio, std::int64_t dimensions, double gx = 14.0, double gy = 14.0) {
  return studio.mosaic(dimensions, dimensions, gx, gy);
}

inline Studio& JointGrid(Studio& studio, double gx = 14.0, double gy = 14.0) {
  return studio.mosaic(2, 2, gx, gy);
}

enum class RelGridKind : std::uint8_t { Scatter, Line, Reg };
enum class DisGridKind : std::uint8_t { Hist, Density, Ecdf, Rug };

[[nodiscard]] inline RelGridKind relgrid_kind(std::string_view kind) {
  if (kind == "line") {
    return RelGridKind::Line;
  }
  if (kind == "reg") {
    return RelGridKind::Reg;
  }
  if (kind == "scatter") {
    return RelGridKind::Scatter;
  }
  throw std::invalid_argument("flash::relgridplot unknown kind: " + std::string(kind));
}

[[nodiscard]] inline DisGridKind disgrid_kind(std::string_view kind) {
  if (kind == "hist") {
    return DisGridKind::Hist;
  }
  if (kind == "density") {
    return DisGridKind::Density;
  }
  if (kind == "ecdf") {
    return DisGridKind::Ecdf;
  }
  if (kind == "rug") {
    return DisGridKind::Rug;
  }
  throw std::invalid_argument("flash::disgridplot unknown kind: " + std::string(kind));
}

struct RelGridPlotOptions {
  RelGridKind kind{RelGridKind::Scatter};
  std::int64_t rows{0};
  std::int64_t cols{0};
  double grid_gap_x{16.0};
  double grid_gap_y{16.0};
  bool show_grid{true};
  std::vector<std::string> titles{};
  std::vector<std::string> x_labels{};
  std::vector<std::string> y_labels{};
  RegPlotOptions reg{};
};

struct CatGridPlotOptions {
  std::int64_t rows{0};
  std::int64_t cols{0};
  double grid_gap_x{16.0};
  double grid_gap_y{16.0};
  bool show_grid{true};
  std::vector<std::string> titles{};
  std::vector<std::string> x_labels{};
  std::vector<std::string> y_labels{};
  CatPlotOptions cat{};
};

struct DisGridPlotOptions {
  DisGridKind kind{DisGridKind::Hist};
  std::int64_t rows{0};
  std::int64_t cols{0};
  double grid_gap_x{16.0};
  double grid_gap_y{16.0};
  bool show_grid{true};
  std::vector<std::string> titles{};
  std::vector<std::string> x_labels{};
  std::vector<std::string> y_labels{};
  std::int64_t bins{12};
  std::int64_t density_points{96};
  double rug_height{0.06};
};

[[nodiscard]] inline std::pair<std::int64_t, std::int64_t> resolve_grid_shape(
    std::size_t facets,
    std::int64_t rows_hint,
    std::int64_t cols_hint) {
  if (facets == 0) {
    throw std::invalid_argument("flash grid helpers require at least one facet");
  }
  const auto facet_count = static_cast<std::int64_t>(facets);
  if (rows_hint > 0 && cols_hint > 0) {
    if (rows_hint * cols_hint < facet_count) {
      throw std::invalid_argument("flash grid helpers: rows*cols is smaller than facet count");
    }
    return {rows_hint, cols_hint};
  }
  if (rows_hint > 0) {
    const auto cols = std::max<std::int64_t>(1, (facet_count + rows_hint - 1) / rows_hint);
    return {rows_hint, cols};
  }
  if (cols_hint > 0) {
    const auto rows = std::max<std::int64_t>(1, (facet_count + cols_hint - 1) / cols_hint);
    return {rows, cols_hint};
  }
  const auto cols = std::max<std::int64_t>(
      1,
      static_cast<std::int64_t>(std::ceil(std::sqrt(static_cast<double>(facet_count)))));
  const auto rows = std::max<std::int64_t>(1, (facet_count + cols - 1) / cols);
  return {rows, cols};
}

inline Studio& relgridplot(
    Studio& studio,
    const std::vector<Series>& xs,
    const std::vector<Series>& ys,
    const RelGridPlotOptions& options = {}) {
  if (xs.empty() || ys.empty() || xs.size() != ys.size()) {
    throw std::invalid_argument("flash::relgridplot requires non-empty aligned x/y facet vectors");
  }
  const auto [rows, cols] = resolve_grid_shape(xs.size(), options.rows, options.cols);
  FacetGrid(studio, rows, cols, options.grid_gap_x, options.grid_gap_y);
  const auto panel_count = static_cast<std::size_t>(rows * cols);
  for (const auto panel : std::views::iota(std::size_t{0}, panel_count)) {
    const auto row = static_cast<std::int64_t>(panel / static_cast<std::size_t>(cols));
    const auto col = static_cast<std::int64_t>(panel % static_cast<std::size_t>(cols));
    Pane& pane =
        panel < xs.size() && panel < options.titles.size() ? studio.pane2(options.titles[panel]) : studio.pane2();
    pane.place(row, col).grid(options.show_grid ? GridStyle::Major : GridStyle::None);
    if (panel >= xs.size()) {
      pane.hide();
      continue;
    }
    switch (options.kind) {
      case RelGridKind::Line:
        lineplot(pane, xs[panel], ys[panel]);
        break;
      case RelGridKind::Reg:
        regplot(pane, xs[panel], ys[panel], options.reg);
        break;
      case RelGridKind::Scatter:
      default:
        scatterplot(pane, xs[panel], ys[panel]);
        break;
    }
    if (panel < options.x_labels.size()) {
      pane.x_label(options.x_labels[panel]);
    }
    if (panel < options.y_labels.size()) {
      pane.y_label(options.y_labels[panel]);
    }
  }
  return studio;
}

inline Studio& relgridplot(
    Studio& studio,
    const std::vector<Series>& xs,
    const std::vector<Series>& ys,
    std::string_view kind) {
  RelGridPlotOptions options;
  options.kind = relgrid_kind(kind);
  return relgridplot(studio, xs, ys, options);
}

inline Studio& catgridplot(
    Studio& studio,
    const std::vector<Note>& groups,
    const std::vector<Series>& values,
    const CatGridPlotOptions& options = {}) {
  if (groups.empty() || values.empty() || groups.size() != values.size()) {
    throw std::invalid_argument("flash::catgridplot requires non-empty aligned group/value facet vectors");
  }
  const auto [rows, cols] = resolve_grid_shape(groups.size(), options.rows, options.cols);
  FacetGrid(studio, rows, cols, options.grid_gap_x, options.grid_gap_y);
  const auto panel_count = static_cast<std::size_t>(rows * cols);
  for (const auto panel : std::views::iota(std::size_t{0}, panel_count)) {
    const auto row = static_cast<std::int64_t>(panel / static_cast<std::size_t>(cols));
    const auto col = static_cast<std::int64_t>(panel % static_cast<std::size_t>(cols));
    Pane& pane =
        panel < groups.size() && panel < options.titles.size() ? studio.pane2(options.titles[panel]) : studio.pane2();
    pane.place(row, col).grid(options.show_grid ? GridStyle::Major : GridStyle::None);
    if (panel >= groups.size()) {
      pane.hide();
      continue;
    }
    catplot(pane, groups[panel], values[panel], options.cat);
    if (panel < options.x_labels.size()) {
      pane.x_label(options.x_labels[panel]);
    }
    if (panel < options.y_labels.size()) {
      pane.y_label(options.y_labels[panel]);
    }
  }
  return studio;
}

inline Studio& catgridplot(
    Studio& studio,
    const std::vector<Note>& groups,
    const std::vector<Series>& values,
    const std::vector<Note>& hues,
    const CatGridPlotOptions& options = {}) {
  if (groups.empty() || values.empty() || hues.empty() ||
      groups.size() != values.size() || groups.size() != hues.size()) {
    throw std::invalid_argument("flash::catgridplot requires aligned group/value/hue facet vectors");
  }
  const auto [rows, cols] = resolve_grid_shape(groups.size(), options.rows, options.cols);
  FacetGrid(studio, rows, cols, options.grid_gap_x, options.grid_gap_y);
  const auto panel_count = static_cast<std::size_t>(rows * cols);
  for (const auto panel : std::views::iota(std::size_t{0}, panel_count)) {
    const auto row = static_cast<std::int64_t>(panel / static_cast<std::size_t>(cols));
    const auto col = static_cast<std::int64_t>(panel % static_cast<std::size_t>(cols));
    Pane& pane =
        panel < groups.size() && panel < options.titles.size() ? studio.pane2(options.titles[panel]) : studio.pane2();
    pane.place(row, col).grid(options.show_grid ? GridStyle::Major : GridStyle::None);
    if (panel >= groups.size()) {
      pane.hide();
      continue;
    }
    catplot(pane, groups[panel], values[panel], hues[panel], options.cat);
    if (panel < options.x_labels.size()) {
      pane.x_label(options.x_labels[panel]);
    }
    if (panel < options.y_labels.size()) {
      pane.y_label(options.y_labels[panel]);
    }
  }
  return studio;
}

inline Studio& disgridplot(
    Studio& studio,
    const std::vector<Series>& values,
    const DisGridPlotOptions& options = {}) {
  if (values.empty()) {
    throw std::invalid_argument("flash::disgridplot requires at least one facet series");
  }
  const auto [rows, cols] = resolve_grid_shape(values.size(), options.rows, options.cols);
  FacetGrid(studio, rows, cols, options.grid_gap_x, options.grid_gap_y);
  const auto panel_count = static_cast<std::size_t>(rows * cols);
  for (const auto panel : std::views::iota(std::size_t{0}, panel_count)) {
    const auto row = static_cast<std::int64_t>(panel / static_cast<std::size_t>(cols));
    const auto col = static_cast<std::int64_t>(panel % static_cast<std::size_t>(cols));
    Pane& pane =
        panel < values.size() && panel < options.titles.size() ? studio.pane2(options.titles[panel]) : studio.pane2();
    pane.place(row, col).grid(options.show_grid ? GridStyle::Major : GridStyle::None);
    if (panel >= values.size()) {
      pane.hide();
      continue;
    }
    switch (options.kind) {
      case DisGridKind::Density:
        kdeplot(pane, values[panel], options.density_points);
        break;
      case DisGridKind::Ecdf:
        ecdfplot(pane, values[panel]);
        break;
      case DisGridKind::Rug:
        rugplot(pane, values[panel], options.rug_height);
        break;
      case DisGridKind::Hist:
      default:
        histplot(pane, values[panel], static_cast<int>(std::max<std::int64_t>(1, options.bins)));
        break;
    }
    if (panel < options.x_labels.size()) {
      pane.x_label(options.x_labels[panel]);
    }
    if (panel < options.y_labels.size()) {
      pane.y_label(options.y_labels[panel]);
    }
  }
  return studio;
}

inline Studio& disgridplot(Studio& studio, const std::vector<Series>& values, std::string_view kind) {
  DisGridPlotOptions options;
  options.kind = disgrid_kind(kind);
  return disgridplot(studio, values, options);
}

template <class Mapper>
  requires std::invocable<Mapper, Pane&, std::int64_t, std::int64_t>
inline Studio& facet_map_plot(
    Studio& studio,
    std::int64_t rows,
    std::int64_t cols,
    Mapper&& mapper,
    double gx = 18.0,
    double gy = 18.0) {
  if (rows <= 0 || cols <= 0) {
    throw std::invalid_argument("flash::facet_map_plot rows/cols must be positive");
  }
  FacetGrid(studio, rows, cols, gx, gy);
  for (const auto row : std::views::iota(std::int64_t{0}, rows)) {
    for (const auto col : std::views::iota(std::int64_t{0}, cols)) {
      Pane& pane = studio.pane2();
      pane.place(row, col);
      std::invoke(mapper, pane, row, col);
    }
  }
  return studio;
}

template <class Mapper>
  requires std::invocable<Mapper, Pane&, std::int64_t, std::int64_t, bool>
inline Studio& pairgrid_map_plot(
    Studio& studio,
    std::int64_t dimensions,
    Mapper&& mapper,
    double gx = 14.0,
    double gy = 14.0) {
  if (dimensions <= 0) {
    throw std::invalid_argument("flash::pairgrid_map_plot dimensions must be positive");
  }
  PairGrid(studio, dimensions, gx, gy);
  for (const auto row : std::views::iota(std::int64_t{0}, dimensions)) {
    for (const auto col : std::views::iota(std::int64_t{0}, dimensions)) {
      Pane& pane = studio.pane2();
      pane.place(row, col);
      std::invoke(mapper, pane, row, col, row == col);
    }
  }
  return studio;
}

template <class JointMapper, class MarginalMapper>
  requires std::invocable<JointMapper, Pane&> && std::invocable<MarginalMapper, Pane&, bool>
inline Studio& jointgrid_map_plot(
    Studio& studio,
    JointMapper&& joint_mapper,
    MarginalMapper&& marginal_mapper,
    double gx = 14.0,
    double gy = 14.0) {
  JointGrid(studio, gx, gy);
  Pane& top = studio.pane2();
  top.place(0, 0);
  std::invoke(marginal_mapper, top, true);

  Pane& joint = studio.pane2();
  joint.place(1, 0);
  std::invoke(joint_mapper, joint);

  Pane& right = studio.pane2();
  right.place(1, 1);
  std::invoke(marginal_mapper, right, false);

  studio.pane2().place(0, 1).hide();
  return studio;
}

enum class PairPlotTriangle : std::uint8_t { Full, Lower, Upper };
enum class PairPlotKind : std::uint8_t { Scatter, Reg };
enum class PairPlotDiagKind : std::uint8_t { Histogram, Density };

struct PairPlotOptions {
  PairPlotTriangle triangle{PairPlotTriangle::Full};
  PairPlotKind kind{PairPlotKind::Scatter};
  PairPlotDiagKind diag_kind{PairPlotDiagKind::Histogram};
  bool corner{false};
  bool dropna{false};
  bool monochrome{true};
  std::optional<int> auto_tick_decimals{std::nullopt};
  std::vector<std::string> labels{};
  bool auto_size{true};
  double min_width{1280.0};
  double min_height{900.0};
  double cell_width{210.0};
  double cell_height{210.0};
  double outer_pad_x{120.0};
  double outer_pad_y{120.0};
  std::int64_t max_points{2500};
  std::int64_t diag_bins{14};
  double grid_gap_x{8.0};
  double grid_gap_y{8.0};
  double tick_font_scale{0.8};
  double axis_label_font_scale{0.9};
  float diag_trace_width{0.75F};
  float scatter_trace_width{0.45F};
  double marker_area_scale{380.0};
  double marker_area_min{7.0};
  double marker_area_max{18.0};
  bool show_grid{false};
  bool hide_inner_ticks{true};
  std::int64_t diag_density_points{128};
};

inline Studio& pairplot(Studio& studio, const std::vector<Series>& columns, const PairPlotOptions& options = {}) {
  if (columns.empty()) {
    throw std::invalid_argument("flash::pairplot requires at least one series");
  }

  auto pair_options = options;
  if (pair_options.corner && pair_options.triangle == PairPlotTriangle::Full) {
    pair_options.triangle = PairPlotTriangle::Lower;
  }

  std::vector<Series> prepared = detail::prepare_numeric_columns(columns, false);
  const std::int64_t n = static_cast<std::int64_t>(prepared.size());
  (void)detail::require_equal_lengths(prepared, "flash::pairplot");

  if (pair_options.dropna) {
    Series mask = prepared.front().isfinite();
    for (const auto idx : std::views::iota(std::size_t{1}, prepared.size())) {
      mask = mask.logical_and(prepared[idx].isfinite());
    }
    for (auto& series : prepared) {
      series = series.masked_select(mask);
    }
  }

  detail::downsample_columns(prepared, pair_options.max_points);
  const std::int64_t row_count = prepared.front().numel();
  if (row_count <= 0) {
    throw std::invalid_argument("flash::pairplot has no rows after filtering/sampling");
  }

  if (pair_options.auto_size) {
    const double cols = static_cast<double>(n);
    const double rows = static_cast<double>(n);
    const double suggested_width =
        std::max(
            pair_options.min_width,
            cols * pair_options.cell_width + std::max(0.0, cols - 1.0) * pair_options.grid_gap_x + pair_options.outer_pad_x);
    const double suggested_height =
        std::max(
            pair_options.min_height,
            rows * pair_options.cell_height + std::max(0.0, rows - 1.0) * pair_options.grid_gap_y + pair_options.outer_pad_y);
    studio.size(suggested_width, suggested_height);
  }
  studio.mosaic(n, n, pair_options.grid_gap_x, pair_options.grid_gap_y);
  const auto colors = detail::categorical_colors(n);
  const bool labels_match = pair_options.labels.size() == static_cast<std::size_t>(n);
  const double safe_tick_font_scale =
      (std::isfinite(pair_options.tick_font_scale) && pair_options.tick_font_scale > 0.0)
          ? pair_options.tick_font_scale
          : 0.8;
  const double safe_axis_label_font_scale =
      (std::isfinite(pair_options.axis_label_font_scale) && pair_options.axis_label_font_scale > 0.0)
          ? pair_options.axis_label_font_scale
          : 0.9;

  for (const auto row : std::views::iota(std::int64_t{0}, n)) {
    for (const auto col : std::views::iota(std::int64_t{0}, n)) {
      Pane& pane = studio.pane2();
      const bool edge_x = row == n - 1;
      const bool edge_y = col == 0;
      const bool show_ticks = !pair_options.hide_inner_ticks || edge_x || edge_y;
      const double tick_scale = show_ticks ? safe_tick_font_scale : 0.01;
      pane.place(row, col)
          .grid(pair_options.show_grid ? GridStyle::Major : GridStyle::None)
          .tick_font_scale(tick_scale)
          .axis_label_font_scale(safe_axis_label_font_scale);
      if (pair_options.auto_tick_decimals.has_value()) {
        pane.x_auto_tick_decimals(*pair_options.auto_tick_decimals);
        pane.y_auto_tick_decimals(*pair_options.auto_tick_decimals);
      }
      const bool keep_cell =
          pair_options.triangle == PairPlotTriangle::Full ||
          (pair_options.triangle == PairPlotTriangle::Lower && row >= col) ||
          (pair_options.triangle == PairPlotTriangle::Upper && row <= col);
      if (!keep_cell) {
        pane.hide();
        continue;
      }
      if (!edge_x) {
        pane.x_label("");
      } else {
        pane.x_label(labels_match ? pair_options.labels[static_cast<std::size_t>(col)] : "x" + std::to_string(col + 1));
      }
      if (!edge_y) {
        pane.y_label("");
      } else {
        pane.y_label(labels_match ? pair_options.labels[static_cast<std::size_t>(row)] : "x" + std::to_string(row + 1));
      }

      const auto& x_values = prepared[static_cast<std::size_t>(col)];
      const auto& y_values = prepared[static_cast<std::size_t>(row)];
      if (row == col) {
        if (pair_options.diag_kind == PairPlotDiagKind::Density) {
          const auto [dx, dy] = detail::gaussian_kde(
              y_values,
              std::max<std::int64_t>(32, pair_options.diag_density_points));
          auto& trace = pane.trend(dx, dy);
          trace.width(pair_options.diag_trace_width);
          if (!pair_options.monochrome && !colors.empty()) {
            trace.stroke(colors[static_cast<std::size_t>(row) % colors.size()]);
          }
        } else {
          auto& trace = pane.bucket(
              y_values,
              static_cast<int>(std::max<std::int64_t>(1, pair_options.diag_bins)));
          trace.width(pair_options.diag_trace_width);
          if (!pair_options.monochrome && !colors.empty()) {
            trace.stroke(colors[static_cast<std::size_t>(row) % colors.size()]);
          }
        }
      } else {
        const std::int64_t points = std::max<std::int64_t>(1, std::min(x_values.numel(), y_values.numel()));
        const double marker_area = std::clamp(
            pair_options.marker_area_scale
                / std::sqrt(static_cast<double>(points) * static_cast<double>(std::max<std::int64_t>(n, 1))),
            pair_options.marker_area_min,
            pair_options.marker_area_max);
        auto& scatter = pane.cloud(x_values, y_values, std::array<double, 1>{marker_area});
        scatter.width(pair_options.scatter_trace_width);
        if (!pair_options.monochrome && !colors.empty()) {
          scatter.stroke(colors[static_cast<std::size_t>(row + col) % colors.size()]);
        }
        if (pair_options.kind == PairPlotKind::Reg) {
          RegPlotOptions reg_options;
          reg_options.show_scatter = false;
          reg_options.show_fit = true;
          reg_options.show_ci = false;
          reg_options.fit_points = 120;
          reg_options.fit_trace_width = std::max(0.8F, pair_options.scatter_trace_width + 0.45F);
          auto& fit_trace = regplot(pane, x_values, y_values, reg_options);
          if (!pair_options.monochrome && !colors.empty()) {
            fit_trace.stroke(colors[static_cast<std::size_t>(row + col) % colors.size()]);
          }
        }
      }
    }
  }
  return studio;
}

struct ParallelCoordinatesOptions {
  std::vector<std::string> labels{};
  bool normalize{true};
  std::int64_t max_points{1200};
  double alpha{0.24};
  float trace_width{0.9F};
  bool show_grid{true};
  bool show_legend{true};
};

struct AndrewsCurvesOptions {
  std::vector<std::string> labels{};
  bool normalize{true};
  std::int64_t points{220};
  std::int64_t max_curves{600};
  double alpha{0.28};
  float trace_width{0.9F};
  bool show_grid{true};
  bool show_legend{true};
};

struct RadvizOptions {
  std::vector<std::string> labels{};
  bool normalize{true};
  std::int64_t max_points{3000};
  double marker_area{18.0};
  float trace_width{0.55F};
  bool show_grid{true};
  bool show_anchors{true};
  bool show_legend{true};
};

struct LagPlotOptions {
  std::int64_t lag{1};
  std::int64_t max_points{2500};
  double marker_area{16.0};
  float trace_width{0.55F};
};

struct AutocorrelationPlotOptions {
  std::int64_t max_lag{40};
  bool show_confidence_band{true};
  double confidence_level{0.95};
  float trace_width{1.2F};
};

struct AcfPlotOptions {
  std::int64_t max_lag{40};
  bool include_zero_lag{false};
  bool show_confidence_band{true};
  double confidence_level{0.95};
  float trace_width{1.2F};
  double marker_area{14.0};
};

struct ResidAcfPlotOptions {
  RegPlotOptions reg{};
  AcfPlotOptions acf{};
};

struct PacfPlotOptions {
  std::int64_t max_lag{40};
  bool include_zero_lag{false};
  bool show_confidence_band{true};
  double confidence_level{0.95};
  float trace_width{1.2F};
  double marker_area{14.0};
};

struct CcfPlotOptions {
  std::int64_t max_lag{40};
  bool show_confidence_band{true};
  double confidence_level{0.95};
  float trace_width{1.2F};
  double marker_area{14.0};
};

enum class BootstrapStatistic : std::uint8_t {
  Mean,
  Median,
  Std,
};

struct BootstrapPlotOptions {
  std::int64_t samples{300};
  std::int64_t sample_size{0};
  std::uint64_t seed{7};
  int bins{24};
  BootstrapStatistic statistic{BootstrapStatistic::Mean};
  float trace_width{0.85F};
};

struct SummaryOptions {
  std::string title{"table"};
  int precision{4};
  bool include_header{true};
  std::int64_t max_cases{10};
  bool transpose{false};
  bool include_case_index{true};
  std::int64_t features_per_table{0};
  std::int64_t tables_per_row{1};
  std::int64_t table_gap_spaces{4};
};

inline Studio& parallelplot(
    Studio& studio,
    const std::vector<Series>& columns,
    const ParallelCoordinatesOptions& options = {}) {
  if (columns.size() < 2) {
    throw std::invalid_argument("flash::parallelplot requires at least two columns");
  }

  auto prepared = detail::prepare_numeric_columns(columns, options.normalize);
  const std::int64_t ncols = static_cast<std::int64_t>(prepared.size());
  const std::int64_t nrows = detail::require_equal_lengths(prepared, "flash::parallelplot");
  if (nrows <= 0) {
    throw std::invalid_argument("flash::parallelplot requires at least one row");
  }

  detail::downsample_columns(prepared, options.max_points);
  const std::int64_t plotted_rows = detail::require_equal_lengths(prepared, "flash::parallelplot");

  const std::vector<double> x = detail::ordinal_levels(ncols, 0.0);
  const auto labels = detail::default_feature_labels(options.labels, ncols);
  const auto flat_values = detail::row_major_values(prepared, plotted_rows);

  auto& pane = studio.pane2();
  pane.grid(options.show_grid ? GridStyle::Major : GridStyle::None)
      .legend(options.show_legend ? KeyPlace::Auto : KeyPlace::None)
      .x_ticks(x, labels)
      .x_label("feature")
      .y_label(options.normalize ? "normalized value" : "value");

  const auto colors = detail::categorical_colors(8);
  std::vector<double> y(static_cast<std::size_t>(ncols), 0.0);
  const auto cols_us = static_cast<std::size_t>(ncols);
  for (const auto row : std::views::iota(std::int64_t{0}, plotted_rows)) {
    const auto offset = static_cast<std::size_t>(row) * cols_us;
    std::copy_n(flat_values.begin() + static_cast<std::ptrdiff_t>(offset), cols_us, y.begin());
    auto& trace = pane.trend(x, y);
    Rgba color = colors[static_cast<std::size_t>(row) % colors.size()];
    color.alpha = std::clamp(options.alpha, 0.05, 1.0);
    trace.stroke(color).width(options.trace_width).transparent(color.alpha < 0.999);
  }
  return studio;
}

inline Studio& parallelplot(
    Studio& studio,
    const std::vector<Series>& columns,
    const Note& class_labels,
    const ParallelCoordinatesOptions& options = {}) {
  if (columns.size() < 2) {
    throw std::invalid_argument("flash::parallelplot requires at least two columns");
  }
  std::vector<std::string> labels = detail::note_values(class_labels);
  if (labels.empty()) {
    return parallelplot(studio, columns, options);
  }

  auto prepared = detail::prepare_numeric_columns(columns, options.normalize);
  const std::int64_t ncols = static_cast<std::int64_t>(prepared.size());
  const std::int64_t nrows = detail::require_equal_lengths(prepared, "flash::parallelplot");
  if (nrows != static_cast<std::int64_t>(labels.size())) {
    throw std::invalid_argument("flash::parallelplot: class label length must match rows");
  }

  detail::downsample_columns(prepared, options.max_points, &labels);
  const std::int64_t plotted_rows = detail::require_equal_lengths(prepared, "flash::parallelplot");

  const std::vector<double> x = detail::ordinal_levels(ncols, 0.0);
  const auto feature_labels = detail::default_feature_labels(options.labels, ncols);
  const auto flat_values = detail::row_major_values(prepared, plotted_rows);

  std::unordered_map<std::string, std::size_t> color_index;
  std::vector<std::string> ordered_classes;
  for (const auto& label : labels) {
    const auto [it, inserted] = color_index.emplace(label, color_index.size());
    if (inserted) {
      ordered_classes.push_back(label);
    }
    (void)it;
  }

  auto& pane = studio.pane2();
  pane.grid(options.show_grid ? GridStyle::Major : GridStyle::None)
      .legend(options.show_legend ? KeyPlace::Auto : KeyPlace::None)
      .x_ticks(x, feature_labels)
      .x_label("feature")
      .y_label(options.normalize ? "normalized value" : "value");
  const auto colors = detail::categorical_colors(static_cast<std::int64_t>(ordered_classes.size()));
  std::unordered_map<std::string, bool> emitted;
  std::vector<double> y(static_cast<std::size_t>(ncols), 0.0);
  const auto cols_us = static_cast<std::size_t>(ncols);
  for (const auto row : std::views::iota(std::size_t{0}, labels.size())) {
    const auto offset = row * cols_us;
    std::copy_n(flat_values.begin() + static_cast<std::ptrdiff_t>(offset), cols_us, y.begin());
    auto& trace = pane.trend(x, y);
    const auto class_name = labels[row];
    const auto class_idx = color_index[class_name];
    Rgba color = colors[class_idx % colors.size()];
    color.alpha = std::clamp(options.alpha, 0.05, 1.0);
    trace.stroke(color).width(options.trace_width).transparent(color.alpha < 0.999);
    if (!emitted[class_name]) {
      trace.label(class_name);
      emitted[class_name] = true;
    }
  }
  return studio;
}

inline Studio& andrews_curves(
    Studio& studio,
    const std::vector<Series>& columns,
    const AndrewsCurvesOptions& options = {}) {
  if (columns.size() < 2) {
    throw std::invalid_argument("flash::andrews_curves requires at least two columns");
  }
  auto prepared = detail::prepare_numeric_columns(columns, options.normalize);
  const std::int64_t ncols = static_cast<std::int64_t>(prepared.size());
  const std::int64_t nrows = detail::require_equal_lengths(prepared, "flash::andrews_curves");
  if (nrows <= 0) {
    throw std::invalid_argument("flash::andrews_curves requires at least one row");
  }

  detail::downsample_columns(prepared, options.max_curves);
  const std::int64_t plotted_rows = detail::require_equal_lengths(prepared, "flash::andrews_curves");

  const auto t = pluss::linspace(-3.14159265358979323846,
                                 3.14159265358979323846,
                                 std::max<std::int64_t>(64, options.points),
                                 ScalarType::Float64);
  const auto t_values = detail::to_f64_vector(t);
  const auto basis = detail::andrews_basis(t_values, ncols);
  const auto values_matrix = detail::matrix_from_columns(prepared, plotted_rows);
  const auto curve_matrix = values_matrix.matmul(basis).to(ScalarType::Float64).contiguous();
  const auto flat_curves = curve_matrix.to_vec<double>();
  const auto point_count = static_cast<std::size_t>(t_values.size());

  auto& pane = studio.pane2();
  pane.grid(options.show_grid ? GridStyle::Major : GridStyle::None)
      .legend(options.show_legend ? KeyPlace::Auto : KeyPlace::None)
      .x_label("t")
      .y_label("f(t)");
  const auto colors = detail::categorical_colors(8);
  std::vector<double> curve(point_count, 0.0);
  for (const auto row : std::views::iota(std::int64_t{0}, plotted_rows)) {
    const auto offset = static_cast<std::size_t>(row) * point_count;
    std::copy_n(flat_curves.begin() + static_cast<std::ptrdiff_t>(offset), point_count, curve.begin());
    auto& trace = pane.trend(t_values, curve);
    Rgba color = colors[static_cast<std::size_t>(row) % colors.size()];
    color.alpha = std::clamp(options.alpha, 0.05, 1.0);
    trace.stroke(color).width(options.trace_width).transparent(color.alpha < 0.999);
  }
  return studio;
}

inline Studio& andrews_curves(
    Studio& studio,
    const std::vector<Series>& columns,
    const Note& class_labels,
    const AndrewsCurvesOptions& options = {}) {
  if (columns.size() < 2) {
    throw std::invalid_argument("flash::andrews_curves requires at least two columns");
  }

  std::vector<std::string> labels = detail::note_values(class_labels);
  auto prepared = detail::prepare_numeric_columns(columns, options.normalize);
  const std::int64_t ncols = static_cast<std::int64_t>(prepared.size());
  const std::int64_t nrows = detail::require_equal_lengths(prepared, "flash::andrews_curves");
  if (nrows != static_cast<std::int64_t>(labels.size())) {
    throw std::invalid_argument("flash::andrews_curves: class label length must match rows");
  }

  detail::downsample_columns(prepared, options.max_curves, &labels);
  const std::int64_t plotted_rows = detail::require_equal_lengths(prepared, "flash::andrews_curves");

  const auto t = pluss::linspace(-3.14159265358979323846,
                                 3.14159265358979323846,
                                 std::max<std::int64_t>(64, options.points),
                                 ScalarType::Float64);
  const auto t_values = detail::to_f64_vector(t);
  const auto basis = detail::andrews_basis(t_values, ncols);
  const auto values_matrix = detail::matrix_from_columns(prepared, plotted_rows);
  const auto curve_matrix = values_matrix.matmul(basis).to(ScalarType::Float64).contiguous();
  const auto flat_curves = curve_matrix.to_vec<double>();
  const auto point_count = static_cast<std::size_t>(t_values.size());
  std::unordered_map<std::string, std::size_t> color_index;
  std::vector<std::string> ordered_classes;
  for (const auto& label : labels) {
    const auto [it, inserted] = color_index.emplace(label, color_index.size());
    if (inserted) {
      ordered_classes.push_back(label);
    }
    (void)it;
  }
  const auto colors = detail::categorical_colors(static_cast<std::int64_t>(ordered_classes.size()));
  std::unordered_map<std::string, bool> emitted;

  auto& pane = studio.pane2();
  pane.grid(options.show_grid ? GridStyle::Major : GridStyle::None)
      .legend(options.show_legend ? KeyPlace::Auto : KeyPlace::None)
      .x_label("t")
      .y_label("f(t)");

  std::vector<double> curve(point_count, 0.0);
  for (const auto row : std::views::iota(std::size_t{0}, labels.size())) {
    const auto offset = row * point_count;
    std::copy_n(flat_curves.begin() + static_cast<std::ptrdiff_t>(offset), point_count, curve.begin());
    auto& trace = pane.trend(t_values, curve);
    const auto class_name = labels[row];
    const auto class_idx = color_index[class_name];
    Rgba color = colors[class_idx % colors.size()];
    color.alpha = std::clamp(options.alpha, 0.05, 1.0);
    trace.stroke(color).width(options.trace_width).transparent(color.alpha < 0.999);
    if (!emitted[class_name]) {
      trace.label(class_name);
      emitted[class_name] = true;
    }
  }
  return studio;
}

inline Studio& radviz(
    Studio& studio,
    const std::vector<Series>& columns,
    const RadvizOptions& options = {}) {
  if (columns.size() < 3) {
    throw std::invalid_argument("flash::radviz requires at least three columns");
  }

  auto prepared = detail::prepare_numeric_columns(columns, options.normalize);
  const std::int64_t ncols = static_cast<std::int64_t>(prepared.size());
  const std::int64_t nrows = detail::require_equal_lengths(prepared, "flash::radviz");
  if (nrows <= 0) {
    throw std::invalid_argument("flash::radviz requires at least one row");
  }

  detail::downsample_columns(prepared, options.max_points);
  const std::int64_t plotted_rows = detail::require_equal_lengths(prepared, "flash::radviz");
  const auto flat_values = detail::row_major_values(prepared, plotted_rows);
  std::vector<double> anchor_x;
  std::vector<double> anchor_y;
  anchor_x.reserve(static_cast<std::size_t>(ncols));
  anchor_y.reserve(static_cast<std::size_t>(ncols));
  for (const auto i : std::views::iota(std::int64_t{0}, ncols)) {
    const double theta = 2.0 * 3.14159265358979323846 * static_cast<double>(i) / static_cast<double>(ncols);
    anchor_x.push_back(std::cos(theta));
    anchor_y.push_back(std::sin(theta));
  }

  std::vector<double> x_points;
  std::vector<double> y_points;
  x_points.assign(static_cast<std::size_t>(plotted_rows), 0.0);
  y_points.assign(static_cast<std::size_t>(plotted_rows), 0.0);
  std::vector<double> weights(static_cast<std::size_t>(plotted_rows), 0.0);
  const auto flat_size = flat_values.size();
  const auto cols_us = static_cast<std::size_t>(ncols);
  for (const auto idx : std::views::iota(std::size_t{0}, flat_size)) {
    const auto row = idx / cols_us;
    const auto col = idx % cols_us;
    const double weight = std::max(0.0, flat_values[idx]);
    weights[row] += weight;
    x_points[row] += weight * anchor_x[col];
    y_points[row] += weight * anchor_y[col];
  }
  for (const auto row : std::views::iota(std::size_t{0}, x_points.size())) {
    const double denom = weights[row] > 0.0 ? weights[row] : 1.0;
    x_points[row] /= denom;
    y_points[row] /= denom;
  }

  auto& pane = studio.pane2();
  pane.grid(options.show_grid ? GridStyle::Major : GridStyle::None)
      .legend(KeyPlace::None)
      .x_label("radviz-x")
      .y_label("radviz-y")
      .x_limits({-1.1, 1.1})
      .y_limits({-1.1, 1.1});
  auto& trace = pane.cloud(x_points, y_points, std::array<double, 1>{std::max(2.0, options.marker_area)});
  trace.width(options.trace_width);
  if (options.show_anchors) {
    auto& anchors = pane.cloud(anchor_x, anchor_y, std::array<double, 1>{26.0});
    anchors.width(0.85F);
    const auto labels = detail::default_feature_labels(options.labels, ncols);
    for (const auto i : std::views::iota(std::size_t{0}, labels.size())) {
      pane.pin(labels[i], anchor_x[i], anchor_y[i], 18.0, -10.0);
    }
  }
  return studio;
}

inline Studio& radviz(
    Studio& studio,
    const std::vector<Series>& columns,
    const Note& class_labels,
    const RadvizOptions& options = {}) {
  if (columns.size() < 3) {
    throw std::invalid_argument("flash::radviz requires at least three columns");
  }
  std::vector<std::string> labels = detail::note_values(class_labels);

  auto prepared = detail::prepare_numeric_columns(columns, options.normalize);
  const std::int64_t ncols = static_cast<std::int64_t>(prepared.size());
  const std::int64_t nrows = detail::require_equal_lengths(prepared, "flash::radviz");
  if (nrows != static_cast<std::int64_t>(labels.size())) {
    throw std::invalid_argument("flash::radviz: class label length must match rows");
  }

  detail::downsample_columns(prepared, options.max_points, &labels);
  const std::int64_t plotted_rows = detail::require_equal_lengths(prepared, "flash::radviz");
  const auto flat_values = detail::row_major_values(prepared, plotted_rows);

  std::vector<double> anchor_x;
  std::vector<double> anchor_y;
  anchor_x.reserve(static_cast<std::size_t>(ncols));
  anchor_y.reserve(static_cast<std::size_t>(ncols));
  for (const auto i : std::views::iota(std::int64_t{0}, ncols)) {
    const double theta = 2.0 * 3.14159265358979323846 * static_cast<double>(i) / static_cast<double>(ncols);
    anchor_x.push_back(std::cos(theta));
    anchor_y.push_back(std::sin(theta));
  }

  std::unordered_map<std::string, std::size_t> color_index;
  std::vector<std::string> ordered_classes;
  for (const auto& label : labels) {
    const auto [it, inserted] = color_index.emplace(label, color_index.size());
    if (inserted) {
      ordered_classes.push_back(label);
    }
    (void)it;
  }
  std::vector<std::vector<double>> grouped_x(ordered_classes.size());
  std::vector<std::vector<double>> grouped_y(ordered_classes.size());
  std::vector<double> x_points(static_cast<std::size_t>(plotted_rows), 0.0);
  std::vector<double> y_points(static_cast<std::size_t>(plotted_rows), 0.0);
  std::vector<double> weights(static_cast<std::size_t>(plotted_rows), 0.0);
  const auto cols_us = static_cast<std::size_t>(ncols);
  for (const auto idx : std::views::iota(std::size_t{0}, flat_values.size())) {
    const auto row = idx / cols_us;
    const auto col = idx % cols_us;
    const double weight = std::max(0.0, flat_values[idx]);
    weights[row] += weight;
    x_points[row] += weight * anchor_x[col];
    y_points[row] += weight * anchor_y[col];
  }
  for (const auto row : std::views::iota(std::size_t{0}, labels.size())) {
    const double denom = weights[row] > 0.0 ? weights[row] : 1.0;
    const auto class_idx = color_index[labels[row]];
    grouped_x[class_idx].push_back(x_points[row] / denom);
    grouped_y[class_idx].push_back(y_points[row] / denom);
  }

  auto& pane = studio.pane2();
  pane.grid(options.show_grid ? GridStyle::Major : GridStyle::None)
      .legend(options.show_legend ? KeyPlace::Auto : KeyPlace::None)
      .x_label("radviz-x")
      .y_label("radviz-y")
      .x_limits({-1.1, 1.1})
      .y_limits({-1.1, 1.1});
  const auto colors = detail::categorical_colors(static_cast<std::int64_t>(ordered_classes.size()));
  for (const auto i : std::views::iota(std::size_t{0}, ordered_classes.size())) {
    auto& trace = pane.cloud(grouped_x[i], grouped_y[i], std::array<double, 1>{std::max(2.0, options.marker_area)});
    trace.stroke(colors[i % colors.size()]).width(options.trace_width).label(ordered_classes[i]);
  }
  if (options.show_anchors) {
    auto& anchors = pane.cloud(anchor_x, anchor_y, std::array<double, 1>{26.0});
    anchors.width(0.85F);
    const auto feature_labels = detail::default_feature_labels(options.labels, ncols);
    for (const auto i : std::views::iota(std::size_t{0}, feature_labels.size())) {
      pane.pin(feature_labels[i], anchor_x[i], anchor_y[i], 18.0, -10.0);
    }
  }
  return studio;
}

template <class Values>
Studio& lagplot(Studio& studio, Values&& values, const LagPlotOptions& options = {}) {
  const Series base = detail::flatten_f64(detail::to_series(std::forward<Values>(values)));
  const std::int64_t n = base.numel();
  if (n < 2) {
    throw std::invalid_argument("flash::lagplot requires at least two points");
  }
  const std::int64_t lag = std::clamp<std::int64_t>(options.lag, 1, n - 1);
  const std::int64_t usable = n - lag;
  const std::int64_t step =
      (options.max_points > 0 && usable > options.max_points)
          ? std::max<std::int64_t>(1, (usable + options.max_points - 1) / options.max_points)
          : 1;
  const auto idx_x = Series::arange(0, usable, step, ScalarType::Int64);
  const auto idx_y = Series::arange(lag, lag + usable, step, ScalarType::Int64);
  auto& pane = studio.pane2();
  pane.grid(GridStyle::Major).x_label("x(t)").y_label("x(t + lag)");
  auto& trace = pane.cloud(base.take(idx_x), base.take(idx_y), std::array<double, 1>{std::max(2.0, options.marker_area)});
  trace.width(options.trace_width).label("lag");
  return studio;
}

template <class Values>
Studio& acfplot(Studio& studio, Values&& values, const AcfPlotOptions& options = {}) {
  const Series base = detail::flatten_f64(detail::to_series(std::forward<Values>(values)));
  const std::int64_t n = base.numel();
  if (n < 3) {
    throw std::invalid_argument("flash::acfplot requires at least three points");
  }
  const double global_mean = detail::scalar_value(base.mean());
  const Series centered = base.sub(global_mean);
  const double denom = detail::scalar_value(centered.mul(centered).sum());
  const bool valid_denom = std::isfinite(denom) && denom > 0.0;
  const std::int64_t max_lag = std::clamp<std::int64_t>(options.max_lag, 1, n - 1);
  const std::int64_t lag_start = options.include_zero_lag ? 0 : 1;
  std::vector<double> lags;
  std::vector<double> acf;
  lags.reserve(static_cast<std::size_t>(max_lag - lag_start + 1));
  acf.reserve(static_cast<std::size_t>(max_lag - lag_start + 1));
  for (const auto lag : std::views::iota(lag_start, max_lag + 1)) {
    lags.push_back(static_cast<double>(lag));
    if (lag == 0) {
      acf.push_back(1.0);
      continue;
    }
    if (!valid_denom) {
      acf.push_back(0.0);
      continue;
    }
    const Series idx_x = Series::arange(0, n - lag, 1, ScalarType::Int64);
    const Series idx_y = Series::arange(lag, n, 1, ScalarType::Int64);
    const double numerator = detail::scalar_value(centered.take(idx_x).mul(centered.take(idx_y)).sum());
    const double corr = numerator / denom;
    acf.push_back(std::clamp(corr, -1.0, 1.0));
  }

  auto& pane = studio.pane2();
  pane.grid(GridStyle::Major).x_label("lag").y_label("autocorrelation").y_limits({-1.0, 1.0});
  auto& spikes = pane.spike(lags, acf, 0.0);
  spikes.width(options.trace_width).label("acf");
  pane.rule_y(0.0, "zero");
  if (options.show_confidence_band) {
    const double z = detail::z_for_confidence_level(options.confidence_level);
    const double band = z / std::sqrt(static_cast<double>(n));
    pane.rule_y(band, "+CI").rule_y(-band, "-CI");
  }
  return studio;
}

template <class Values>
Studio& pacfplot(Studio& studio, Values&& values, const PacfPlotOptions& options = {}) {
  const Series base = detail::flatten_f64(detail::to_series(std::forward<Values>(values)));
  const std::int64_t n = base.numel();
  if (n < 4) {
    throw std::invalid_argument("flash::pacfplot requires at least four points");
  }
  const std::int64_t max_lag = std::clamp<std::int64_t>(options.max_lag, 1, n - 1);
  const double global_mean = detail::scalar_value(base.mean());
  const Series centered = base.sub(global_mean);
  const double denom = detail::scalar_value(centered.mul(centered).sum());
  const bool valid_denom = std::isfinite(denom) && denom > 0.0;

  std::vector<double> acf(static_cast<std::size_t>(max_lag + 1), 0.0);
  acf[0] = 1.0;
  if (valid_denom) {
    for (const auto lag : std::views::iota(std::int64_t{1}, max_lag + 1)) {
      const Series idx_x = Series::arange(0, n - lag, 1, ScalarType::Int64);
      const Series idx_y = Series::arange(lag, n, 1, ScalarType::Int64);
      const double numerator = detail::scalar_value(centered.take(idx_x).mul(centered.take(idx_y)).sum());
      acf[static_cast<std::size_t>(lag)] = std::clamp(numerator / denom, -1.0, 1.0);
    }
  }

  std::vector<double> pacf(static_cast<std::size_t>(max_lag + 1), 0.0);
  pacf[0] = 1.0;
  std::vector<double> phi_prev(static_cast<std::size_t>(max_lag + 1), 0.0);
  std::vector<double> phi_curr(static_cast<std::size_t>(max_lag + 1), 0.0);
  for (const auto k : std::views::iota(std::int64_t{1}, max_lag + 1)) {
    double num = acf[static_cast<std::size_t>(k)];
    double den = 1.0;
    for (const auto j : std::views::iota(std::int64_t{1}, k)) {
      num -= phi_prev[static_cast<std::size_t>(j)] * acf[static_cast<std::size_t>(k - j)];
      den -= phi_prev[static_cast<std::size_t>(j)] * acf[static_cast<std::size_t>(j)];
    }
    const double alpha = (std::abs(den) > 1e-12) ? (num / den) : 0.0;
    std::fill(phi_curr.begin(), phi_curr.end(), 0.0);
    phi_curr[static_cast<std::size_t>(k)] = alpha;
    for (const auto j : std::views::iota(std::int64_t{1}, k)) {
      phi_curr[static_cast<std::size_t>(j)] =
          phi_prev[static_cast<std::size_t>(j)] - alpha * phi_prev[static_cast<std::size_t>(k - j)];
    }
    pacf[static_cast<std::size_t>(k)] = std::clamp(alpha, -1.0, 1.0);
    phi_prev.swap(phi_curr);
  }

  const std::int64_t lag_start = options.include_zero_lag ? 0 : 1;
  std::vector<double> lags;
  std::vector<double> shown_pacf;
  lags.reserve(static_cast<std::size_t>(max_lag - lag_start + 1));
  shown_pacf.reserve(static_cast<std::size_t>(max_lag - lag_start + 1));
  for (const auto lag : std::views::iota(lag_start, max_lag + 1)) {
    lags.push_back(static_cast<double>(lag));
    shown_pacf.push_back(pacf[static_cast<std::size_t>(lag)]);
  }

  auto& pane = studio.pane2();
  pane.grid(GridStyle::Major).x_label("lag").y_label("partial autocorrelation").y_limits({-1.0, 1.0});
  auto& spikes = pane.spike(lags, shown_pacf, 0.0);
  spikes.width(options.trace_width).label("pacf");
  pane.rule_y(0.0, "zero");
  if (options.show_confidence_band) {
    const double z = detail::z_for_confidence_level(options.confidence_level);
    const double band = z / std::sqrt(static_cast<double>(n));
    pane.rule_y(band, "+CI").rule_y(-band, "-CI");
  }
  return studio;
}

template <class X, class Y>
Studio& ccfplot(Studio& studio, X&& x, Y&& y, const CcfPlotOptions& options = {}) {
  const Series sx = detail::flatten_f64(detail::to_series(std::forward<X>(x)));
  const Series sy = detail::flatten_f64(detail::to_series(std::forward<Y>(y)));
  if (sx.numel() != sy.numel() || sx.numel() < 3) {
    throw std::invalid_argument("flash::ccfplot requires aligned x/y with at least three points");
  }
  const std::int64_t n = sx.numel();
  const std::int64_t max_lag = std::clamp<std::int64_t>(options.max_lag, 1, n - 1);
  const Series x_centered = sx.sub(detail::scalar_value(sx.mean()));
  const Series y_centered = sy.sub(detail::scalar_value(sy.mean()));
  const double x_energy = detail::scalar_value(x_centered.mul(x_centered).sum());
  const double y_energy = detail::scalar_value(y_centered.mul(y_centered).sum());
  const double denom = std::sqrt(std::max(0.0, x_energy) * std::max(0.0, y_energy));
  const bool valid_denom = std::isfinite(denom) && denom > 0.0;

  std::vector<double> lags;
  std::vector<double> ccf;
  lags.reserve(static_cast<std::size_t>(2 * max_lag + 1));
  ccf.reserve(static_cast<std::size_t>(2 * max_lag + 1));
  for (const auto lag : std::views::iota(-max_lag, max_lag + 1)) {
    lags.push_back(static_cast<double>(lag));
    if (!valid_denom) {
      ccf.push_back(0.0);
      continue;
    }
    const std::int64_t lead = std::max<std::int64_t>(lag, 0);
    const std::int64_t tail = std::max<std::int64_t>(-lag, 0);
    const std::int64_t usable = n - lead - tail;
    if (usable <= 0) {
      ccf.push_back(0.0);
      continue;
    }
    const Series idx_x = Series::arange(tail, tail + usable, 1, ScalarType::Int64);
    const Series idx_y = Series::arange(lead, lead + usable, 1, ScalarType::Int64);
    const double numerator = detail::scalar_value(x_centered.take(idx_x).mul(y_centered.take(idx_y)).sum());
    ccf.push_back(std::clamp(numerator / denom, -1.0, 1.0));
  }

  auto& pane = studio.pane2();
  pane.grid(GridStyle::Major).x_label("lag").y_label("cross-correlation").y_limits({-1.0, 1.0});
  auto& spikes = pane.spike(lags, ccf, 0.0);
  spikes.width(options.trace_width).label("ccf");
  pane.rule_y(0.0, "zero");
  if (options.show_confidence_band) {
    const double z = detail::z_for_confidence_level(options.confidence_level);
    const double band = z / std::sqrt(static_cast<double>(n));
    pane.rule_y(band, "+CI").rule_y(-band, "-CI");
  }
  return studio;
}

template <class Values>
Studio& actplot(Studio& studio, Values&& values, const AutocorrelationPlotOptions& options = {}) {
  AcfPlotOptions acf_options;
  acf_options.max_lag = options.max_lag;
  acf_options.include_zero_lag = false;
  acf_options.show_confidence_band = options.show_confidence_band;
  acf_options.confidence_level = options.confidence_level;
  acf_options.trace_width = options.trace_width;
  return acfplot(studio, std::forward<Values>(values), acf_options);
}

template <class X, class Y>
Studio& resid_acfplot(Studio& studio, X&& x, Y&& y, const ResidAcfPlotOptions& options = {}) {
  const auto [sx, sy] = detail::finite_xy(
      detail::to_series(std::forward<X>(x)),
      detail::to_series(std::forward<Y>(y)));
  if (sx.numel() < 3) {
    throw std::invalid_argument("flash::resid_acfplot requires at least three finite x/y pairs");
  }
  const std::vector<double> vx = detail::to_f64_vector(sx);
  std::vector<double> vy = detail::to_f64_vector(sy);
  const auto pred = detail::regression_predict(
      vx,
      vy,
      vx,
      std::max<std::int64_t>(1, options.reg.order),
      options.reg.robust,
      options.reg.lowess,
      options.reg.lowess_frac);
  std::transform(vy.begin(), vy.end(), pred.begin(), vy.begin(), [](const double y_i, const double p_i) {
    return y_i - p_i;
  });
  return acfplot(
      studio,
      pluss::make_series(vy, {static_cast<std::int64_t>(vy.size())}),
      options.acf);
}

template <class Values>
Studio& bootstrapplot(Studio& studio, Values&& values, const BootstrapPlotOptions& options = {}) {
  const Series base = detail::flatten_f64(detail::to_series(std::forward<Values>(values)));
  const std::int64_t n = base.numel();
  if (n < 2) {
    throw std::invalid_argument("flash::bootstrapplot requires at least two points");
  }
  const std::int64_t draw_size = options.sample_size > 0 ? options.sample_size : n;
  const std::int64_t samples = std::max<std::int64_t>(8, options.samples);
  std::vector<double> bootstrap_stats;
  bootstrap_stats.reserve(static_cast<std::size_t>(samples));
  for (const auto i : std::views::iota(std::int64_t{0}, samples)) {
    const auto idx = pluss::randint(0, n, {draw_size}, ScalarType::Int64, options.seed + static_cast<std::uint64_t>(i + 1));
    const Series sample = base.take(idx);
    double stat = 0.0;
    switch (options.statistic) {
      case BootstrapStatistic::Mean:
        stat = detail::scalar_value(sample.mean());
        break;
      case BootstrapStatistic::Std:
        stat = detail::scalar_value(sample.std(false));
        break;
      case BootstrapStatistic::Median: {
        auto values_vec = detail::to_f64_vector(sample);
        std::sort(values_vec.begin(), values_vec.end());
        stat = detail::quantile_sorted(values_vec, 0.5);
        break;
      }
    }
    bootstrap_stats.push_back(stat);
  }

  double observed = 0.0;
  switch (options.statistic) {
    case BootstrapStatistic::Mean:
      observed = detail::scalar_value(base.mean());
      break;
    case BootstrapStatistic::Std:
      observed = detail::scalar_value(base.std(false));
      break;
    case BootstrapStatistic::Median: {
      auto values_vec = detail::to_f64_vector(base);
      std::sort(values_vec.begin(), values_vec.end());
      observed = detail::quantile_sorted(values_vec, 0.5);
      break;
    }
  }

  auto& pane = studio.pane2();
  pane.grid(GridStyle::Major).x_label("bootstrap statistic").y_label("count");
  auto& trace = pane.bucket(bootstrap_stats, std::max(4, options.bins));
  trace.width(options.trace_width).label("bootstrap");
  pane.rule_x(observed, "observed");
  return studio;
}

inline Studio& summaryplot(
    Studio& studio,
    const std::vector<Series>& columns,
    const std::vector<std::string>& labels,
    const SummaryOptions& options = {}) {
  if (columns.empty()) {
    throw std::invalid_argument("flash::summaryplot requires at least one column");
  }
  if (!labels.empty() && labels.size() != columns.size()) {
    throw std::invalid_argument("flash::summaryplot: labels size must match columns size");
  }

  std::vector<std::string> names = labels;
  if (names.empty()) {
    names.reserve(columns.size());
    for (const auto i : std::views::iota(std::size_t{0}, columns.size())) {
      names.push_back("x" + std::to_string(i + 1));
    }
  }

  std::vector<std::vector<double>> values;
  values.reserve(columns.size());
  for (const auto& column : columns) {
    values.push_back(detail::to_f64_vector(detail::flatten_f64(column)));
  }
  const std::int64_t ncases = static_cast<std::int64_t>(values.front().size());
  for (const auto& column_values : values) {
    if (static_cast<std::int64_t>(column_values.size()) != ncases) {
      throw std::invalid_argument("flash::summaryplot requires equal-length columns");
    }
  }

  const std::int64_t max_cases = options.max_cases > 0 ? options.max_cases : 10;
  const std::int64_t shown_cases = std::min(ncases, max_cases);

  const auto format_cell = [&](double value) -> std::string {
    if (!std::isfinite(value)) {
      return "nan";
    }
    std::ostringstream cell;
    cell.setf(std::ios::fixed);
    cell << std::setprecision(std::max(0, options.precision)) << value;
    return cell.str();
  };

  const auto build_table_lines = [&](std::size_t col_begin, std::size_t col_end) {
    std::vector<std::string> lines;
    if (col_begin >= col_end || col_end > values.size()) {
      return lines;
    }
    auto push_line = [&](std::ostringstream& row) {
      lines.push_back(row.str());
      row.str("");
      row.clear();
    };

    if (options.transpose) {
      if (options.include_header) {
        std::ostringstream row;
        row << std::left << std::setw(18) << "feature";
        for (const auto case_idx : std::views::iota(std::int64_t{0}, shown_cases)) {
          row << std::right << std::setw(14) << ("case_" + std::to_string(case_idx));
        }
        push_line(row);
      }
      for (const auto col : std::views::iota(col_begin, col_end)) {
        std::ostringstream row;
        row << std::left << std::setw(18) << names[col];
        for (const auto case_idx : std::views::iota(std::int64_t{0}, shown_cases)) {
          row << std::right << std::setw(14) << format_cell(values[col][static_cast<std::size_t>(case_idx)]);
        }
        push_line(row);
      }
    } else {
      if (options.include_header) {
        std::ostringstream row;
        if (options.include_case_index) {
          row << std::left << std::setw(10) << "case";
        }
        for (const auto col : std::views::iota(col_begin, col_end)) {
          row << std::right << std::setw(14) << names[col];
        }
        push_line(row);
      }
      for (const auto case_idx : std::views::iota(std::int64_t{0}, shown_cases)) {
        std::ostringstream row;
        if (options.include_case_index) {
          row << std::left << std::setw(10) << case_idx;
        }
        for (const auto col : std::views::iota(col_begin, col_end)) {
          row << std::right << std::setw(14) << format_cell(values[col][static_cast<std::size_t>(case_idx)]);
        }
        push_line(row);
      }
    }
    if (ncases > shown_cases) {
      std::ostringstream row;
      row << "... (" << (ncases - shown_cases) << " more cases)";
      push_line(row);
    }
    return lines;
  };

  const std::size_t feature_count = names.size();
  const std::size_t features_per_table = static_cast<std::size_t>(
      std::max<std::int64_t>(1, options.features_per_table > 0 ? options.features_per_table
                                                                : static_cast<std::int64_t>(feature_count)));
  const std::size_t tables_per_row = static_cast<std::size_t>(std::max<std::int64_t>(1, options.tables_per_row));
  const std::size_t gap_spaces = static_cast<std::size_t>(std::max<std::int64_t>(1, options.table_gap_spaces));

  std::vector<std::vector<std::string>> tables;
  tables.reserve((feature_count + features_per_table - 1) / features_per_table);
  for (std::size_t begin = 0; begin < feature_count; begin += features_per_table) {
    const std::size_t end = std::min(feature_count, begin + features_per_table);
    tables.push_back(build_table_lines(begin, end));
  }

  std::ostringstream out;
  if (tables.size() <= 1 || tables_per_row <= 1) {
    for (const auto table_idx : std::views::iota(std::size_t{0}, tables.size())) {
      for (const auto& line : tables[table_idx]) {
        out << line << "\n";
      }
      if (table_idx + 1 < tables.size()) {
        out << "\n";
      }
    }
  } else {
    for (std::size_t block_begin = 0; block_begin < tables.size(); block_begin += tables_per_row) {
      const std::size_t block_end = std::min(tables.size(), block_begin + tables_per_row);
      std::vector<std::size_t> widths(block_end - block_begin, 0);
      std::size_t max_lines = 0;
      for (const auto block_idx : std::views::iota(block_begin, block_end)) {
        const auto local = block_idx - block_begin;
        max_lines = std::max(max_lines, tables[block_idx].size());
        for (const auto& line : tables[block_idx]) {
          widths[local] = std::max(widths[local], line.size());
        }
      }
      for (const auto line_idx : std::views::iota(std::size_t{0}, max_lines)) {
        for (const auto block_idx : std::views::iota(block_begin, block_end)) {
          const auto local = block_idx - block_begin;
          const auto& table = tables[block_idx];
          const std::string_view cell =
              line_idx < table.size() ? std::string_view(table[line_idx]) : std::string_view{};
          out << cell;
          if (cell.size() < widths[local]) {
            out << std::string(widths[local] - cell.size(), ' ');
          }
          if (block_idx + 1 < block_end) {
            out << std::string(gap_spaces, ' ');
          }
        }
        out << "\n";
      }
      if (block_end < tables.size()) {
        out << "\n";
      }
    }
  }

  auto& pane = studio.pane2(options.title);
  pane.grid(GridStyle::None)
      .legend(KeyPlace::None)
      .x_label("")
      .y_label("")
      .tick_font_scale(0.01)
      .axis_label_font_scale(0.01)
      .caption(out.str());
  return studio;
}

template <class X, class Y>
Studio& jointplot(Studio& studio, X&& x, Y&& y, std::string_view kind = "scatter") {
  JointPlotOptions options;
  if (kind == "reg") {
    options.kind = JointPlotKind::Reg;
  } else if (kind == "hex") {
    options.kind = JointPlotKind::Hex;
  } else if (kind == "kde") {
    options.kind = JointPlotKind::Kde;
    options.marginal = JointMarginalKind::Kde;
  } else {
    options.kind = JointPlotKind::Scatter;
  }
  return jointplot(studio, std::forward<X>(x), std::forward<Y>(y), options);
}

template <class X, class Y>
Studio& jointplot(Studio& studio, X&& x, Y&& y, const JointPlotOptions& options) {
  const Series sx = detail::to_series(std::forward<X>(x));
  const Series sy = detail::to_series(std::forward<Y>(y));
  if (sx.numel() != sy.numel()) {
    throw std::invalid_argument("flash::jointplot requires x/y with identical lengths");
  }
  studio.mosaic(4, 4, options.grid_gap_x, options.grid_gap_y);

  Pane& joint = studio.pane2();
  joint.place(1, 0, 3, 3).grid(options.show_joint_grid ? GridStyle::Major : GridStyle::None);
  if (options.kind == JointPlotKind::Reg) {
    regplot(joint, sx, sy, options.reg);
  } else if (options.kind == JointPlotKind::Hex) {
    const auto grid = detail::hist2d_grid(sx, sy, options.hex_bins_x, options.hex_bins_y);
    HeatmapOptions heat_opts;
    heat_opts.scale_guide = true;
    heat_opts.annotate_levels = false;
    heatmap(joint, grid.x, grid.y, grid.z, heat_opts);
  } else if (options.kind == JointPlotKind::Kde) {
    auto grid = detail::hist2d_grid(sx, sy, options.kde_bins_x, options.kde_bins_y);
    grid.z = detail::smooth_grid(grid.z, std::max<std::int64_t>(0, options.kde_smooth_passes));
    if (options.kde_fill) {
      HeatmapOptions heat_opts;
      heat_opts.scale_guide = true;
      heatmap(joint, grid.x, grid.y, grid.z, heat_opts);
    }
    joint.contour(
        grid.x,
        grid.y,
        grid.z,
        static_cast<int>(std::max<std::int64_t>(4, options.kde_levels)))
        .width(std::max(0.85F, options.reg.fit_trace_width * 0.9F))
        .label("kde");
  } else {
    joint.cloud(sx, sy, std::array<double, 1>{std::max(1.0, options.reg.marker_area)});
  }

  Pane& top = studio.pane2();
  top.place(0, 0, 1, 3).grid(options.show_marginal_grid ? GridStyle::Major : GridStyle::None).y_label("");
  if (options.marginal == JointMarginalKind::Kde) {
    kdeplot(top, sx, std::max<std::int64_t>(16, options.marginal_kde_points));
  } else {
    top.bucket(sx, static_cast<int>(std::max<std::int64_t>(4, options.marginal_bins)));
  }

  Pane& right = studio.pane2();
  right.place(1, 3, 3, 1).grid(options.show_marginal_grid ? GridStyle::Major : GridStyle::None).x_label("");
  if (options.marginal == JointMarginalKind::Kde) {
    const auto [kde_x, kde_y] = detail::gaussian_kde(detail::flatten_f64(sy), std::max<std::int64_t>(16, options.marginal_kde_points));
    right.trend(kde_y, kde_x);
  } else {
    right.bucket(sy, static_cast<int>(std::max<std::int64_t>(4, options.marginal_bins)));
  }
  return studio;
}

[[nodiscard]] inline PaletteRamp color_palette(std::string_view name = "ocean") {
  if (name == "deep" || name == "muted" || name == "pastel" || name == "bright" || name == "dark"
      || name == "colorblind") {
    return flash_palette(name);
  }
  if (name == "thermal") return PaletteRamp::thermal();
  if (name == "lagoon") return PaletteRamp::lagoon();
  if (name == "meadow") return PaletteRamp::meadow();
  if (name == "neutral") return PaletteRamp::neutral();
  return PaletteRamp::ocean();
}

[[nodiscard]] inline PaletteRamp blend_palette(std::vector<Rgba> colors, std::string name = "blend") {
  if (colors.empty()) {
    colors = {Rgba::from_u8(42, 95, 153), Rgba::from_u8(229, 107, 111)};
  }
  return PaletteRamp(std::move(name), std::move(colors));
}

[[nodiscard]] inline PaletteRamp hls_palette(std::int64_t n = 6, double saturation = 0.68, double lightness = 0.52) {
  const std::int64_t count = std::max<std::int64_t>(n, 2);
  std::vector<Rgba> stops;
  stops.reserve(static_cast<std::size_t>(count));
  for (std::int64_t i = 0; i < count; ++i) {
    const double hue = static_cast<double>(i) / static_cast<double>(count);
    stops.push_back(detail::hsl_to_rgb(hue, saturation, lightness));
  }
  return PaletteRamp("hls", std::move(stops));
}

[[nodiscard]] inline PaletteRamp husl_palette(std::int64_t n = 6) {
  return hls_palette(n, 0.74, 0.55);
}

[[nodiscard]] inline PaletteRamp cubehelix_palette(std::int64_t n = 6) {
  return hls_palette(n, 0.58, 0.46);
}

[[nodiscard]] inline PaletteRamp light_palette(Rgba color, std::int64_t n = 6) {
  const std::int64_t count = std::max<std::int64_t>(n, 2);
  std::vector<Rgba> stops;
  stops.reserve(static_cast<std::size_t>(count));
  const Rgba white{1.0, 1.0, 1.0, 1.0};
  for (std::int64_t i = 0; i < count; ++i) {
    stops.push_back(detail::mix(white, color, static_cast<double>(i) / static_cast<double>(count - 1)));
  }
  return PaletteRamp("light", std::move(stops));
}

[[nodiscard]] inline PaletteRamp dark_palette(Rgba color, std::int64_t n = 6) {
  const std::int64_t count = std::max<std::int64_t>(n, 2);
  std::vector<Rgba> stops;
  stops.reserve(static_cast<std::size_t>(count));
  const Rgba black{0.0, 0.0, 0.0, 1.0};
  for (std::int64_t i = 0; i < count; ++i) {
    stops.push_back(detail::mix(black, color, static_cast<double>(i) / static_cast<double>(count - 1)));
  }
  return PaletteRamp("dark", std::move(stops));
}

[[nodiscard]] inline PaletteRamp diverging_palette(Rgba left, Rgba right, std::int64_t n = 7) {
  const std::int64_t count = std::max<std::int64_t>(n, 3);
  std::vector<Rgba> stops;
  stops.reserve(static_cast<std::size_t>(count));
  const std::int64_t mid = count / 2;
  for (std::int64_t i = 0; i < count; ++i) {
    if (i <= mid) {
      const double t = mid == 0 ? 1.0 : static_cast<double>(i) / static_cast<double>(mid);
      stops.push_back(detail::mix(left, Rgba{1.0, 1.0, 1.0, 1.0}, t));
    } else {
      const double t = static_cast<double>(i - mid) / static_cast<double>(count - 1 - mid);
      stops.push_back(detail::mix(Rgba{1.0, 1.0, 1.0, 1.0}, right, t));
    }
  }
  return PaletteRamp("diverging", std::move(stops));
}

[[nodiscard]] inline PaletteRamp mpl_palette(std::string_view = "viridis") {
  return PaletteRamp::lagoon();
}

[[nodiscard]] inline PaletteRamp xkcd_palette() {
  return blend_palette(
      {Rgba::from_u8(49, 117, 187), Rgba::from_u8(255, 160, 64), Rgba::from_u8(89, 182, 116)},
      "xkcd");
}

[[nodiscard]] inline PaletteRamp crayon_palette() {
  return blend_palette(
      {Rgba::from_u8(239, 71, 111), Rgba::from_u8(255, 209, 102), Rgba::from_u8(6, 214, 160)},
      "crayon");
}

[[nodiscard]] inline Note get_dataset_names() {
  return pluss::make_note({"tips", "flights"});
}

[[nodiscard]] inline std::optional<Dataset> load_dataset(std::string_view name) {
  Dataset data;
  if (name == "tips") {
    data.numeric_names = pluss::make_note({"total_bill", "tip", "size"});
    data.numeric_values = {
        pluss::make_series(std::vector<double>{16.99, 10.34, 21.01, 23.68}, {4}),
        pluss::make_series(std::vector<double>{1.01, 1.66, 3.5, 3.31}, {4}),
        pluss::make_series(std::vector<double>{2.0, 3.0, 3.0, 2.0}, {4}),
    };
    data.note_names = pluss::make_note({"day"});
    data.note_values = {
        pluss::make_note({"Sun", "Sun", "Sun", "Sun"}),
    };
    return data;
  }
  if (name == "flights") {
    data.numeric_names = pluss::make_note({"month", "passengers"});
    data.numeric_values = {
        pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {6}),
        pluss::make_series(std::vector<double>{112, 118, 132, 129, 121, 135}, {6}),
    };
    data.note_names = pluss::make_note({"year"});
    data.note_values = {
        pluss::make_note({"1949", "1949", "1949", "1949", "1949", "1949"}),
    };
    return data;
  }
  return std::nullopt;
}

} // namespace pluss::pl::flash
