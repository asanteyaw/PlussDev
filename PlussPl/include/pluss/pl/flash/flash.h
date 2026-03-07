#pragma once

#include "pluss/coden/note_factory.h"
#include "pluss/codeh/codeh.h"
#include "pluss/pl/studio.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
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
  for (const double value : samples) {
    mean += value;
  }
  mean /= static_cast<double>(samples.size());
  double variance = 0.0;
  for (const double value : samples) {
    const double delta = value - mean;
    variance += delta * delta;
  }
  variance /= static_cast<double>(samples.size());
  const double sigma = std::sqrt(std::max(variance, 1e-18));
  const double span = std::max(high - low, 1e-6);
  const double bandwidth =
      sigma > 0.0 ? (1.06 * sigma * std::pow(static_cast<double>(samples.size()), -0.2)) : (span / 24.0);
  const double h = std::max(bandwidth, 1e-6);
  const double inv_norm = 1.0 / (std::sqrt(2.0 * 3.14159265358979323846) * h * static_cast<double>(samples.size()));

  for (std::size_t i = 0; i < x.size(); ++i) {
    double acc = 0.0;
    for (const double sample : samples) {
      const double u = (x[i] - sample) / h;
      acc += std::exp(-0.5 * u * u);
    }
    y[i] = acc * inv_norm;
  }

  return {std::move(x), std::move(y)};
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

inline Studio& set(Studio& studio, Theme theme = Theme::paper()) {
  studio.theme(std::move(theme));
  return studio;
}

inline Studio& set_theme(Studio& studio, Theme theme = Theme::paper()) {
  return set(studio, std::move(theme));
}

inline Pane& set_style(Pane& pane, GridStyle style = GridStyle::Major) {
  return pane.grid(style);
}

inline Pane& axes_style(Pane& pane, GridStyle style = GridStyle::Major) {
  return set_style(pane, style);
}

inline Studio& set_context(
    Studio& studio,
    double width,
    double height,
    double tick_font_scale = 1.0,
    double axis_label_font_scale = 1.0) {
  studio.size(width, height);
  for (const Pane& pane_const : studio.panes()) {
    auto& pane = const_cast<Pane&>(pane_const);
    detail::apply_tick_font_scale(pane, tick_font_scale, axis_label_font_scale);
  }
  return studio;
}

inline Studio& plotting_context(
    Studio& studio,
    double width,
    double height,
    double tick_font_scale = 1.0,
    double axis_label_font_scale = 1.0) {
  return set_context(studio, width, height, tick_font_scale, axis_label_font_scale);
}

inline Studio& tight_layout(Studio& studio, double padding = 6.0) {
  return studio.tight_layout(true, padding);
}

inline Pane& despine(Pane& pane, bool = true) noexcept {
  return pane;
}

inline Pane& move_legend(Pane& pane, KeyPlace place) noexcept {
  return pane.legend(place);
}

inline Studio& move_legend(Studio& studio, KeyDock dock) noexcept {
  return studio.legend(dock);
}

inline Pane& legend(Pane& pane, KeyPlace place) noexcept {
  return pane.legend(place);
}

inline Studio& legend(Studio& studio, KeyDock dock) noexcept {
  return studio.legend(dock);
}

inline Pane& legend_frame(Pane& pane, KeyFrame frame) noexcept {
  return pane.legend_frame(frame);
}

inline Studio& legend_frame(Studio& studio, KeyFrame frame) noexcept {
  return studio.legend_frame(frame);
}

inline bool show(Studio& studio,
                 std::string_view title = {},
                 std::int64_t width = 1280,
                 std::int64_t height = 820,
                 bool launch = true) {
  return studio.show(title, width, height, launch);
}

inline void save_png(Studio& studio, std::string_view path, std::int64_t dpi = 144) {
  studio.save_png(path, dpi);
}

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

