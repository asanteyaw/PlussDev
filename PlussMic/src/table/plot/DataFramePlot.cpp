#include "table/plot/DataFramePlot.h"

#include <cstdlib>
#include <limits>
#include <string>
#include <vector>

#include "CodeM/core/Errors.h"

namespace pluss::table {

namespace {

constexpr std::string_view kDefaultPlotTheme = "paper";
constexpr std::string_view kDefaultPlotFont = "IBM Plex Sans";

std::vector<double> positional_plot_x(std::int64_t nrows) {
  std::vector<double> out(static_cast<std::size_t>(nrows), 0.0);
  for (std::int64_t i = 0; i < nrows; ++i) {
    out[static_cast<std::size_t>(i)] = static_cast<double>(i);
  }
  return out;
}

std::vector<double> thread_to_plot_numeric_values(const Thread& t) {
  std::vector<double> out;
  switch (t.kind()) {
    case ThreadKind::Series: {
      out = t.as_series().to(pluss::codem::ScalarType::Float64).to_vec<double>();
      break;
    }
    case ThreadKind::Timeline: {
      const auto ticks = t.as_timeline().to_epoch_nanos_vector();
      out.reserve(ticks.size());
      for (const auto tick : ticks) {
        out.push_back(static_cast<double>(tick));
      }
      break;
    }
    case ThreadKind::TimeIndex: {
      const auto ticks = t.as_timeindex().timeline().to_epoch_nanos_vector();
      out.reserve(ticks.size());
      for (const auto tick : ticks) {
        out.push_back(static_cast<double>(tick));
      }
      break;
    }
    case ThreadKind::Note: {
      const auto& note = t.as_note();
      out.reserve(static_cast<std::size_t>(note.numel()));
      note.for_each_flat([&](std::string_view s) {
        std::string v(s);
        char* end = nullptr;
        const double parsed = std::strtod(v.c_str(), &end);
        const bool valid = (end != nullptr) && (end != v.c_str()) && (*end == '\0');
        out.push_back(valid ? parsed : std::numeric_limits<double>::quiet_NaN());
      });
      break;
    }
    default:
      PLUSS_FAIL("DataFrame::plot: unsupported Thread kind for numeric plotting");
  }
  return out;
}

std::vector<std::string> thread_to_plot_labels(const Thread& t) {
  const auto note_like = (t.kind() == ThreadKind::Note) ? t : t.cast(ThreadCastTarget::Note);
  std::vector<std::string> out;
  const auto& note = note_like.as_note();
  out.reserve(static_cast<std::size_t>(note.numel()));
  note.for_each_flat([&](std::string_view s) { out.emplace_back(s); });
  return out;
}

void require_plot_size(const std::vector<double>& values, std::int64_t expected, std::string_view feature_name) {
  PLUSS_CHECK(static_cast<std::int64_t>(values.size()) == expected,
              "DataFrame::plot: feature '" + std::string(feature_name) + "' length mismatch with cases");
}

void require_plot_size(const std::vector<std::string>& values,
                       std::int64_t expected,
                       std::string_view feature_name,
                       std::string_view where) {
  PLUSS_CHECK(static_cast<std::int64_t>(values.size()) == expected,
              std::string(where) + ": feature '" + std::string(feature_name) + "' length mismatch with cases");
}

bool thread_prefers_labeled_axis(const Thread& t) {
  return t.kind() == ThreadKind::Note || t.kind() == ThreadKind::Timeline || t.kind() == ThreadKind::TimeIndex;
}

struct ResolvedPlotAxis {
  std::vector<double> values{};
  std::vector<std::string> labels{};
  std::string label{"case"};
};

ResolvedPlotAxis resolve_plot_axis(const DataFrame& frame,
                                   std::optional<std::string_view> x_feature,
                                   std::string_view where) {
  const auto nrows = frame.nrows();
  const auto axis_name = x_feature.has_value() ? std::string(*x_feature) : std::string("case");
  const auto axis_thread = x_feature.has_value() ? frame.features(*x_feature) : frame.cases();

  ResolvedPlotAxis axis;
  axis.label = axis_name;
  if (thread_prefers_labeled_axis(axis_thread)) {
    axis.values = positional_plot_x(nrows);
    axis.labels = thread_to_plot_labels(axis_thread);
    require_plot_size(axis.labels, nrows, axis_name, where);
    return axis;
  }

  axis.values = thread_to_plot_numeric_values(axis_thread);
  require_plot_size(axis.values, nrows, axis_name);
  return axis;
}

pluss::plotism::Figure make_dataframe_figure(bool show_legend, std::string legend_title = {}) {
  pluss::plotism::Figure fig;
  fig.theme(std::string(kDefaultPlotTheme)).font_family(std::string(kDefaultPlotFont));
  fig.legend(pluss::plotism::Legend{
      .visible = show_legend,
      .location = show_legend ? pluss::plotism::LegendLocation::OutsideRight : pluss::plotism::LegendLocation::Best,
      .title = std::move(legend_title),
  });
  return fig;
}

} // namespace

DataFramePlotAccessor::DataFramePlotAccessor(DataFrame frame) : frame_(std::move(frame)) {}

pluss::plotism::Figure DataFramePlotAccessor::line(
    std::string_view y_feature,
    std::optional<std::string_view> x_feature) const {
  return line({y_feature}, x_feature);
}

pluss::plotism::Figure DataFramePlotAccessor::line(
    std::initializer_list<std::string_view> y_features,
    std::optional<std::string_view> x_feature) const {
  PLUSS_CHECK(y_features.size() > 0, "DataFrame::plot().line: at least one y feature is required");

  const auto nrows = frame_.nrows();
  const auto axis = resolve_plot_axis(frame_, x_feature, "DataFrame::plot().line");
  auto fig = make_dataframe_figure(y_features.size() > 1, y_features.size() > 1 ? "feature" : "");
  fig.x_label(axis.label);
  for (const auto y_name : y_features) {
    const auto y_thread = frame_.features(y_name);
    auto y_values = thread_to_plot_numeric_values(y_thread);
    require_plot_size(y_values, nrows, y_name);
    fig.add_line(axis.values, y_values, std::string(y_name), axis.labels);
  }
  if (y_features.size() == 1) {
    fig.y_label(std::string(*y_features.begin()));
  } else {
    fig.y_label("value");
  }
  return fig;
}

pluss::plotism::Figure DataFramePlotAccessor::scatter(std::string_view x_feature,
                                                      std::string_view y_feature) const {
  const auto axis = resolve_plot_axis(frame_, x_feature, "DataFrame::plot().scatter");
  const auto y_values = thread_to_plot_numeric_values(frame_.features(y_feature));
  require_plot_size(y_values, frame_.nrows(), y_feature);

  auto fig = make_dataframe_figure(false);
  fig.add_scatter(axis.values, y_values, std::string(y_feature), axis.labels);
  fig.x_label(axis.label).y_label(std::string(y_feature));
  return fig;
}

pluss::plotism::Figure DataFramePlotAccessor::bar(
    std::string_view y_feature,
    std::optional<std::string_view> x_feature) const {
  const auto nrows = frame_.nrows();
  const auto y_values = thread_to_plot_numeric_values(frame_.features(y_feature));
  require_plot_size(y_values, nrows, y_feature);

  const auto axis = resolve_plot_axis(frame_, x_feature, "DataFrame::plot().bar");
  auto fig = make_dataframe_figure(false);
  fig.add_bar(axis.values, y_values, std::string(y_feature), axis.labels);
  fig.x_label(axis.label).y_label(std::string(y_feature));
  return fig;
}

pluss::plotism::Figure DataFramePlotAccessor::hist(std::string_view feature, std::int64_t bins) const {
  auto fig = make_dataframe_figure(false);
  fig.add_hist(thread_to_plot_numeric_values(frame_.features(feature)), bins, std::string(feature));
  fig.x_label(std::string(feature)).y_label("count");
  return fig;
}

DataFramePlotAccessor DataFrame::plot() const {
  return DataFramePlotAccessor(*this);
}

} // namespace pluss::table
