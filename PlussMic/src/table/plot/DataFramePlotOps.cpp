#include "table/plot/DataFramePlotOps.h"

#include "CodeM/core/Errors.h"
#include "table/plot/DataFramePlot.h"

namespace pluss::table::_ops {

namespace {

std::string_view first_feature(std::initializer_list<std::string_view> features, std::string_view where) {
  PLUSS_CHECK(features.size() > 0, std::string(where) + ": requires at least one feature");
  return *features.begin();
}

std::pair<std::string_view, std::string_view> first_two_features(std::initializer_list<std::string_view> features,
                                                                  std::string_view where) {
  PLUSS_CHECK(features.size() >= 2, std::string(where) + ": requires at least two features");
  auto it = features.begin();
  const auto first = *it++;
  const auto second = *it;
  return {first, second};
}

std::tuple<std::string_view, std::string_view, std::string_view> first_three_features(
    std::initializer_list<std::string_view> features,
    std::string_view where) {
  PLUSS_CHECK(features.size() >= 3, std::string(where) + ": requires three features");
  auto it = features.begin();
  const auto a = *it++;
  const auto b = *it++;
  const auto c = *it;
  return {a, b, c};
}

} // namespace

pluss::pl::Studio call(const DataFramePlotAccessor& accessor, PlotDispatch dispatch, PlotCall args) {
  switch (dispatch) {
    case PlotDispatch::Line:
      return accessor.impl_line(args.features, args.x_feature);
    case PlotDispatch::Step:
      return accessor.impl_step(args.features, args.x_feature);
    case PlotDispatch::Area:
      return accessor.impl_area(args.features, args.x_feature);
    case PlotDispatch::AreaStacked:
      return accessor.impl_area_stacked(args.features, args.x_feature);
    case PlotDispatch::Scatter: {
      const auto [x_feature, y_feature] = first_two_features(args.features, "DataFrame::plot().scatter");
      return accessor.impl_scatter(x_feature, y_feature, args.hue_feature);
    }
    case PlotDispatch::Bar:
      return accessor.impl_bar(args.features, args.x_feature);
    case PlotDispatch::BarStacked:
      return accessor.impl_bar_stacked(args.features, args.x_feature);
    case PlotDispatch::Box:
      return accessor.impl_box(args.features);
    case PlotDispatch::Hist:
      return accessor.impl_hist(first_feature(args.features, "DataFrame::plot().hist"), args.bins);
    case PlotDispatch::Heatmap:
      return accessor.impl_heatmap(args.features);
    case PlotDispatch::HeatmapPivot: {
      const auto [case_feature, feature_feature, value_feature] =
          first_three_features(args.features, "DataFrame::plot().heatmap");
      return accessor.impl_heatmap(case_feature, feature_feature, value_feature, args.group_op);
    }
    case PlotDispatch::CorrHeatmap:
      return accessor.impl_corr_heatmap(args.features);
    case PlotDispatch::ScatterMatrix:
      return accessor.impl_scatter_matrix(args.features);
    case PlotDispatch::AutoPlot:
      return accessor.impl_auto_plot(args.features);
  }

  PLUSS_FAIL("DataFrame::plot().*: unsupported dispatch operation");
}

} // namespace pluss::table::_ops