inline Trace& violinplot(Pane& pane, const Note& categories, const Series& values) {
  const auto [encoded, labels] = detail::encode_categories(categories);
  pane.x_categories(labels);
  return pane.violin(pluss::make_series(encoded, {static_cast<std::int64_t>(encoded.size())}), values);
}

template <class Group, class Values>
  requires(!std::same_as<std::remove_cvref_t<Group>, Note>)
Trace& violinplot(Pane& pane, Group&& group, Values&& values) {
  return pane.violin(detail::to_series(std::forward<Group>(group)),
                     detail::to_series(std::forward<Values>(values)));
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
Trace& regplot(Pane& pane, X&& x, Y&& y) {
  const Series sx = detail::flatten_f64(detail::to_series(std::forward<X>(x)));
  const Series sy = detail::flatten_f64(detail::to_series(std::forward<Y>(y)));
  pane.cloud(sx, sy);
  const auto [slope, intercept] = detail::linear_fit(sx, sy);
  const std::vector<double> vx = detail::to_f64_vector(sx);
  const auto [xmin, xmax] = std::minmax_element(vx.begin(), vx.end());
  const std::vector<double> fit_x{*xmin, *xmax};
  const std::vector<double> fit_y{intercept + slope * (*xmin), intercept + slope * (*xmax)};
  return pane.trend(fit_x, fit_y);
}

template <class X, class Y>
Trace& lmplot(Studio& studio, X&& x, Y&& y, std::string title = {}) {
  Pane& pane = studio.pane2(std::move(title));
  return regplot(pane, std::forward<X>(x), std::forward<Y>(y));
}

template <class X, class Y>
Trace& residplot(Pane& pane, X&& x, Y&& y) {
  const Series sx = detail::flatten_f64(detail::to_series(std::forward<X>(x)));
  const Series sy = detail::flatten_f64(detail::to_series(std::forward<Y>(y)));
  const auto [slope, intercept] = detail::linear_fit(sx, sy);
  std::vector<double> vx = detail::to_f64_vector(sx);
  std::vector<double> vy = detail::to_f64_vector(sy);
  for (std::size_t i = 0; i < vy.size(); ++i) {
    vy[i] -= intercept + slope * vx[i];
  }
  return pane.cloud(pluss::make_series(vx, {static_cast<std::int64_t>(vx.size())}),
                    pluss::make_series(vy, {static_cast<std::int64_t>(vy.size())}));
}

template <class X, class Y, class Z>
Trace& heatmap(Pane& pane, X&& x, Y&& y, Z&& z) {
  return pane.fieldmap(detail::to_series(std::forward<X>(x)),
                       detail::to_series(std::forward<Y>(y)),
                       detail::to_series(std::forward<Z>(z)));
}

template <class X, class Y, class Z, class C>
Trace& heatmap(Pane& pane, X&& x, Y&& y, Z&& z, C&& c) {
  return pane.fieldmap(detail::to_series(std::forward<X>(x)),
                       detail::to_series(std::forward<Y>(y)),
                       detail::to_series(std::forward<Z>(z)),
                       detail::to_series(std::forward<C>(c)));
}

template <class X, class Y, class Z>
Trace& clustermap(Pane& pane, X&& x, Y&& y, Z&& z) {
  return heatmap(pane, std::forward<X>(x), std::forward<Y>(y), std::forward<Z>(z));
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

enum class PairPlotTriangle : std::uint8_t { Full, Lower, Upper };

struct PairPlotOptions {
  PairPlotTriangle triangle{PairPlotTriangle::Full};
  bool monochrome{true};
  std::optional<int> auto_tick_decimals{std::nullopt};
  std::int64_t diag_bins{14};
  double grid_gap_x{8.0};
  double grid_gap_y{8.0};
  double tick_font_scale{0.9};
  double axis_label_font_scale{0.9};
  float diag_trace_width{0.75F};
  float scatter_trace_width{0.45F};
  double marker_area_scale{380.0};
  double marker_area_min{7.0};
  double marker_area_max{18.0};
  bool show_grid{false};
};

inline Studio& pairplot(Studio& studio, const std::vector<Series>& columns, const PairPlotOptions& options = {}) {
  if (columns.empty()) {
    throw std::invalid_argument("flash::pairplot requires at least one series");
  }
  const std::int64_t n = static_cast<std::int64_t>(columns.size());
  studio.mosaic(n, n, options.grid_gap_x, options.grid_gap_y);
  const auto colors = detail::categorical_colors(n);
  for (const auto row : std::views::iota(std::int64_t{0}, n)) {
    for (const auto col : std::views::iota(std::int64_t{0}, n)) {
      Pane& pane = studio.pane2();
      pane.place(row, col)
          .grid(options.show_grid ? GridStyle::Major : GridStyle::None)
          .tick_font_scale(options.tick_font_scale)
          .axis_label_font_scale(options.axis_label_font_scale);
      if (options.auto_tick_decimals.has_value()) {
        pane.x_auto_tick_decimals(*options.auto_tick_decimals);
        pane.y_auto_tick_decimals(*options.auto_tick_decimals);
      }
      const bool keep_cell =
          options.triangle == PairPlotTriangle::Full ||
          (options.triangle == PairPlotTriangle::Lower && row >= col) ||
          (options.triangle == PairPlotTriangle::Upper && row <= col);
      if (!keep_cell) {
        pane.hide();
        continue;
      }
      if (row != n - 1) {
        pane.x_label("");
      } else {
        pane.x_label("x" + std::to_string(col + 1));
      }
      if (col != 0) {
        pane.y_label("");
      } else {
        pane.y_label("x" + std::to_string(row + 1));
      }
      if (row == col) {
        auto& trace = pane.bucket(columns[static_cast<std::size_t>(row)], static_cast<int>(std::max<std::int64_t>(1, options.diag_bins)));
        trace.width(options.diag_trace_width);
        if (!options.monochrome && !colors.empty()) {
          trace.stroke(colors[static_cast<std::size_t>(row) % colors.size()]);
        }
      } else {
        const std::int64_t points = std::max<std::int64_t>(
            1,
            std::min(
                columns[static_cast<std::size_t>(col)].numel(),
                columns[static_cast<std::size_t>(row)].numel()));
        const double marker_area = std::clamp(
            options.marker_area_scale / std::sqrt(static_cast<double>(points) * static_cast<double>(std::max<std::int64_t>(n, 1))),
            options.marker_area_min,
            options.marker_area_max);
        auto& trace = pane.cloud(
                columns[static_cast<std::size_t>(col)],
                columns[static_cast<std::size_t>(row)],
                std::array<double, 1>{marker_area});
        trace.width(options.scatter_trace_width);
        if (!options.monochrome && !colors.empty()) {
          trace.stroke(colors[static_cast<std::size_t>(row + col) % colors.size()]);
        }
      }
    }
  }
  return studio;
}

template <class X, class Y>
Studio& jointplot(Studio& studio, X&& x, Y&& y, std::string_view kind = "scatter") {
  const Series sx = detail::to_series(std::forward<X>(x));
  const Series sy = detail::to_series(std::forward<Y>(y));
  studio.mosaic(4, 4, 8.0, 8.0);

  Pane& joint = studio.pane2();
  joint.place(1, 0, 3, 3).grid(GridStyle::Major);
  relplot(joint, sx, sy, kind);

  Pane& top = studio.pane2();
  top.place(0, 0, 1, 3).grid(GridStyle::None).y_label("");
  top.bucket(sx, 18);

  Pane& right = studio.pane2();
  right.place(1, 3, 3, 1).grid(GridStyle::None).x_label("");
  right.bucket(sy, 18);
  return studio;
}

[[nodiscard]] inline PaletteRamp color_palette(std::string_view name = "ocean") {
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
