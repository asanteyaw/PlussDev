#include "table/plot/DataFramePlot.h"

#include <cstdlib>
#include <limits>
#include <string>
#include <vector>

#include "CodeM/core/Errors.h"

namespace pluss::table {

namespace {

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
  auto x_values = positional_plot_x(nrows);
  std::vector<std::string> x_labels{};
  std::string x_label = "case";

  if (x_feature.has_value()) {
    const auto x_thread = frame_.features(*x_feature);
    if (x_thread.kind() == ThreadKind::Note) {
      x_labels = thread_to_plot_labels(x_thread);
      PLUSS_CHECK(static_cast<std::int64_t>(x_labels.size()) == nrows,
                  "DataFrame::plot().line: x label feature length mismatch with cases");
    } else {
      x_values = thread_to_plot_numeric_values(x_thread);
      require_plot_size(x_values, nrows, *x_feature);
    }
    x_label = std::string(*x_feature);
  }

  pluss::plotism::Figure fig;
  fig.x_label(x_label);
  for (const auto y_name : y_features) {
    const auto y_thread = frame_.features(y_name);
    auto y_values = thread_to_plot_numeric_values(y_thread);
    require_plot_size(y_values, nrows, y_name);
    fig.add_line(x_values, y_values, std::string(y_name), x_labels);
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
  const auto x_values = thread_to_plot_numeric_values(frame_.features(x_feature));
  const auto y_values = thread_to_plot_numeric_values(frame_.features(y_feature));
  require_plot_size(x_values, frame_.nrows(), x_feature);
  require_plot_size(y_values, frame_.nrows(), y_feature);

  auto fig = pluss::plotism::scatter(x_values, y_values, std::string(y_feature));
  fig.x_label(std::string(x_feature)).y_label(std::string(y_feature));
  return fig;
}

pluss::plotism::Figure DataFramePlotAccessor::bar(
    std::string_view y_feature,
    std::optional<std::string_view> x_feature) const {
  const auto nrows = frame_.nrows();
  const auto y_values = thread_to_plot_numeric_values(frame_.features(y_feature));
  require_plot_size(y_values, nrows, y_feature);

  auto x_values = positional_plot_x(nrows);
  std::vector<std::string> x_labels{};
  std::string x_label = "case";
  if (x_feature.has_value()) {
    const auto x_thread = frame_.features(*x_feature);
    if (x_thread.kind() == ThreadKind::Note) {
      x_labels = thread_to_plot_labels(x_thread);
      PLUSS_CHECK(static_cast<std::int64_t>(x_labels.size()) == nrows,
                  "DataFrame::plot().bar: x label feature length mismatch with cases");
    } else {
      x_values = thread_to_plot_numeric_values(x_thread);
      require_plot_size(x_values, nrows, *x_feature);
    }
    x_label = std::string(*x_feature);
  }

  pluss::plotism::Figure fig;
  fig.add_bar(x_values, y_values, std::string(y_feature), std::move(x_labels));
  fig.x_label(x_label).y_label(std::string(y_feature));
  return fig;
}

pluss::plotism::Figure DataFramePlotAccessor::hist(std::string_view feature, std::int64_t bins) const {
  auto fig = pluss::plotism::hist(thread_to_plot_numeric_values(frame_.features(feature)),
                                  bins,
                                  std::string(feature));
  fig.x_label(std::string(feature)).y_label("count");
  return fig;
}

DataFramePlotAccessor DataFrame::plot() const {
  return DataFramePlotAccessor(*this);
}

} // namespace pluss::table
