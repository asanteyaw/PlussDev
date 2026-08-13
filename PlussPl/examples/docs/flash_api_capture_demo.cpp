#include <pluss/pluss.h>
#include "../../../PlussMic/src/table/DataFrame.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace pluss;
using namespace pluss::pl;
using namespace pluss::pl::flash;

Series make_grid_surface(std::int64_t rows, std::int64_t cols) {
  const auto x_axis = series::linspace(-3.0, 3.0, cols, ScalarType::Float64).reshape({1, cols});
  const auto y_axis = series::linspace(-3.0, 3.0, rows, ScalarType::Float64).reshape({rows, 1});
  const std::array<std::int64_t, 2> x_tile{rows, 1};
  const std::array<std::int64_t, 2> y_tile{1, cols};
  const auto x = x_axis.tile(std::span<const std::int64_t>(x_tile.data(), x_tile.size()));
  const auto y = y_axis.tile(std::span<const std::int64_t>(y_tile.data(), y_tile.size()));
  return x.mul(0.6).sin().add(y.mul(0.7).cos());
}

Series make_feature_matrix(std::int64_t rows = 140) {
  std::vector<double> flat;
  flat.reserve(static_cast<std::size_t>(rows * 5));
  for (std::int64_t i = 0; i < rows; ++i) {
    const double t = static_cast<double>(i) / 12.0;
    const double f1 = std::sin(t) + 0.15 * std::cos(2.0 * t);
    const double f2 = std::cos(0.8 * t) + 0.12 * std::sin(1.5 * t);
    const double f3 = 0.35 * f1 + 0.65 * f2;
    const double f4 = 0.05 * static_cast<double>(i) + 0.2 * std::sin(0.45 * t);
    const double f5 = f1 - f2;
    flat.push_back(f1);
    flat.push_back(f2);
    flat.push_back(f3);
    flat.push_back(f4);
    flat.push_back(f5);
  }
  return make_series(std::move(flat), {rows, 5});
}

std::vector<Series> matrix_columns(const Series& matrix) {
  std::vector<Series> cols;
  const auto shape = matrix.sizes();
  const std::int64_t ncols = (matrix.dim() >= 2) ? shape[1] : shape[0];
  cols.reserve(static_cast<std::size_t>(ncols));
  for (std::int64_t c = 0; c < ncols; ++c) {
    cols.push_back(matrix.select(1, c));
  }
  return cols;
}

Dataset make_demo_dataset(const Series& matrix) {
  return make_dataset(matrix, {"f1", "f2", "f3", "f4", "f5"}, Axis::Cols);
}

struct HyperCurveData {
  Series hx;
  Series hy;
  Series hz;
  Series hw;
};

HyperCurveData make_hyper_curve(std::int64_t points = 480) {
  constexpr double kPi = 3.14159265358979323846;
  const auto t = series::linspace(0.0, 2.0 * kPi, points, ScalarType::Float64);
  return HyperCurveData{
      .hx = t.cos(),
      .hy = t.sin(),
      .hz = t.mul(2.0).cos().mul(0.72),
      .hw = t.mul(3.0).sin().mul(0.58),
  };
}

bool render_case(std::string_view key, const std::filesystem::path& out_svg) {
  constexpr double kPi = 3.14159265358979323846;

  auto save = [&](Studio& fig) {
    if (out_svg.has_parent_path()) {
      std::error_code ec;
      std::filesystem::create_directories(out_svg.parent_path(), ec);
    }
    fig.save_svg(out_svg.string());
  };

  const auto x = series::linspace(0.0, 12.0, 180, ScalarType::Float64);
  const auto y = x.mul(0.6).sin().add(x.mul(0.22).cos().mul(0.5));
  const auto z = x.mul(0.35).cos().add(0.3);

  const auto matrix = make_feature_matrix();
  const auto columns = matrix_columns(matrix);
  const auto dataset = make_demo_dataset(matrix);

  if (key == "scatterplot") {
    Studio fig("scatterplot", Theme::paper());
    set_context(fig, 960.0, 620.0, 1.0, 1.0);
    auto& pane = fig.pane2("scatterplot");
    scatterplot(pane, x, y).label("scatterplot");
    save(fig);
    return true;
  }
  if (key == "lineplot") {
    Studio fig("lineplot", Theme::paper());
    set_context(fig, 960.0, 620.0, 1.0, 1.0);
    auto& pane = fig.pane2("lineplot");
    lineplot(pane, x, y).label("lineplot");
    save(fig);
    return true;
  }
  if (key == "stepplot") {
    Studio fig("stepplot", Theme::paper());
    auto& pane = fig.pane2("stepplot");
    stepplot(pane, x, y).label("stepplot");
    save(fig);
    return true;
  }
  if (key == "relplot") {
    Studio fig("relplot", Theme::paper());
    auto& pane = fig.pane2("relplot");
    relplot(pane, x, y, "line").label("relplot");
    save(fig);
    return true;
  }
  if (key == "histplot") {
    Studio fig("histplot", Theme::paper());
    auto& pane = fig.pane2("histplot");
    histplot(pane, y, 20).label("histplot");
    save(fig);
    return true;
  }
  if (key == "displot") {
    Studio fig("displot", Theme::paper());
    auto& pane = fig.pane2("displot");
    displot(pane, y, 18).label("displot");
    save(fig);
    return true;
  }
  if (key == "kdeplot") {
    Studio fig("kdeplot", Theme::paper());
    auto& pane = fig.pane2("kdeplot");
    kdeplot(pane, y, 220).label("kdeplot");
    save(fig);
    return true;
  }
  if (key == "ecdfplot") {
    Studio fig("ecdfplot", Theme::paper());
    auto& pane = fig.pane2("ecdfplot");
    ecdfplot(pane, y).label("ecdfplot");
    save(fig);
    return true;
  }
  if (key == "areaplot") {
    Studio fig("areaplot", Theme::paper());
    auto& pane = fig.pane2("areaplot");
    areaplot(pane, x, y, -0.2).label("areaplot");
    save(fig);
    return true;
  }
  if (key == "rugplot") {
    Studio fig("rugplot", Theme::paper());
    auto& pane = fig.pane2("rugplot");
    rugplot(pane, y, 0.09).label("rugplot");
    save(fig);
    return true;
  }
  if (key == "stripplot") {
    Studio fig("stripplot", Theme::paper());
    auto& pane = fig.pane2("stripplot");
    const Note cats = make_note({"A", "A", "A", "B", "B", "B", "C", "C", "C"});
    const Series vals = make_series({0.9, 1.2, 1.4, 1.1, 1.6, 1.9, 0.7, 0.8, 1.0});
    stripplot(pane, cats, vals).label("stripplot");
    save(fig);
    return true;
  }
  if (key == "swarmplot") {
    Studio fig("swarmplot", Theme::paper());
    auto& pane = fig.pane2("swarmplot");
    const Note cats = make_note({"A", "A", "A", "B", "B", "B", "C", "C", "C"});
    const Series vals = make_series({0.9, 1.2, 1.4, 1.1, 1.6, 1.9, 0.7, 0.8, 1.0});
    swarmplot(pane, cats, vals).label("swarmplot");
    save(fig);
    return true;
  }
  if (key == "boxplot") {
    Studio fig("boxplot", Theme::paper());
    auto& pane = fig.pane2("boxplot");
    Note cats = make_note({"A", "A", "A", "B", "B", "B", "C", "C", "C"});
    Series vals = make_series({0.9, 1.2, 1.4, 1.1, 1.6, 1.9, 0.7, 0.8, 1.0});
    boxplot(pane, cats, vals).label("boxplot");
    save(fig);
    return true;
  }
  if (key == "violinplot") {
    Studio fig("violinplot", Theme::paper());
    auto& pane = fig.pane2("violinplot");
    Note cats = make_note({"A", "A", "A", "B", "B", "B", "C", "C", "C"});
    Series vals = make_series({0.9, 1.2, 1.4, 1.1, 1.6, 1.9, 0.7, 0.8, 1.0});
    violinplot(pane, cats, vals).label("violinplot");
    save(fig);
    return true;
  }
  if (key == "boxenplot") {
    Studio fig("boxenplot", Theme::paper());
    auto& pane = fig.pane2("boxenplot");
    boxenplot(pane, y).label("boxenplot");
    save(fig);
    return true;
  }
  if (key == "pointplot") {
    Studio fig("pointplot", Theme::paper());
    auto& pane = fig.pane2("pointplot");
    auto px = make_series({1.0, 2.0, 3.0, 4.0});
    auto py = make_series({3.2, 3.9, 4.1, 4.8});
    auto pe = make_series({0.15, 0.2, 0.17, 0.25});
    pointplot(pane, px, py, pe).label("pointplot");
    save(fig);
    return true;
  }
  if (key == "whiskplot") {
    Studio fig("whiskplot", Theme::paper());
    auto& pane = fig.pane2("whiskplot");
    auto px = make_series({1.0, 2.0, 3.0, 4.0});
    auto py = make_series({3.2, 3.9, 4.1, 4.8});
    auto pe = make_series({0.15, 0.2, 0.17, 0.25});
    whiskplot(pane, px, py, pe).label("whiskplot");
    save(fig);
    return true;
  }
  if (key == "barplot") {
    Studio fig("barplot", Theme::paper());
    auto& pane = fig.pane2("barplot");
    barplot(pane,
            make_note({"infra", "models", "data", "ops"}),
            make_series({21.0, 28.0, 24.0, 19.0}))
        .label("barplot");
    save(fig);
    return true;
  }
  if (key == "barplot_note_horizontal") {
    Studio fig("barplot_note_horizontal", Theme::paper());
    auto& pane = fig.pane2("barplot_note_horizontal");
    BarPlotOptions options;
    options.horizontal = true;
    barplot(pane,
            make_note({"a", "b", "c"}),
            make_series({4.0, 7.0, 3.0}),
            options)
        .label("barplot");
    save(fig);
    return true;
  }
  if (key == "barplot_numeric_horizontal") {
    Studio fig("barplot_numeric_horizontal", Theme::paper());
    auto& pane = fig.pane2("barplot_numeric_horizontal");
    BarPlotOptions options;
    options.horizontal = true;
    barplot(pane,
            make_series({4.0, 7.0, 3.0}),
            make_series({0.0, 1.0, 2.0}),
            options)
        .label("barplot");
    save(fig);
    return true;
  }
  if (key == "countplot") {
    Studio fig("countplot", Theme::paper());
    auto& pane = fig.pane2("countplot");
    countplot(pane, make_note({"A", "B", "A", "C", "A", "B", "B", "C", "A"})).label("countplot");
    save(fig);
    return true;
  }
  if (key == "pieplot") {
    Studio fig("pieplot", Theme::paper());
    auto& pane = fig.pane2("pieplot");
    pieplot(
        pane,
        make_series({18.0, 27.0, 21.0, 34.0}),
        {"NLP", "Risk", "Ops", "Infra"})
        .label("pieplot");
    save(fig);
    return true;
  }
  if (key == "catplot") {
    Studio fig("catplot", Theme::paper());
    auto& pane = fig.pane2("catplot");
    catplot(pane,
            make_note({"A", "A", "A", "B", "B", "B", "C", "C", "C"}),
            make_series({0.9, 1.2, 1.4, 1.1, 1.6, 1.9, 0.7, 0.8, 1.0}),
            "violin")
        .label("catplot");
    save(fig);
    return true;
  }
  if (key == "regplot") {
    Studio fig("regplot", Theme::paper());
    auto& pane = fig.pane2("regplot");
    regplot(pane, x, y).label("regplot");
    save(fig);
    return true;
  }
  if (key == "lmplot") {
    Studio fig("lmplot", Theme::paper());
    lmplot(fig, x, y, "lmplot").label("lmplot");
    save(fig);
    return true;
  }
  if (key == "qqplot") {
    Studio fig("qqplot", Theme::paper());
    auto& pane = fig.pane2("qqplot");
    qqplot(pane, y);
    save(fig);
    return true;
  }
  if (key == "residplot") {
    Studio fig("residplot", Theme::paper());
    auto& pane = fig.pane2("residplot");
    residplot(pane, x, y);
    save(fig);
    return true;
  }
  if (key == "resid_histplot") {
    Studio fig("resid_histplot", Theme::paper());
    auto& pane = fig.pane2("resid_histplot");
    resid_histplot(pane, x, y);
    save(fig);
    return true;
  }
  if (key == "heatmap") {
    Studio fig("heatmap", Theme::paper());
    auto& pane = fig.pane2("heatmap");
    const auto gx = series::linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gy = series::linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gz = make_grid_surface(40, 40);
    heatmap(pane, gx, gy, gz, HeatmapOptions{.scale_guide = true});
    save(fig);
    return true;
  }
  if (key == "clustermap") {
    Studio fig("clustermap", Theme::paper());
    auto& pane = fig.pane2("clustermap");
    const auto gx = series::linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gy = series::linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gz = make_grid_surface(40, 40);
    clustermap(pane, gx, gy, gz).label("clustermap");
    save(fig);
    return true;
  }
  if (key == "pairplot") {
    Studio fig("pairplot", Theme::paper());
    pairplot(fig, dataset);
    save(fig);
    return true;
  }
  if (key == "jointplot") {
    Studio fig("jointplot", Theme::paper());
    jointplot(fig, x, y, "scatter");
    save(fig);
    return true;
  }
  if (key == "triplot") {
    Studio fig("triplot", Theme::paper());
    TriplotOptions opts;
    opts.max_panels = 6;
    opts.max_points = 140;
    opts.kind = TriplotKind::Cloud;
    triplot(fig, matrix, {"f1", "f2", "f3", "f4", "f5"}, Axis::Cols, opts);
    save(fig);
    return true;
  }
  if (key == "parallelplot") {
    Studio fig("parallelplot", Theme::paper());
    ParallelCoordinatesOptions opts;
    opts.max_points = 120;
    parallelplot(fig, matrix, {"f1", "f2", "f3", "f4", "f5"}, Axis::Cols, opts);
    save(fig);
    return true;
  }
  if (key == "andrews_curves") {
    Studio fig("andrews_curves", Theme::paper());
    AndrewsCurvesOptions opts;
    opts.max_curves = 120;
    andrews_curves(fig, matrix, {"f1", "f2", "f3", "f4", "f5"}, Axis::Cols, opts);
    save(fig);
    return true;
  }
  if (key == "radviz") {
    Studio fig("radviz", Theme::paper());
    RadvizOptions opts;
    opts.max_points = 120;
    radviz(fig, matrix, {"f1", "f2", "f3", "f4", "f5"}, Axis::Cols, opts);
    save(fig);
    return true;
  }
  if (key == "lagplot") {
    Studio fig("lagplot", Theme::paper());
    lagplot(fig, y);
    save(fig);
    return true;
  }
  if (key == "acfplot") {
    Studio fig("acfplot", Theme::paper());
    acfplot(fig, y);
    save(fig);
    return true;
  }
  if (key == "pacfplot") {
    Studio fig("pacfplot", Theme::paper());
    pacfplot(fig, y);
    save(fig);
    return true;
  }
  if (key == "ccfplot") {
    Studio fig("ccfplot", Theme::paper());
    ccfplot(fig, y, z);
    save(fig);
    return true;
  }
  if (key == "actplot") {
    Studio fig("actplot", Theme::paper());
    actplot(fig, y);
    save(fig);
    return true;
  }
  if (key == "resid_acfplot") {
    Studio fig("resid_acfplot", Theme::paper());
    resid_acfplot(fig, x, y);
    save(fig);
    return true;
  }
  if (key == "bootstrapplot") {
    Studio fig("bootstrapplot", Theme::paper());
    bootstrapplot(fig, y);
    save(fig);
    return true;
  }
  if (key == "summaryplot") {
    Studio fig("summaryplot", Theme::paper());
    summaryplot(fig, columns, {"f1", "f2", "f3", "f4", "f5"});
    save(fig);
    return true;
  }
  if (key == "set_context") {
    Studio fig("set_context", Theme::paper());
    set_context(fig, 1180.0, 760.0, 1.2, 1.15);
    auto& pane = fig.pane2("set_context");
    lineplot(pane, x, y).label("signal");
    save(fig);
    return true;
  }
  if (key == "plotting_context") {
    Studio fig("plotting_context", Theme::paper());
    plotting_context(fig, 1200.0, 780.0, 1.0, 1.0);
    auto& pane = fig.pane2("plotting_context");
    lineplot(pane, x, y).label("signal");
    save(fig);
    return true;
  }
  if (key == "set_theme_studio") {
    Studio fig("set_theme_studio", Theme::paper());
    set_theme(
        fig,
        FlashThemeOptions{
            .style = FlashStyle::Sandgrid,
            .context = FlashContext::Talk,
            .borderless = false,
            .horizontal_grid = true,
        });
    auto& pane = fig.pane2("set_theme_studio");
    lineplot(pane, x, y).label("signal");
    save(fig);
    return true;
  }
  if (key == "set_theme_pane") {
    Studio fig("set_theme_pane", Theme::paper());
    auto& pane = fig.pane2("set_theme_pane");
    set_theme(
        pane,
        FlashThemeOptions{
            .style = FlashStyle::Lightgrid,
            .context = FlashContext::Notebook,
            .borderless = false,
            .horizontal_grid = false,
        });
    lineplot(pane, x, y).label("signal");
    save(fig);
    return true;
  }
  if (key == "set_style") {
    Studio fig("set_style", Theme::paper());
    auto& pane = fig.pane2("set_style");
    set_style(pane, FlashStyle::Darkgrid);
    lineplot(pane, x, y).label("signal");
    save(fig);
    return true;
  }
  if (key == "horizontal_grid") {
    Studio fig("horizontal_grid", Theme::paper());
    auto& pane = fig.pane2("horizontal_grid");
    set_style(pane, FlashStyle::Lightgrid);
    horizontal_grid(pane, true);
    lineplot(pane, x, y).label("signal");
    save(fig);
    return true;
  }
  if (key == "despine") {
    Studio fig("despine", Theme::paper());
    auto& pane = fig.pane2("despine");
    set_style(pane, FlashStyle::Light);
    despine(pane, true);
    lineplot(pane, x, y).label("signal");
    save(fig);
    return true;
  }
  if (key == "tight_layout") {
    Studio fig("tight_layout", Theme::paper());
    fig.mosaic(1, 2, 12.0, 12.0);
    auto& left = fig.pane2("left");
    left.place(0, 0);
    lineplot(left, x, y).label("left");
    auto& right = fig.pane2("right");
    right.place(0, 1);
    lineplot(right, x, y.mul(0.6).add(0.2)).label("right");
    tight_layout(fig, 8.0);
    save(fig);
    return true;
  }
  if (key == "color_palette") {
    Studio fig("color_palette", Theme::paper());
    auto& pane = fig.pane2("color_palette");
    const auto gx = series::linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gy = series::linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gz = make_grid_surface(40, 40);
    auto& trace = heatmap(pane, gx, gy, gz, HeatmapOptions{.scale_guide = true});
    trace.palette(color_palette("ocean"));
    save(fig);
    return true;
  }
  if (key == "flash_palette") {
    Studio fig("flash_palette", Theme::paper());
    auto& pane = fig.pane2("flash_palette");
    const auto gx = series::linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gy = series::linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gz = make_grid_surface(40, 40);
    auto& trace = heatmap(pane, gx, gy, gz, HeatmapOptions{.scale_guide = true});
    trace.palette(flash_palette(FlashPalette::Muted));
    save(fig);
    return true;
  }
  if (key == "flash_theme") {
    Studio fig("flash_theme", Theme::paper());
    fig.theme(flash_theme(FlashStyle::Sandgrid, FlashPalette::Bright));
    auto& pane = fig.pane2("flash_theme");
    lineplot(pane, x, y).label("flash_theme");
    save(fig);
    return true;
  }
  if (key == "flash_palette_kind") {
    Studio fig("flash_palette_kind", Theme::paper());
    auto& pane = fig.pane2("flash_palette_kind");
    const auto gx = series::linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gy = series::linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gz = make_grid_surface(40, 40);
    auto& trace = heatmap(pane, gx, gy, gz, HeatmapOptions{.scale_guide = true});
    trace.palette(flash_palette(FlashPalette::Colorblind));
    save(fig);
    return true;
  }
  if (key == "blend_palette") {
    Studio fig("blend_palette", Theme::paper());
    auto& pane = fig.pane2("blend_palette");
    const auto gx = series::linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gy = series::linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gz = make_grid_surface(40, 40);
    auto& trace = heatmap(pane, gx, gy, gz, HeatmapOptions{.scale_guide = true});
    trace.palette(blend_palette(
        {Rgba::from_u8(42, 95, 153), Rgba::from_u8(242, 178, 109), Rgba::from_u8(181, 80, 54)},
        "blend_demo"));
    save(fig);
    return true;
  }
  if (key == "hls_palette") {
    Studio fig("hls_palette", Theme::paper());
    auto& pane = fig.pane2("hls_palette");
    const auto gx = series::linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gy = series::linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gz = make_grid_surface(40, 40);
    auto& trace = heatmap(pane, gx, gy, gz, HeatmapOptions{.scale_guide = true});
    trace.palette(hls_palette(8, 0.72, 0.54));
    save(fig);
    return true;
  }
  if (key == "husl_palette") {
    Studio fig("husl_palette", Theme::paper());
    auto& pane = fig.pane2("husl_palette");
    const auto gx = series::linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gy = series::linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gz = make_grid_surface(40, 40);
    auto& trace = heatmap(pane, gx, gy, gz, HeatmapOptions{.scale_guide = true});
    trace.palette(husl_palette(8));
    save(fig);
    return true;
  }
  if (key == "cubehelix_palette") {
    Studio fig("cubehelix_palette", Theme::paper());
    auto& pane = fig.pane2("cubehelix_palette");
    const auto gx = series::linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gy = series::linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gz = make_grid_surface(40, 40);
    auto& trace = heatmap(pane, gx, gy, gz, HeatmapOptions{.scale_guide = true});
    trace.palette(cubehelix_palette(8));
    save(fig);
    return true;
  }
  if (key == "light_palette") {
    Studio fig("light_palette", Theme::paper());
    auto& pane = fig.pane2("light_palette");
    const auto gx = series::linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gy = series::linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gz = make_grid_surface(40, 40);
    auto& trace = heatmap(pane, gx, gy, gz, HeatmapOptions{.scale_guide = true});
    trace.palette(light_palette(Rgba::from_u8(42, 95, 153), 8));
    save(fig);
    return true;
  }
  if (key == "dark_palette") {
    Studio fig("dark_palette", Theme::paper());
    auto& pane = fig.pane2("dark_palette");
    const auto gx = series::linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gy = series::linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gz = make_grid_surface(40, 40);
    auto& trace = heatmap(pane, gx, gy, gz, HeatmapOptions{.scale_guide = true});
    trace.palette(dark_palette(Rgba::from_u8(42, 95, 153), 8));
    save(fig);
    return true;
  }
  if (key == "diverging_palette") {
    Studio fig("diverging_palette", Theme::paper());
    auto& pane = fig.pane2("diverging_palette");
    const auto gx = series::linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gy = series::linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gz = make_grid_surface(40, 40);
    auto& trace = heatmap(pane, gx, gy, gz, HeatmapOptions{.scale_guide = true});
    trace.palette(diverging_palette(Rgba::from_u8(42, 95, 153), Rgba::from_u8(181, 80, 54), 9));
    save(fig);
    return true;
  }
  if (key == "mpl_palette") {
    Studio fig("mpl_palette", Theme::paper());
    auto& pane = fig.pane2("mpl_palette");
    const auto gx = series::linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gy = series::linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gz = make_grid_surface(40, 40);
    auto& trace = heatmap(pane, gx, gy, gz, HeatmapOptions{.scale_guide = true});
    trace.palette(mpl_palette("viridis"));
    save(fig);
    return true;
  }
  if (key == "xkcd_palette") {
    Studio fig("xkcd_palette", Theme::paper());
    auto& pane = fig.pane2("xkcd_palette");
    const auto gx = series::linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gy = series::linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gz = make_grid_surface(40, 40);
    auto& trace = heatmap(pane, gx, gy, gz, HeatmapOptions{.scale_guide = true});
    trace.palette(xkcd_palette());
    save(fig);
    return true;
  }
  if (key == "crayon_palette") {
    Studio fig("crayon_palette", Theme::paper());
    auto& pane = fig.pane2("crayon_palette");
    const auto gx = series::linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gy = series::linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gz = make_grid_surface(40, 40);
    auto& trace = heatmap(pane, gx, gy, gz, HeatmapOptions{.scale_guide = true});
    trace.palette(crayon_palette());
    save(fig);
    return true;
  }
  if (key == "axes_style") {
    Studio fig("axes_style", Theme::paper());
    auto& pane = fig.pane2("axes_style");
    axes_style(pane, FlashStyle::Darkgrid);
    lineplot(pane, x, y).label("axes_style");
    save(fig);
    return true;
  }
  if (key == "tickless") {
    Studio fig("tickless", Theme::paper());
    auto& pane = fig.pane2("tickless");
    set_style(pane, FlashStyle::Lightgrid);
    tickless(pane, true);
    lineplot(pane, x, y).label("tickless");
    save(fig);
    return true;
  }
  if (key == "move_legend") {
    Studio fig("move_legend", Theme::paper());
    auto& pane = fig.pane2("move_legend");
    lineplot(pane, x, y).label("signal");
    lineplot(pane, x, y.mul(0.8).add(0.2)).label("baseline");
    move_legend(pane, KeyPlace::NorthEast);
    move_legend(fig, KeyDock::East);
    save(fig);
    return true;
  }
  if (key == "flash_set") {
    Studio fig("flash_set", Theme::paper());
    set(fig, Theme::ocean());
    auto& pane = fig.pane2("flash_set");
    lineplot(pane, x, y).label("flash_set");
    save(fig);
    return true;
  }
  if (key == "flash_show") {
    Studio fig("flash_show", Theme::paper());
    auto& pane = fig.pane2("flash_show");
    lineplot(pane, x, y).label("flash_show");
    const bool ok = show(fig, "flash_show", 920, 620, false);
    pane.caption(ok ? "show() call succeeded" : "show() call returned false");
    save(fig);
    return true;
  }
  if (key == "flash_save_png") {
    Studio fig("flash_save_png", Theme::paper());
    auto& pane = fig.pane2("flash_save_png");
    lineplot(pane, x, y).label("flash_save_png");
    const auto png_out = out_svg.parent_path() / "flash_save_png_tmp.png";
    save_png(fig, png_out.string(), 144);
    save(fig);
    return true;
  }
  if (key == "make_dataset") {
    Studio fig("make_dataset", Theme::paper());
    const auto ds = make_dataset(matrix, {"f1", "f2", "f3", "f4", "f5"}, Axis::Cols);
    auto& pane = fig.pane2("make_dataset");
    if (ds.numeric_values.size() >= 2) {
      scatterplot(pane, ds.numeric_values[0], ds.numeric_values[1]).label("dataset features");
    } else {
      lineplot(pane, x, y).label("fallback");
    }
    save(fig);
    return true;
  }
  if (key == "dataset_from_frame_pairplot") {
    using namespace pluss::table;

    auto frame = DataFrame({
        {"city", make_note({"Accra", "Tema", "Kumasi", "Tamale", "Takoradi", "Cape Coast"})},
        {"units", make_series(std::vector<double>{14.0, 9.0, 17.0, 6.0, 11.0, 13.0})},
        {"price", make_series(std::vector<double>{3.2, 4.1, 2.8, 5.0, 3.6, 3.9})},
        {"margin", make_series(std::vector<double>{1.1, 1.5, 0.9, 1.9, 1.3, 1.4})},
    }).title("sales");

    auto data = Dataset::from_frame(
        frame,
        {.features = {"units", "price", "margin", "city"}});

    Studio fig("dataset_from_frame_pairplot", Theme::paper());
    pairplot(fig, data);
    save(fig);
    return true;
  }
  if (key == "dataset_from_frame_jointplot") {
    using namespace pluss::table;

    auto frame = DataFrame({
        {"city", make_note({"Accra", "Tema", "Kumasi", "Tamale", "Takoradi", "Cape Coast"})},
        {"units", make_series(std::vector<double>{14.0, 9.0, 17.0, 6.0, 11.0, 13.0})},
        {"price", make_series(std::vector<double>{3.2, 4.1, 2.8, 5.0, 3.6, 3.9})},
        {"margin", make_series(std::vector<double>{1.1, 1.5, 0.9, 1.9, 1.3, 1.4})},
    }).title("sales");

    auto data = Dataset::from_frame(
        frame,
        {.features = {"units", "price", "margin", "city"}});

    Studio fig("dataset_from_frame_jointplot", Theme::paper());
    jointplot(fig, data, "units", "margin", "reg");
    save(fig);
    return true;
  }
  if (key == "get_dataset_names") {
    Studio fig("get_dataset_names", Theme::paper());
    auto& pane = fig.pane2("get_dataset_names");
    const auto names = get_dataset_names();
    const std::vector<double> counts(static_cast<std::size_t>(names.numel()), 1.0);
    barplot(pane, names, make_series(counts)).label("datasets");
    save(fig);
    return true;
  }
  if (key == "catplot_kind") {
    Studio fig("catplot_kind", Theme::paper());
    auto& pane = fig.pane2("catplot_kind");
    CatPlotOptions options;
    options.kind = CatPlotKind::Bar;
    options.errorbar = CatErrorBar::SE;
    catplot(
        pane,
        make_note({"A", "A", "A", "B", "B", "B", "C", "C", "C"}),
        make_series({0.9, 1.2, 1.4, 1.1, 1.6, 1.9, 0.7, 0.8, 1.0}),
        options)
        .label("catplot_kind");
    save(fig);
    return true;
  }
  if (key == "relgrid_kind") {
    Studio fig("relgrid_kind", Theme::paper());
    RelGridPlotOptions options;
    options.kind = RelGridKind::Line;
    relgridplot(
        fig,
        {x, x, x},
        {y, y.mul(0.8), y.mul(0.6).add(0.2)},
        options);
    save(fig);
    return true;
  }
  if (key == "disgrid_kind") {
    Studio fig("disgrid_kind", Theme::paper());
    DisGridPlotOptions options;
    options.kind = DisGridKind::Ecdf;
    disgridplot(fig, {y, y.add(0.25), y.sub(0.3)}, options);
    save(fig);
    return true;
  }
  if (key == "load_dataset") {
    Studio fig("load_dataset", Theme::paper());
    if (auto data = load_dataset("tips")) {
      pairplot(fig, *data);
    } else {
      auto& pane = fig.pane2("fallback");
      lineplot(pane, x, y).label("fallback");
    }
    save(fig);
    return true;
  }
  if (key == "load_dataset_weather_hourly") {
    Studio fig("load_dataset_weather_hourly", Theme::paper());
    auto& pane = fig.pane2("weather_hourly");
    if (auto data = load_dataset("weather_hourly")) {
      auto time = data->continuous("timestamp");
      auto demand = data->numeric("demand_mw");
      if (time && demand) {
        lineplot(
            pane,
            time->slice(0, 0, 336),
            demand->slice(0, 0, 336))
            .label("demand_mw");
      } else {
        lineplot(pane, x, y).label("fallback");
      }
    } else {
      lineplot(pane, x, y).label("fallback");
    }
    save(fig);
    return true;
  }
  if (key == "load_dataset_options_chain") {
    Studio fig("load_dataset_options_chain", Theme::paper());
    auto& pane = fig.pane2("options_chain");
    if (auto data = load_dataset("options_chain")) {
      auto strike = data->numeric("strike");
      auto iv = data->numeric("iv");
      if (strike && iv) {
        scatterplot(
            pane,
            strike->slice(0, 0, 240),
            iv->slice(0, 0, 240));
      } else {
        lineplot(pane, x, y).label("fallback");
      }
    } else {
      lineplot(pane, x, y).label("fallback");
    }
    save(fig);
    return true;
  }
  if (key == "scene3d") {
    Studio fig("scene3d", Theme::paper());
    fig.mosaic(1, 1, 12.0, 12.0);
    auto& pane = fig.pane3("scene");
    pane.place(0, 0);
    scene3d(pane, Scene3DOptions{.preset = ScenePreset3D::Presentation});
    const auto gx = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gy = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gz = make_grid_surface(24, 24);
    auto& tr = sheet(pane, gx, gy, gz).label("sheet");
    trace3d(tr, Trace3DOptions{.preset = TracePreset3D::Surface});
    save(fig);
    return true;
  }
  if (key == "camera3d") {
    Studio fig("camera3d", Theme::paper());
    auto& pane = fig.pane3("camera3d");
    const auto gx = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gy = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gz = make_grid_surface(24, 24);
    camera3d(
        pane,
        Camera3DOptions{
            .azimuth_deg = -28.0,
            .elevation_deg = 22.0,
            .view_angle_deg = 26.0,
            .projection = ProjectionKind::Perspective,
        });
    sheet(pane, gx, gy, gz).label("camera3d");
    save(fig);
    return true;
  }
  if (key == "camera3d_topdown") {
    Studio fig("camera3d_topdown", Theme::paper());
    auto& pane = fig.pane3("camera3d_topdown");
    const auto gx = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gy = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gz = make_grid_surface(24, 24);
    camera3d(pane, 0.0, 90.0, ProjectionKind::Orthographic, 30.0);
    auto& tr = sheet(pane, gx, gy, gz).label("camera3d_topdown");
    trace_preset3d(tr, TracePreset3D::Surface);
    save(fig);
    return true;
  }
  if (key == "camera_preset3d") {
    Studio fig("camera_preset3d", Theme::paper());
    auto& pane = fig.pane3("camera_preset3d");
    const auto gx = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gy = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gz = make_grid_surface(24, 24);
    camera_preset3d(pane, CameraPreset3D::Isometric);
    sheet(pane, gx, gy, gz).label("camera_preset3d");
    save(fig);
    return true;
  }
  if (key == "lighting3d") {
    Studio fig("lighting3d", Theme::paper());
    auto& pane = fig.pane3("lighting3d");
    const auto gx = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gy = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gz = make_grid_surface(24, 24);
    lighting3d(
        pane,
        Lighting3DOptions{
            .clear_first = true,
            .headlight = true,
            .lamp_left = true,
            .lamp_right = true,
            .sun_key = false,
        });
    sheet(pane, gx, gy, gz).label("lighting3d");
    save(fig);
    return true;
  }
  if (key == "lighting3d_rig") {
    Studio fig("lighting3d_rig", Theme::paper());
    auto& pane = fig.pane3("lighting3d_rig");
    const auto gx = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gy = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gz = make_grid_surface(24, 24);
    auto& tr = sheet(pane, gx, gy, gz).label("lighting3d_rig");
    trace_preset3d(tr, TracePreset3D::Surface);
    lighting3d(
        pane,
        Lighting3DOptions{
            .clear_first = true,
            .headlight = false,
            .lamp_left = true,
            .lamp_right = true,
            .sun_key = true,
        });
    camera_preset3d(pane, CameraPreset3D::Isometric);
    save(fig);
    return true;
  }
  if (key == "theme_preset3d") {
    Studio fig("theme_preset3d", Theme::paper());
    auto& pane = fig.pane3("theme_preset3d");
    const auto gx = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gy = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gz = make_grid_surface(24, 24);
    theme_preset3d(pane, ThemePreset3D::Slate);
    sheet(pane, gx, gy, gz).label("theme_preset3d");
    save(fig);
    return true;
  }
  if (key == "theme_preset3d_mist") {
    Studio fig("theme_preset3d_mist", Theme::paper());
    auto& pane = fig.pane3("theme_preset3d_mist");
    const auto gx = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gy = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gz = make_grid_surface(24, 24);
    theme_preset3d(pane, ThemePreset3D::Mist);
    auto& tr = sheet(pane, gx, gy, gz).label("theme_preset3d_mist");
    trace_preset3d(tr, TracePreset3D::Surface);
    save(fig);
    return true;
  }
  if (key == "scene_preset3d") {
    Studio fig("scene_preset3d", Theme::paper());
    auto& pane = fig.pane3("scene_preset3d");
    const auto gx = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gy = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gz = make_grid_surface(24, 24);
    scene_preset3d(pane, ScenePreset3D::Studio);
    sheet(pane, gx, gy, gz).label("scene_preset3d");
    save(fig);
    return true;
  }
  if (key == "scene_preset3d_presentation") {
    Studio fig("scene_preset3d_presentation", Theme::paper());
    auto& pane = fig.pane3("scene_preset3d_presentation");
    const auto gx = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gy = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gz = make_grid_surface(24, 24);
    scene_preset3d(pane, ScenePreset3D::Presentation);
    auto& tr = sheet(pane, gx, gy, gz).label("scene_preset3d_presentation");
    trace_preset3d(tr, TracePreset3D::Surface);
    save(fig);
    return true;
  }
  if (key == "scene3d_custom") {
    Studio fig("scene3d_custom", Theme::paper());
    auto& pane = fig.pane3("scene3d_custom");
    const auto gx = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gy = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gz = make_grid_surface(24, 24);
    Scene3DOptions opts;
    opts.preset = ScenePreset3D::Studio;
    opts.theme = ThemePreset3D::Mist;
    opts.camera = Camera3DOptions{
        .azimuth_deg = -18.0,
        .elevation_deg = 52.0,
        .view_angle_deg = 30.0,
        .projection = ProjectionKind::Perspective,
    };
    opts.grid_style = GridStyle::Major;
    opts.lighting = Lighting3DOptions{
        .clear_first = true,
        .headlight = false,
        .lamp_left = true,
        .lamp_right = true,
        .sun_key = true,
    };
    scene3d(pane, opts);
    auto& tr = sheet(pane, gx, gy, gz).label("scene3d_custom");
    trace3d(tr, Trace3DOptions{.preset = TracePreset3D::Surface});
    save(fig);
    return true;
  }
  if (key == "trace3d") {
    Studio fig("trace3d", Theme::paper());
    auto& pane = fig.pane3("trace3d");
    const auto gx = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gy = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gz = make_grid_surface(24, 24);
    auto& tr = sheet(pane, gx, gy, gz).label("sheet");
    trace3d(tr, Trace3DOptions{.preset = TracePreset3D::Surface});
    save(fig);
    return true;
  }
  if (key == "trace3d_custom") {
    Studio fig("trace3d_custom", Theme::paper());
    auto& pane = fig.pane3("trace3d_custom");
    const auto gx = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gy = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gz = make_grid_surface(24, 24);
    scene_preset3d(pane, ScenePreset3D::Presentation);
    auto& tr = sheet(pane, gx, gy, gz).label("trace3d_custom");
    Trace3DOptions opts;
    opts.preset = TracePreset3D::Surface;
    opts.material = MaterialProfile::Metal;
    opts.transparent = true;
    trace3d(tr, opts);
    save(fig);
    return true;
  }
  if (key == "trace3d_wireframe") {
    Studio fig("trace3d_wireframe", Theme::paper());
    auto& pane = fig.pane3("trace3d_wireframe");
    const auto gx = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gy = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gz = make_grid_surface(24, 24);
    scene_preset3d(pane, ScenePreset3D::Survey);
    auto& tr = lattice(pane, gx, gy, gz).label("trace3d_wireframe");
    Trace3DOptions opts;
    opts.preset = TracePreset3D::Wireframe;
    opts.line_width = 2.2F;
    trace3d(tr, opts);
    save(fig);
    return true;
  }
  if (key == "trace_preset3d") {
    Studio fig("trace_preset3d", Theme::paper());
    auto& pane = fig.pane3("trace_preset3d");
    const auto gx = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gy = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gz = make_grid_surface(24, 24);
    auto& tr = sheet(pane, gx, gy, gz).label("sheet");
    trace_preset3d(tr, TracePreset3D::Surface);
    save(fig);
    return true;
  }
  if (key == "trace_preset3d_volume") {
    Studio fig("trace_preset3d_volume", Theme::paper());
    auto& pane = fig.pane3("trace_preset3d_volume");
    const auto gx = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gy = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gz = make_grid_surface(24, 24);
    scene_preset3d(pane, ScenePreset3D::Presentation);
    auto& tr = sheet(pane, gx, gy, gz).label("trace_preset3d_volume");
    trace_preset3d(tr, TracePreset3D::Volume);
    save(fig);
    return true;
  }
  if (key == "path3") {
    Studio fig("path3", Theme::paper());
    auto& pane = fig.pane3("path3");
    path3(pane,
          std::array<double, 6>{0.0, 0.4, 0.8, 1.2, 1.6, 2.0},
          std::array<double, 6>{0.0, 0.3, 0.9, 1.1, 1.4, 2.0},
          std::array<double, 6>{0.2, 0.5, 0.7, 1.1, 1.35, 1.8})
        .label("path3");
    save(fig);
    return true;
  }
  if (key == "cloud3") {
    Studio fig("cloud3", Theme::paper());
    auto& pane = fig.pane3("cloud3");
    cloud3(pane,
           std::array<double, 6>{0.2, 0.5, 0.9, 1.1, 1.5, 1.9},
           std::array<double, 6>{0.1, 0.4, 0.85, 1.0, 1.35, 1.8},
           std::array<double, 6>{0.3, 0.7, 0.9, 1.25, 1.52, 1.96},
           std::array<double, 6>{10.0, 12.0, 14.0, 16.0, 18.0, 20.0},
           std::array<double, 6>{0.1, 0.22, 0.4, 0.58, 0.73, 0.9})
        .label("cloud3");
    save(fig);
    return true;
  }
  if (key == "sheet") {
    Studio fig("sheet", Theme::paper());
    auto& pane = fig.pane3("sheet");
    const auto gx = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gy = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gz = make_grid_surface(24, 24);
    sheet(pane, gx, gy, gz).label("sheet");
    save(fig);
    return true;
  }
  if (key == "lattice") {
    Studio fig("lattice", Theme::paper());
    auto& pane = fig.pane3("lattice");
    const auto gx = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gy = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gz = make_grid_surface(24, 24);
    lattice(pane, gx, gy, gz).label("lattice");
    save(fig);
    return true;
  }
  if (key == "ridge3") {
    Studio fig("ridge3", Theme::paper());
    auto& pane = fig.pane3("ridge3");
    const auto gx = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gy = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gz = make_grid_surface(24, 24);
    ridge3(pane, gx, gy, gz, 8).label("ridge3");
    save(fig);
    return true;
  }
  if (key == "spire3") {
    Studio fig("spire3", Theme::paper());
    auto& pane = fig.pane3("spire3");
    auto zs = make_series(
        std::vector<double>{0.4,0.8,1.2,0.9, 0.7,1.1,1.6,1.3, 0.9,1.4,2.0,1.7, 0.6,1.0,1.5,1.2},
        {4,4});
    spire3(pane, std::array<double, 4>{1,2,3,4}, std::array<double, 4>{1,2,3,4}, zs).label("spire3");
    save(fig);
    return true;
  }
  if (key == "isoshell") {
    Studio fig("isoshell", Theme::paper());
    auto& pane = fig.pane3("isoshell");
    auto values = make_series(
        std::vector<double>{
            0.0, 0.1, 0.2, 0.12, 0.4, 0.75, 0.25, 0.72, 1.05,
            0.1, 0.22, 0.35, 0.24, 0.65, 1.02, 0.41, 0.98, 1.36,
            0.2, 0.33, 0.48, 0.36, 0.82, 1.2, 0.57, 1.18, 1.62,
        },
        {3, 3, 3});
    isoshell(pane,
             std::array<double, 3>{0.0, 1.0, 2.0},
             std::array<double, 3>{0.0, 1.0, 2.0},
             std::array<double, 3>{0.0, 1.0, 2.0},
             values,
             4)
        .label("isoshell");
    save(fig);
    return true;
  }
  if (key == "path4") {
    Studio fig("path4", Theme::paper());
    auto& pane = fig.pane3("path4");
    const auto hyper = make_hyper_curve(480);
    HyperPlotOptions opts;
    opts.projection = HyperProjection::PerspectiveW;
    opts.rot_xw_deg = 30.0;
    opts.rot_yz_deg = 15.0;
    opts.rot_zw_deg = 22.0;
    opts.w_perspective_distance = 3.4;
    path4(pane, hyper.hx, hyper.hy, hyper.hz, hyper.hw, opts).label("path4");
    save(fig);
    return true;
  }
  if (key == "cloud4") {
    Studio fig("cloud4", Theme::paper());
    auto& pane = fig.pane3("cloud4");
    const auto hyper = make_hyper_curve(480);
    HyperPlotOptions opts;
    opts.projection = HyperProjection::PerspectiveW;
    opts.rot_xy_deg = 12.0;
    opts.rot_xw_deg = 26.0;
    opts.rot_yw_deg = 20.0;
    opts.rot_zw_deg = 35.0;
    opts.w_perspective_distance = 3.8;
    opts.w_slice = Range1d{-0.55, 0.55};
    opts.point_size = 14.0;
    cloud4(pane, hyper.hx, hyper.hy, hyper.hz, hyper.hw, opts).label("cloud4");
    save(fig);
    return true;
  }
  if (key == "hyper_animation_sequence_preview") {
    Studio fig("hyper_animation_sequence_preview", Theme::paper());
    auto& pane = fig.pane3("hyper_animation_sequence_preview");
    const auto hyper = make_hyper_curve(480);
    HyperPlotOptions base;
    base.projection = HyperProjection::PerspectiveW;
    base.rot_yz_deg = 15.0;
    base.rot_zw_deg = 24.0;
    base.w_perspective_distance = 3.6;
    HyperSweepOptions sweep;
    sweep.frames = 5;
    sweep.plane = HyperSweepPlane::XW;
    sweep.start_deg = 0.0;
    sweep.end_deg = 60.0;
    sweep.include_endpoint = true;
    const auto frames = hyper_animation_sequence(base, sweep);
    cloud4(pane, hyper.hx, hyper.hy, hyper.hz, hyper.hw, frames.at(2)).label("hyper_animation_sequence_preview");
    save(fig);
    return true;
  }
  if (key == "animate3_example1_companion") {
    Studio fig("animate3_example1_companion", Theme::paper());
    auto& pane = fig.pane3("animate3_example1_companion");
    camera3d(
        pane,
        Camera3DOptions{
            .azimuth_deg = 30.0,
            .elevation_deg = -14.0,
            .view_angle_deg = 30.0,
            .projection = ProjectionKind::Perspective,
        });
    auto& tr = path3(pane, x, y, z).label("animate3_example1_companion");
    trace_preset3d(tr, TracePreset3D::Path);
    save(fig);
    return true;
  }
  if (key == "animate3_example2_companion") {
    Studio fig("animate3_example2_companion", Theme::paper());
    auto& pane = fig.pane3("animate3_example2_companion");
    camera3d(
        pane,
        Camera3DOptions{
            .azimuth_deg = 30.0,
            .elevation_deg = -14.0,
            .view_angle_deg = 30.0,
            .projection = ProjectionKind::Perspective,
        });
    auto sizes = Series::full({x.numel()}, 7.0, ScalarType::Float64);
    auto colors = series::linspace(0.15, 0.95, x.numel(), ScalarType::Float64);
    auto& tr = cloud3(pane, x, y, z, sizes, colors).label("animate3_example2_companion");
    trace_preset3d(tr, TracePreset3D::Cloud);
    save(fig);
    return true;
  }
  if (key == "animate4_example1_companion") {
    Studio fig("animate4_example1_companion", Theme::paper());
    auto& pane = fig.pane3("animate4_example1_companion");
    const auto hyper = make_hyper_curve(480);
    HyperPlotOptions opts;
    opts.projection = HyperProjection::PerspectiveW;
    opts.rot_xy_deg = 8.0;
    opts.rot_yz_deg = 15.0;
    opts.rot_zw_deg = 24.0;
    opts.w_perspective_distance = 3.6;
    path4(pane, hyper.hx, hyper.hy, hyper.hz, hyper.hw, opts).label("animate4_example1_companion");
    save(fig);
    return true;
  }
  if (key == "animate4_example2_companion") {
    Studio fig("animate4_example2_companion", Theme::paper());
    auto& pane = fig.pane3("animate4_example2_companion");
    const auto hyper = make_hyper_curve(480);
    HyperPlotOptions opts;
    opts.projection = HyperProjection::PerspectiveW;
    opts.rot_xy_deg = 8.0;
    opts.rot_xw_deg = 20.0;
    opts.rot_yz_deg = 15.0;
    opts.rot_zw_deg = 24.0;
    opts.w_perspective_distance = 3.6;
    opts.w_slice = Range1d{-0.42, 0.42};
    opts.point_size = 10.0;
    cloud4(pane, hyper.hx, hyper.hy, hyper.hz, hyper.hw, opts).label("animate4_example2_companion");
    save(fig);
    return true;
  }
  if (key == "export4_frames") {
    const auto hyper = make_hyper_curve(360);

    const auto out_dir = out_svg.parent_path() / "export4_frames_tmp";
    HyperFrameExportOptions opts;
    opts.output_dir = out_dir;
    opts.file_prefix = "frame";
    opts.format = AnimationFrameFormat::Svg;
    opts.width = 960;
    opts.height = 620;
    opts.sweep.frames = 3;
    opts.sweep.plane = HyperSweepPlane::XW;
    opts.sweep.start_deg = 0.0;
    opts.sweep.end_deg = 45.0;
    opts.sweep.include_endpoint = true;
    const auto n = export4_frames(hyper.hx, hyper.hy, hyper.hz, hyper.hw, opts);
    if (n <= 0) {
      return false;
    }
    const auto frame0 = out_dir / "frame_0.svg";
    if (!std::filesystem::exists(frame0)) {
      return false;
    }
    std::error_code ec;
    std::filesystem::copy_file(frame0, out_svg, std::filesystem::copy_options::overwrite_existing, ec);
    return !ec;
  }
  if (key == "export4_frames_frame2") {
    const auto hyper = make_hyper_curve(360);
    const auto out_dir = out_svg.parent_path() / "export4_frames_tmp_frame2";
    HyperFrameExportOptions opts;
    opts.output_dir = out_dir;
    opts.file_prefix = "frame";
    opts.format = AnimationFrameFormat::Svg;
    opts.width = 960;
    opts.height = 620;
    opts.sweep.frames = 4;
    opts.sweep.plane = HyperSweepPlane::YW;
    opts.sweep.start_deg = -30.0;
    opts.sweep.end_deg = 30.0;
    opts.sweep.include_endpoint = true;
    const auto n = export4_frames(hyper.hx, hyper.hy, hyper.hz, hyper.hw, opts);
    if (n <= 0) {
      return false;
    }
    const auto frame2 = out_dir / "frame_2.svg";
    if (!std::filesystem::exists(frame2)) {
      return false;
    }
    std::error_code ec;
    std::filesystem::copy_file(frame2, out_svg, std::filesystem::copy_options::overwrite_existing, ec);
    return !ec;
  }
  if (key == "plot3D") {
    Studio fig("plot3D", Theme::paper());
    auto& pane = fig.pane3("plot3D");
    plot3D(
        pane,
        std::array<double, 6>{0.0, 0.4, 0.8, 1.2, 1.6, 2.0},
        std::array<double, 6>{0.0, 0.3, 0.9, 1.1, 1.4, 2.0},
        std::array<double, 6>{0.2, 0.5, 0.7, 1.1, 1.35, 1.8})
        .label("plot3D");
    save(fig);
    return true;
  }
  if (key == "scatter3D") {
    Studio fig("scatter3D", Theme::paper());
    auto& pane = fig.pane3("scatter3D");
    scatter3D(
        pane,
        std::array<double, 6>{0.2, 0.5, 0.9, 1.1, 1.5, 1.9},
        std::array<double, 6>{0.1, 0.4, 0.85, 1.0, 1.35, 1.8},
        std::array<double, 6>{0.3, 0.7, 0.9, 1.25, 1.52, 1.96})
        .label("scatter3D");
    save(fig);
    return true;
  }
  if (key == "plot_surface") {
    Studio fig("plot_surface", Theme::paper());
    auto& pane = fig.pane3("plot_surface");
    const auto gx = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gy = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gz = make_grid_surface(24, 24);
    plot_surface(pane, gx, gy, gz).label("plot_surface");
    save(fig);
    return true;
  }
  if (key == "plot_wireframe") {
    Studio fig("plot_wireframe", Theme::paper());
    auto& pane = fig.pane3("plot_wireframe");
    const auto gx = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gy = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gz = make_grid_surface(24, 24);
    plot_wireframe(pane, gx, gy, gz).label("plot_wireframe");
    save(fig);
    return true;
  }
  if (key == "contour3D") {
    Studio fig("contour3D", Theme::paper());
    auto& pane = fig.pane3("contour3D");
    const auto gx = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gy = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gz = make_grid_surface(24, 24);
    contour3D(pane, gx, gy, gz, 10).label("contour3D");
    save(fig);
    return true;
  }
  if (key == "bar3d") {
    Studio fig("bar3d", Theme::paper());
    auto& pane = fig.pane3("bar3d");
    auto zs = make_series(
        std::vector<double>{0.4,0.8,1.2,0.9, 0.7,1.1,1.6,1.3, 0.9,1.4,2.0,1.7, 0.6,1.0,1.5,1.2},
        {4,4});
    bar3d(pane, std::array<double, 4>{1,2,3,4}, std::array<double, 4>{1,2,3,4}, zs).label("bar3d");
    save(fig);
    return true;
  }
  if (key == "hist3d") {
    Studio fig("hist3d", Theme::paper());
    auto& pane = fig.pane3("hist3d");
    hist3d(pane, x, y, Hist3DOptions{.bins_x = 28, .bins_y = 22, .as_surface = true}).label("hist3d");
    save(fig);
    return true;
  }

  if (key == "relgridplot") {
    Studio fig("relgridplot", Theme::paper());
    relgridplot(
        fig,
        {x, x, x},
        {y, y.mul(0.8), y.mul(0.6).add(0.2)},
        "scatter");
    save(fig);
    return true;
  }
  if (key == "catgridplot") {
    Studio fig("catgridplot", Theme::paper());
    const Note cats = make_note({"A", "A", "A", "B", "B", "B", "C", "C", "C"});
    const Series vals = make_series({0.9, 1.2, 1.4, 1.1, 1.6, 1.9, 0.7, 0.8, 1.0});
    catgridplot(fig, {cats, cats, cats}, {vals, vals.add(0.3), vals.sub(0.2)});
    save(fig);
    return true;
  }
  if (key == "disgridplot") {
    Studio fig("disgridplot", Theme::paper());
    disgridplot(fig, {y, y.add(0.25), y.sub(0.3)}, "hist");
    save(fig);
    return true;
  }
  if (key == "facetgrid") {
    Studio fig("facetgrid", Theme::paper());
    FacetGrid(fig, 2, 2, 14.0, 14.0);
    for (std::int64_t r = 0; r < 2; ++r) {
      for (std::int64_t c = 0; c < 2; ++c) {
        auto& pane = fig.pane2();
        pane.place(r, c);
        lineplot(pane, x, y.mul(0.6 + 0.2 * static_cast<double>(r + c))).label("facet");
      }
    }
    save(fig);
    return true;
  }
  if (key == "pairgrid") {
    Studio fig("pairgrid", Theme::paper());
    PairGrid(fig, 3, 12.0, 12.0);
    for (std::int64_t r = 0; r < 3; ++r) {
      for (std::int64_t c = 0; c < 3; ++c) {
        auto& pane = fig.pane2();
        pane.place(r, c);
        if (r == c) {
          histplot(pane, columns[static_cast<std::size_t>(r)], 16).label("diag");
        } else {
          scatterplot(
              pane,
              columns[static_cast<std::size_t>(c)],
              columns[static_cast<std::size_t>(r)])
              .label("off-diag");
        }
      }
    }
    save(fig);
    return true;
  }
  if (key == "jointgrid") {
    Studio fig("jointgrid", Theme::paper());
    JointGrid(fig, 12.0, 12.0);
    auto& top = fig.pane2();
    top.place(0, 0);
    histplot(top, x, 18).label("top");
    auto& joint = fig.pane2();
    joint.place(1, 0);
    scatterplot(joint, x, y).label("joint");
    auto& right = fig.pane2();
    right.place(1, 1);
    histplot(right, y, 18).label("right");
    fig.pane2().place(0, 1).hide();
    save(fig);
    return true;
  }
  if (key == "facet_map_plot") {
    Studio fig("facet_map_plot", Theme::paper());
    facet_map_plot(
        fig,
        2,
        2,
        [&](Pane& pane, std::int64_t row, std::int64_t col) {
          lineplot(
              pane,
              x,
              y.mul(0.55 + 0.15 * static_cast<double>(row + col))).label("facet-map");
        });
    save(fig);
    return true;
  }
  if (key == "pairgrid_map_plot") {
    Studio fig("pairgrid_map_plot", Theme::paper());
    pairgrid_map_plot(
        fig,
        3,
        [&](Pane& pane, std::int64_t row, std::int64_t col, bool diagonal) {
          if (diagonal) {
            histplot(pane, columns[static_cast<std::size_t>(row)], 16).label("diag");
          } else {
            scatterplot(
                pane,
                columns[static_cast<std::size_t>(col)],
                columns[static_cast<std::size_t>(row)])
                .label("off-diag");
          }
        });
    save(fig);
    return true;
  }
  if (key == "jointgrid_map_plot") {
    Studio fig("jointgrid_map_plot", Theme::paper());
    jointgrid_map_plot(
        fig,
        [&](Pane& joint) {
          scatterplot(joint, x, y).label("joint");
        },
        [&](Pane& marginal, bool top_axis) {
          if (top_axis) {
            histplot(marginal, x, 20).label("top");
          } else {
            histplot(marginal, y, 20).label("right");
          }
        });
    save(fig);
    return true;
  }

  // --------------------------
  // Core Studio / Pane / Trace API captures
  // --------------------------
  if (key == "core_studio_mosaic") {
    Studio fig("core_studio_mosaic", Theme::paper());
    fig.rename("core_mosaic").headline("Mosaic + placement").size(1240.0, 860.0);
    fig.mosaic(2, 2, 14.0, 14.0);

    auto& p00 = fig.pane2("trend").place(0, 0).grid(GridStyle::Major);
    p00.trend(x, y).label("signal");

    auto& p01 = fig.pane2("columns").place(0, 1);
    p01.x_categories({"A", "B", "C", "D"});
    p01.columns(make_series({1.3, 2.1, 1.8, 2.4})).label("bars");

    auto& p10 = fig.pane2("wedge").place(1, 0);
    p10.wedge(make_series({18.0, 27.0, 21.0, 34.0}), {"NLP", "Risk", "Ops", "Infra"}).label("mix");

    auto& p11 = fig.pane2("ribbon").place(1, 1);
    p11.ribbon(x, y.mul(0.65), -0.2).label("band");

    save(fig);
    return true;
  }
  if (key == "core_studio_theme") {
    Studio fig("core_studio_theme", Theme::paper());
    fig.theme(themes::ocean()).headline("Theme::ocean()");
    auto& pane = fig.pane2("theme");
    pane.grid(GridStyle::MajorMinor).x_label("time").y_label("value");
    pane.trend(x, y).label("signal");
    pane.cloud(x, y.add(0.08), y.abs().mul(12.0).add(6.0), y).label("points");
    save(fig);
    return true;
  }
  if (key == "core_studio_context_scales") {
    Studio fig("core_studio_context_scales", Theme::paper());
    fig.context_scales(1.22, 1.28).size(1120.0, 720.0);
    auto& pane = fig.pane2("context_scales");
    pane.x_label("x").y_label("y").grid(GridStyle::Major);
    pane.trend(x, y).label("scaled text");
    save(fig);
    return true;
  }
  if (key == "core_studio_pane_style_defaults") {
    Pane defaults(PaneKind::Planar, Theme::paper(), "defaults");
    defaults.grid(GridStyle::MajorMinor)
        .legend(KeyPlace::NorthEast)
        .legend_frame(KeyFrame::Framed)
        .legend_shape(KeyShape::Vertical)
        .tick_font_scale(1.12)
        .axis_label_font_scale(1.16)
        .x_major_grid(true)
        .y_major_grid(true);

    Studio fig("core_studio_pane_style_defaults", Theme::paper());
    fig.mosaic(1, 2, 12.0, 12.0).pane_style_defaults(defaults);
    auto& left = fig.pane2("inherits defaults");
    left.place(0, 0).x_label("x").y_label("y");
    left.trend(x, y).label("defaulted");

    fig.clear_pane_style_defaults();
    auto& right = fig.pane2("after clear");
    right.place(0, 1).x_label("x").y_label("y");
    right.trend(x, y.mul(0.6).add(0.2)).label("manual");
    save(fig);
    return true;
  }
  if (key == "core_studio_captions_row_titles") {
    Studio fig("core_studio_captions_row_titles", Theme::paper());
    fig.mosaic(2, 1, 12.0, 12.0);
    fig.row_title(0, "Signals");
    fig.row_title(1, "Diagnostics");
    fig.clear_row_title(1);
    fig.row_title(1, "Diagnostics");
    fig.x_caption("shared_xy", "Shared X Axis");
    fig.y_caption("shared_xy", "Shared Y Axis");

    auto& top = fig.pane2("top");
    top.place(0, 0).align_xy("shared_xy");
    top.trend(x, y).label("top");

    auto& bottom = fig.pane2("bottom");
    bottom.place(1, 0).align_xy("shared_xy");
    bottom.trend(x, y.mul(0.55).add(0.22)).label("bottom");
    save(fig);
    return true;
  }
  if (key == "core_studio_subpane2") {
    Studio fig("core_studio_subpane2", Theme::paper());
    fig.mosaic(1, 1, 12.0, 12.0);

    auto& parent = fig.pane2("parent").place(0, 0).caption("subpane2 canvas");
    parent.trend(x, y).label("parent trend");
    const std::size_t root = fig.pane_index(parent);

    auto& top = fig.subpane2(root, 2, 2, 0, 0, 1, 2, "top");
    top.trend(x, y.mul(0.7)).label("top");
    auto& bl = fig.subpane2(root, 2, 2, 1, 0, 1, 1, "bl");
    bl.columns(make_series({2.0, 4.0, 3.0})).label("bl");
    auto& br = fig.subpane2(root, 2, 2, 1, 1, 1, 1, "br");
    br.bucket(y, 14).label("br");
    save(fig);
    return true;
  }
  if (key == "core_studio_subpane3") {
    Studio fig("core_studio_subpane3", Theme::paper());
    fig.mosaic(1, 1, 12.0, 12.0);

    auto& parent = fig.pane3("parent3").place(0, 0);
    const std::size_t root = fig.pane_index(parent);

    const auto gx = series::linspace(-2.4, 2.4, 20, ScalarType::Float64);
    const auto gy = series::linspace(-2.4, 2.4, 20, ScalarType::Float64);
    const auto gz = make_grid_surface(20, 20);

    auto& a = fig.subpane3(root, 1, 2, 0, 0, 1, 1, "sheet");
    a.sheet(gx, gy, gz).label("sheet");
    auto& b = fig.subpane3(root, 1, 2, 0, 1, 1, 1, "lattice");
    b.lattice(gx, gy, gz).label("lattice");
    save(fig);
    return true;
  }
  if (key == "core_studio_tight_layout") {
    Studio fig("core_studio_tight_layout", Theme::paper());
    fig.mosaic(1, 3, 8.0, 8.0);

    auto& p0 = fig.pane2("left pane with long title");
    p0.place(0, 0).x_label("time (days)").y_label("return");
    p0.trend(x, y).label("left");

    auto& p1 = fig.pane2("center pane");
    p1.place(0, 1).x_label("time (days)").y_label("return");
    p1.trend(x, y.mul(0.7).add(0.15)).label("center");

    auto& p2 = fig.pane2("right pane");
    p2.place(0, 2).x_label("time (days)").y_label("return");
    p2.trend(x, y.mul(0.5).add(0.25)).label("right");

    fig.tight_layout(true, 8.0);
    save(fig);
    return true;
  }
  if (key == "core_studio_exhaustive") {
    Pane defaults(PaneKind::Planar, Theme::paper(), "defaults");
    defaults.grid(GridStyle::Major)
        .x_major_grid(true)
        .y_major_grid(true)
        .tick_font_scale(1.1)
        .axis_label_font_scale(1.1);

    Studio fig("core_studio_exhaustive", Theme::paper());
    fig.rename("core_studio_full")
        .headline("Studio exhaustive controls")
        .theme(themes::ocean())
        .context_scales(1.12, 1.16)
        .size(1280.0, 860.0)
        .mosaic(2, 2, 14.0, 14.0)
        .legend(KeyDock::East)
        .legend_frame(KeyFrame::Framed)
        .legend_shape(KeyShape::Vertical)
        .x_caption("xy_shared", "Shared X")
        .y_caption("xy_shared", "Shared Y")
        .z_caption("z_shared", "Shared Z")
        .row_title(0, "Top Row")
        .row_title(1, "Bottom Row")
        .clear_row_title(1)
        .row_title(1, "Bottom Row")
        .pane_style_defaults(defaults);

    const Theme paper_theme = Theme::paper();
    const Theme ocean_theme = Theme::ocean();
    const auto& paper_name = paper_theme.name();
    const auto& paper_accents = paper_theme.accents();
    const auto& paper_bg = paper_theme.background();
    const auto& paper_fg = paper_theme.foreground();
    const auto& paper_surface = paper_theme.surface_palette();
    const auto paper_accent0 = paper_theme.accent(0);
    const auto& ocean_name = ocean_theme.name();

    const PaletteRamp ramp_neutral = PaletteRamp::neutral();
    const PaletteRamp ramp_thermal = PaletteRamp::thermal();
    const PaletteRamp ramp_lagoon = PaletteRamp::lagoon();
    const PaletteRamp ramp_ocean = PaletteRamp::ocean();
    const PaletteRamp ramp_meadow = PaletteRamp::meadow();
    const auto& ramp_name = ramp_ocean.name();
    const auto& ramp_stops = ramp_ocean.stops();
    const bool ramp_empty = ramp_ocean.empty();
    const auto ramp_sample_mid = ramp_ocean.sample(0.5);

    Lens lens = Lens::spatial();
    lens.look(-45.0, 26.0)
        .spin(6.0, -2.0)
        .target_at(Vec3d{0.2, -0.1, 0.0})
        .up(Vec3d{0.0, 0.0, 1.0})
        .project(ProjectionKind::Perspective)
        .view_angle(34.0);
    const Lens planar_lens = Lens::planar();
    const auto lens_az = lens.azimuth_deg();
    const auto lens_el = lens.elevation_deg();
    const auto lens_view = lens.view_angle_deg();
    const auto lens_proj = lens.projection();
    const auto& lens_target = lens.target();
    const auto& lens_up = lens.up_vector();

    LightRig rig;
    rig.headlight().lamp_left().lamp_right().sun_key();
    rig.add(LightSpec{25.0, 35.0, LightSpace::Camera, LightStyle::Local});
    const bool rig_empty_before = rig.empty();
    const auto& rig_lights = rig.lights();
    rig.clear();
    const bool rig_empty_after = rig.empty();

    auto& p00 = fig.pane2("p00");
    p00.place(0, 0).align_xy("xy_shared");
    auto& trend_trace = p00.trend(x, y).label("trend");
    trend_trace.hide().show().shade(ShadeMode::Smooth).shade(ShadeMode::Faceted);
    const auto trace_kind = trend_trace.kind();
    const auto& trace_payload = trend_trace.payload();
    const auto& trace_label = trend_trace.label_text();
    const auto trace_width = trend_trace.stroke_width();
    const auto& trace_stroke = trend_trace.stroke_color();
    const auto& trace_palette = trend_trace.palette_ramp();
    const auto trace_shade_mode = trend_trace.shade_mode();
    const auto trace_lighting = trend_trace.lighting_mode();
    const auto trace_material = trend_trace.material_profile();
    const bool trace_visible = trend_trace.visible();
    const auto parent = fig.pane_index(p00);

    auto& p01 = fig.pane2("p01");
    p01.place(0, 1).align_xy("xy_shared");
    p01.columns(make_series({2.0, 3.0, 4.0, 3.2})).label("bars");

    auto& p10 = fig.pane3("p10");
    p10.place(1, 0).align_z("z_shared");
    const auto parent3 = fig.pane_index(p10);
    const auto gx = series::linspace(-2.5, 2.5, 18, ScalarType::Float64);
    const auto gy = series::linspace(-2.5, 2.5, 18, ScalarType::Float64);
    const auto gz = make_grid_surface(18, 18);
    p10.sheet(gx, gy, gz).label("sheet");

    auto& p11 = fig.pane3("p11");
    p11.place(1, 1).align_z("z_shared");
    p11.lattice(gx, gy, gz).label("lattice");

    auto& nested = fig.subpane2(parent, 1, 2, 0, 0, 1, 1, "nested");
    nested.columns(make_series({1.0, 2.0, 1.5})).label("nested");

    auto& nested3 = fig.subpane3(parent3, 1, 1, 0, 0, 1, 1, "nested3");
    nested3.cloud3(
        std::array<double, 6>{0.2, 0.5, 0.9, 1.1, 1.5, 1.9},
        std::array<double, 6>{0.1, 0.4, 0.85, 1.0, 1.35, 1.8},
        std::array<double, 6>{0.3, 0.7, 0.9, 1.25, 1.52, 1.96})
        .label("nested3");

    const auto& fig_name = fig.name();
    const auto& fig_headline = fig.headline_text();
    const auto& fig_theme = fig.theme_tokens();
    const auto& fig_panes = fig.panes();
    const auto fig_pane_count = fig.pane_count();
    const auto& fig_extent = fig.extent();
    const auto& fig_mosaic = fig.mosaic_layout();
    const auto fig_tight = fig.tight_layout_enabled();
    const auto fig_tight_padding = fig.tight_layout_padding();
    const auto fig_tick_scale = fig.default_tick_font_scale();
    const auto fig_axis_scale = fig.default_axis_label_font_scale();
    const auto fig_legend_dock = fig.legend_dock();
    const auto& fig_x_captions = fig.x_captions();
    const auto& fig_y_captions = fig.y_captions();
    const auto& fig_z_captions = fig.z_captions();
    const auto& fig_row_titles = fig.row_titles();
    const auto fig_svg = fig.to_svg();
    const auto fig_pdf = fig.to_pdf();
    fig.mosaic(2, 2, 14.0, 14.0);
    fig.row_title(0, "Top Row");

    Pane scratch(PaneKind::Planar, Theme::paper(), "scratch");
    scratch.scale_x(ScalePolicy::linear())
        .x_limits({-0.2, 12.2})
        .x_tick_format(TickFormat::Auto)
        .x_tick_rotation(0.0)
        .x_ticks(make_series({0.0, 6.0, 12.0}), {"start", "mid", "end"})
        .grid(GridStyle::Major);

    const auto pane_kind = p00.kind();
    const auto& pane_title = p00.title_text();
    const auto& pane_theme = p00.theme_tokens();
    const auto& pane_x_axis = p00.x_axis();
    const auto& pane_y_axis = p00.y_axis();
    const auto& pane_z_axis = p00.z_axis();
    const auto& pane_color_axis = p00.color_axis();
    const auto pane_legend = p00.legend_place();
    const auto pane_grid_style = p00.grid_style();
    const auto& pane_lens = p00.lens();
    const auto& pane_lights = p00.lights();
    const auto& pane_traces = p00.traces();
    const auto pane_trace_count = p00.trace_count();
    const auto pane_scale_guide = p00.scale_guide_enabled();
    const auto& pane_caption = p00.caption_text();
    const auto& pane_rules = p00.rules();
    const auto& pane_bands = p00.bands();
    const auto& pane_pins = p00.pins();
    const auto& pane_x_align = p00.x_align_group();
    const auto& pane_y_align = p00.y_align_group();
    const auto& pane_z_align = p00.z_align_group();
    const auto& pane_place = p00.placement();
    const auto pane_visible = p00.visible();

    (void)paper_name;
    (void)paper_accents;
    (void)paper_bg;
    (void)paper_fg;
    (void)paper_surface;
    (void)paper_accent0;
    (void)ocean_name;
    (void)ramp_neutral;
    (void)ramp_thermal;
    (void)ramp_lagoon;
    (void)ramp_ocean;
    (void)ramp_meadow;
    (void)ramp_name;
    (void)ramp_stops;
    (void)ramp_empty;
    (void)ramp_sample_mid;
    (void)planar_lens;
    (void)lens_az;
    (void)lens_el;
    (void)lens_view;
    (void)lens_proj;
    (void)lens_target;
    (void)lens_up;
    (void)rig_empty_before;
    (void)rig_lights;
    (void)rig_empty_after;
    (void)trace_kind;
    (void)trace_payload;
    (void)trace_label;
    (void)trace_width;
    (void)trace_stroke;
    (void)trace_palette;
    (void)trace_shade_mode;
    (void)trace_lighting;
    (void)trace_material;
    (void)trace_visible;
    (void)fig_name;
    (void)fig_headline;
    (void)fig_theme;
    (void)fig_panes;
    (void)fig_pane_count;
    (void)fig_extent;
    (void)fig_mosaic;
    (void)fig_tight;
    (void)fig_tight_padding;
    (void)fig_tick_scale;
    (void)fig_axis_scale;
    (void)fig_legend_dock;
    (void)fig_x_captions;
    (void)fig_y_captions;
    (void)fig_z_captions;
    (void)fig_row_titles;
    (void)fig_svg;
    (void)fig_pdf;
    (void)pane_kind;
    (void)pane_title;
    (void)pane_theme;
    (void)pane_x_axis;
    (void)pane_y_axis;
    (void)pane_z_axis;
    (void)pane_color_axis;
    (void)pane_legend;
    (void)pane_grid_style;
    (void)pane_lens;
    (void)pane_lights;
    (void)pane_traces;
    (void)pane_trace_count;
    (void)pane_scale_guide;
    (void)pane_caption;
    (void)pane_rules;
    (void)pane_bands;
    (void)pane_pins;
    (void)pane_x_align;
    (void)pane_y_align;
    (void)pane_z_align;
    (void)pane_place;
    (void)pane_visible;

    fig.clear_pane_style_defaults().tight_layout(true, 8.0);
    const bool shown = fig.show("core_studio_exhaustive", 900, 620, false);
    (void)shown;
    save(fig);
    return true;
  }
  if (key == "core_pane_axes_limits") {
    Studio fig("core_pane_axes_limits", Theme::paper());
    auto& pane = fig.pane2("axes + limits");
    pane.x_label("x").y_label("y");
    pane.x_limits({0.0, 12.0})
        .y_limits({-1.3, 1.3})
        .x_limit_method(LimitMethod::Tight)
        .y_limit_method(LimitMethod::Padded);
    pane.trend(x, y).label("signal");
    save(fig);
    return true;
  }
  if (key == "core_pane_tick_format") {
    Studio fig("core_pane_tick_format", Theme::paper());
    auto& pane = fig.pane2("tick format");
    pane.x_tick_format(TickFormat::Integer)
        .y_tick_format(TickFormat::Fixed3)
        .x_auto_tick_decimals(0)
        .y_auto_tick_decimals(3);
    pane.trend(x, y.mul(123.456)).label("formatted");
    save(fig);
    return true;
  }
  if (key == "core_pane_tick_rotation_gaps") {
    Studio fig("core_pane_tick_rotation_gaps", Theme::paper());
    auto& pane = fig.pane2("rotation + gaps");
    pane.x_categories({"Jan", "Feb", "Mar", "Apr", "May", "Jun"})
        .x_tick_rotation(30.0)
        .x_tick_label_gap(8.0)
        .x_label_gap(18.0)
        .x_label("month")
        .y_label("score")
        .y_label_centered(true);
    pane.columns(make_series({13.0, 16.0, 14.0, 18.0, 17.0, 19.0})).label("scores");
    save(fig);
    return true;
  }
  if (key == "core_pane_ticks_categories") {
    Studio fig("core_pane_ticks_categories", Theme::paper());
    auto& pane = fig.pane2("ticks + categories");
    pane.x_ticks(make_series({1.0, 2.0, 3.0, 4.0}), {"Q1", "Q2", "Q3", "Q4"});
    pane.y_categories({"low", "mid", "high"}, 0.0, 1.0);
    pane.columns(make_series({0.0, 1.0, 2.0, 1.0})).label("band");
    save(fig);
    return true;
  }
  if (key == "core_pane_grid_legend") {
    Studio fig("core_pane_grid_legend", Theme::paper());
    auto& pane = fig.pane2("grid + legend");
    pane.grid(GridStyle::MajorMinor)
        .legend(KeyPlace::NorthEast)
        .legend_frame(KeyFrame::Framed)
        .legend_shape(KeyShape::Vertical);
    pane.trend(x, y).label("trend");
    pane.cloud(x, y.add(0.1)).label("cloud");
    save(fig);
    return true;
  }
  if (key == "core_pane_guides_annotations") {
    Studio fig("core_pane_guides_annotations", Theme::paper());
    auto& pane = fig.pane2("rules + bands + pins");
    pane.rule_x(6.0, "event")
        .rule_y(0.0, "zero")
        .band_x({3.0, 5.0}, "window")
        .band_y({-0.25, 0.25}, "neutral")
        .pin("peak", 8.0, 1.0)
        .caption("overlay guides");
    pane.trend(x, y).label("signal");
    save(fig);
    return true;
  }
  if (key == "core_pane_alignment") {
    Studio fig("core_pane_alignment", Theme::paper());
    fig.mosaic(1, 2, 12.0, 12.0);
    auto& left = fig.pane2("left");
    left.place(0, 0).align_xy("shared_axes");
    left.trend(x, y).label("left");

    auto& right = fig.pane2("right");
    right.place(0, 1).align_xy("shared_axes");
    right.trend(x, y.mul(0.4).add(0.3)).label("right");
    save(fig);
    return true;
  }
  if (key == "core_pane_visibility") {
    Studio fig("core_pane_visibility", Theme::paper());
    fig.mosaic(1, 2, 12.0, 12.0);
    auto& left = fig.pane2("visible");
    left.place(0, 0).show().axes_visible(true);
    left.trend(x, y).label("visible");

    auto& right = fig.pane2("axes hidden");
    right.place(0, 1).visible(true).axes_visible(false);
    right.trend(x, y.mul(0.55).add(0.2)).label("axes off");
    save(fig);
    return true;
  }
  if (key == "core_pane_scales") {
    Studio fig("core_pane_scales", Theme::paper());
    auto& pane = fig.pane2("scale_x/log + scale_y/symlog");
    const auto xs = series::linspace(1.0, 120.0, 180, ScalarType::Float64);
    const auto ys = xs.mul(0.08).sin().mul(xs.mul(0.02).add(0.6));
    pane.scale_x(ScalePolicy::log10()).scale_y(ScalePolicy::symlog(10.0, 0.2));
    pane.trend(xs, ys).label("scaled");
    save(fig);
    return true;
  }
  if (key == "core_axis_layout_exhaustive") {
    Studio fig("core_axis_layout_exhaustive", Theme::paper());
    fig.size(1220.0, 780.0).mosaic(1, 1, 12.0, 12.0);
    auto& pane = fig.pane2("axis/layout exhaustive");
    const auto xs = series::linspace(1.0, 120.0, 180, ScalarType::Float64);
    const auto ys = xs.mul(0.08).sin().mul(xs.mul(0.02).add(0.6));

    pane.title("axis/layout exhaustive");
    pane.scale_x(ScalePolicy::log10()).scale_y(ScalePolicy::symlog(10.0, 0.2));
    pane.x_limits({1.0, 120.0}).y_limits({-2.0, 2.0}).color_limits({-1.0, 1.0});
    pane.x_limit_method(LimitMethod::TickAligned)
        .y_limit_method(LimitMethod::Padded)
        .color_limit_method(LimitMethod::Tight);
    pane.x_tick_format(TickFormat::Scientific).y_tick_format(TickFormat::Fixed3);
    pane.x_auto_tick_decimals(2).y_auto_tick_decimals(3);
    pane.clear_x_auto_tick_decimals().clear_y_auto_tick_decimals();
    pane.x_auto_tick_decimals(1).y_auto_tick_decimals(2);
    pane.tick_font_scale(1.08).axis_label_font_scale(1.10);

    pane.x_tick_rotation(18.0).y_tick_rotation(0.0);
    pane.x_tick_label_gap(7.0).y_tick_label_gap(6.0);
    pane.x_label_gap(16.0).y_label_gap(15.0);
    pane.x_label_rotation(0.0).y_label_rotation(90.0);
    pane.clear_x_tick_rotation().clear_y_tick_rotation();
    pane.clear_x_tick_label_gap().clear_y_tick_label_gap();
    pane.clear_x_label_gap().clear_y_label_gap();
    pane.clear_x_label_rotation().clear_y_label_rotation();
    pane.x_tick_rotation(16.0).x_tick_label_gap(6.0).x_label_gap(14.0);
    pane.y_tick_rotation(0.0).y_tick_label_gap(6.0).y_label_gap(14.0);

    pane.x_tick_labels(true).y_tick_labels(true);
    pane.auto_x().auto_y().auto_color();
    pane.x_label("time").y_label("response");
    pane.x_ticks(make_series({1.0, 10.0, 50.0, 120.0}), {"t1", "t10", "t50", "t120"});
    pane.y_ticks(make_series({-1.0, 0.0, 1.0}), {"low", "mid", "high"});
    pane.clear_x_ticks().clear_y_ticks();
    pane.x_categories({"Q1", "Q2", "Q3", "Q4"});
    pane.y_categories({"vlow", "low", "mid", "high"}, 0.0, 1.0);

    pane.legend(KeyPlace::NorthEast)
        .legend_frame(KeyFrame::Framed)
        .legend_shape(KeyShape::Vertical)
        .grid(GridStyle::MajorMinor)
        .x_major_grid(true)
        .y_major_grid(true)
        .x_minor_grid(true)
        .y_minor_grid(true)
        .scale_guide(false)
        .caption("axis controls")
        .rule_x(10.0, "x rule")
        .rule_y(0.0, "y rule")
        .band_x({2.0, 3.0}, "x band")
        .band_y({-0.2, 0.2}, "y band")
        .pin("marker", 12.0, 0.8)
        .align_x("grp_xy")
        .align_y("grp_xy")
        .align_xy("grp_xy")
        .parallel_xy_labels(true)
        .visible(true)
        .axes_visible(true)
        .hide()
        .show()
        .clear_x_align()
        .clear_y_align()
        .clear_align()
        .place(0, 0, 1, 1);

    pane.trend(xs, ys).label("trend");
    save(fig);
    return true;
  }
  if (key == "core_axis_layout_exhaustive_3d") {
    Studio fig("core_axis_layout_exhaustive_3d", Theme::paper());
    auto& pane = fig.pane3("axis/layout exhaustive 3d");
    const auto gx = series::linspace(-2.4, 2.4, 22, ScalarType::Float64);
    const auto gy = series::linspace(-2.4, 2.4, 22, ScalarType::Float64);
    const auto gz = make_grid_surface(22, 22);

    pane.title("axis/layout exhaustive 3d");
    pane.scale_z(ScalePolicy::linear());
    pane.z_limits({-2.0, 2.0}).color_limits({-1.0, 1.0});
    pane.z_limit_method(LimitMethod::Padded).color_limit_method(LimitMethod::TickAligned);
    pane.z_tick_format(TickFormat::Engineering);
    pane.z_auto_tick_decimals(2);
    pane.clear_z_auto_tick_decimals();
    pane.z_auto_tick_decimals(1);

    pane.z_tick_rotation(0.0)
        .z_tick_label_gap(6.0)
        .z_label_gap(16.0)
        .z_label_rotation(90.0)
        .clear_z_tick_rotation()
        .clear_z_tick_label_gap()
        .clear_z_label_gap()
        .clear_z_label_rotation()
        .z_tick_rotation(0.0)
        .z_tick_label_gap(6.0)
        .z_label_gap(14.0);

    pane.z_tick_labels(true).z_label("z");
    pane.z_ticks(make_series({-1.0, 0.0, 1.0}), {"low", "mid", "high"});
    pane.clear_z_ticks();
    pane.z_categories({"layer1", "layer2", "layer3"}, 0.0, 1.0);
    pane.z_major_grid(true).z_minor_grid(true).scale_guide(true);
    pane.rule_z(0.0, "slice").band_z({-0.2, 0.2}, "z band");
    pane.pin("3d pin", Vec3d{0.2, 0.2, 0.2});
    pane.align_z("grp3").align_xyz("grp3");
    pane.visible(true).axes_visible(true).auto_z().place(0, 0);
    pane.sheet(gx, gy, gz).label("sheet");
    save(fig);
    return true;
  }
  if (key == "core_trace_style_2d") {
    Studio fig("core_trace_style_2d", Theme::paper());
    auto& pane = fig.pane2("trace styling 2d");
    auto& trace = pane.trend(x, y);
    trace.label("styled")
        .width(2.8F)
        .stroke(Rgba::from_u8(178, 65, 37))
        .palette(PaletteRamp::ocean())
        .color_limits({-1.0, 1.0});
    pane.cloud(x, y.add(0.1), y.abs().mul(12.0).add(8.0), y).label("cloud");
    save(fig);
    return true;
  }
  if (key == "core_trace_style_3d") {
    Studio fig("core_trace_style_3d", Theme::paper());
    auto& pane = fig.pane3("trace styling 3d");
    const auto gx = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gy = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gz = make_grid_surface(24, 24);
    pane.sheet(gx, gy, gz)
        .label("sheet")
        .shade(ShadeMode::Smooth)
        .lighting(LightingMode::Gouraud)
        .material(MaterialProfile::Metal)
        .transparent(true)
        .annotate_levels(true);
    save(fig);
    return true;
  }
  if (key == "core_trace_visibility") {
    Studio fig("core_trace_visibility", Theme::paper());
    auto& pane = fig.pane2("trace visibility");
    pane.trend(x, y).label("visible");
    auto& muted = pane.trend(x, y.mul(0.35).add(0.35)).label("hidden then shown");
    muted.hide().show();
    pane.trend(x, y.mul(0.2).sub(0.4)).label("hidden final").hide();
    save(fig);
    return true;
  }
  if (key == "core_trend") {
    Studio fig("core_trend", Theme::paper());
    auto& pane = fig.pane2("trend");
    pane.trend(x, y).label("trend");
    save(fig);
    return true;
  }
  if (key == "core_cloud") {
    Studio fig("core_cloud", Theme::paper());
    auto& pane = fig.pane2("cloud");
    pane.cloud(x, y, y.abs().mul(18.0).add(8.0), y).label("cloud");
    save(fig);
    return true;
  }
  if (key == "core_cloud3") {
    Studio fig("core_cloud3", Theme::paper());
    auto& pane = fig.pane3("cloud3");
    pane.cloud3(x, y, z, y.abs().mul(18.0).add(8.0), y).label("cloud3");
    save(fig);
    return true;
  }
  if (key == "core_fieldmap") {
    Studio fig("core_fieldmap", Theme::paper());
    auto& pane = fig.pane2("fieldmap");
    const auto gx = series::linspace(-3.0, 3.0, 42, ScalarType::Float64);
    const auto gy = series::linspace(-3.0, 3.0, 42, ScalarType::Float64);
    const auto gz = make_grid_surface(42, 42);
    pane.scale_guide(true);
    pane.fieldmap(gx, gy, gz).label("fieldmap");
    save(fig);
    return true;
  }
  if (key == "core_ribbon") {
    Studio fig("core_ribbon", Theme::paper());
    auto& pane = fig.pane2("ribbon");
    pane.ribbon(x, y, -0.25).label("ribbon");
    save(fig);
    return true;
  }
  if (key == "core_spike") {
    Studio fig("core_spike", Theme::paper());
    auto& pane = fig.pane2("spike");
    pane.spike(x, y.abs(), 0.0).label("spike");
    save(fig);
    return true;
  }
  if (key == "core_wedge") {
    Studio fig("core_wedge", Theme::paper());
    auto& pane = fig.pane2("wedge");
    pane.wedge(make_series({15.0, 23.0, 18.0, 44.0}),
               make_series({0.02, 0.04, 0.01, 0.0}),
               {"A", "B", "C", "D"})
        .label("wedge");
    save(fig);
    return true;
  }
  if (key == "core_whisk") {
    Studio fig("core_whisk", Theme::paper());
    auto& pane = fig.pane2("whisk");
    const auto px = make_series({1.0, 2.0, 3.0, 4.0});
    const auto py = make_series({3.2, 3.9, 4.1, 4.8});
    const auto pe = make_series({0.15, 0.2, 0.17, 0.25});
    pane.whisk(px, py, pe).label("whisk");
    save(fig);
    return true;
  }
  if (key == "core_columns") {
    Studio fig("core_columns", Theme::paper());
    auto& pane = fig.pane2("columns");
    pane.x_categories({"infra", "models", "data", "ops"});
    pane.columns(make_series({21.0, 28.0, 24.0, 19.0})).label("columns");
    save(fig);
    return true;
  }
  if (key == "core_columns_h") {
    Studio fig("core_columns_h", Theme::paper());
    auto& pane = fig.pane2("columns_h");
    pane.y_categories({"A", "B", "C", "D"});
    pane.columns_h(make_series({6.0, 9.0, 11.0, 7.0})).label("columns_h");
    save(fig);
    return true;
  }
  if (key == "core_bucket") {
    Studio fig("core_bucket", Theme::paper());
    auto& pane = fig.pane2("bucket");
    pane.bucket(y, 16).label("bucket");
    save(fig);
    return true;
  }
  if (key == "core_quartet") {
    Studio fig("core_quartet", Theme::paper());
    auto& pane = fig.pane2("quartet");
    const auto groups = make_series({0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 2.0, 2.0, 2.0});
    const auto vals = make_series({0.9, 1.2, 1.4, 1.1, 1.6, 1.9, 0.7, 0.8, 1.0});
    pane.quartet(groups, vals).label("quartet");
    save(fig);
    return true;
  }
  if (key == "core_violin") {
    Studio fig("core_violin", Theme::paper());
    auto& pane = fig.pane2("violin");
    ViolinOptions opts;
    opts.inner = ViolinInner::Quartile;
    opts.scale = ViolinScale::Width;
    opts.side = ViolinSide::Full;
    pane.violin(make_series({0.9, 1.2, 1.4, 1.1, 1.6, 1.9, 0.7, 0.8, 1.0}), opts).label("violin");
    save(fig);
    return true;
  }
  if (key == "core_radial") {
    Studio fig("core_radial", Theme::paper());
    auto& pane = fig.pane2("radial");
    const auto theta = series::linspace(0.0, 2.0 * kPi, 180, ScalarType::Float64);
    const auto radius = theta.mul(3.0).sin().abs().add(0.35);
    pane.radial(theta, radius).label("radial");
    save(fig);
    return true;
  }
  if (key == "core_path3") {
    Studio fig("core_path3", Theme::paper());
    auto& pane = fig.pane3("path3");
    pane.path3(x, y, z).label("path3");
    save(fig);
    return true;
  }
  if (key == "core_spire3") {
    Studio fig("core_spire3", Theme::paper());
    auto& pane = fig.pane3("spire3");
    const auto zs = make_series(
        std::vector<double>{0.4, 0.8, 1.2, 0.9, 0.7, 1.1, 1.6, 1.3, 0.9, 1.4, 2.0, 1.7, 0.6, 1.0, 1.5, 1.2},
        {4, 4});
    pane.spire3(std::array<double, 4>{1.0, 2.0, 3.0, 4.0},
                std::array<double, 4>{1.0, 2.0, 3.0, 4.0},
                zs)
        .label("spire3");
    save(fig);
    return true;
  }
  if (key == "core_contour") {
    Studio fig("core_contour", Theme::paper());
    auto& pane = fig.pane2("contour");
    const auto gx = series::linspace(-2.5, 2.5, 26, ScalarType::Float64);
    const auto gy = series::linspace(-2.5, 2.5, 26, ScalarType::Float64);
    const auto gz = make_grid_surface(26, 26);
    pane.contour(gx, gy, gz, 10).label("contour");
    save(fig);
    return true;
  }
  if (key == "core_isoband") {
    Studio fig("core_isoband", Theme::paper());
    auto& pane = fig.pane2("isoband");
    const auto gx = series::linspace(-2.5, 2.5, 34, ScalarType::Float64);
    const auto gy = series::linspace(-2.5, 2.5, 34, ScalarType::Float64);
    const auto gz = make_grid_surface(34, 34);
    pane.isoband(gx, gy, gz, 8).label("isoband");
    save(fig);
    return true;
  }
  if (key == "core_ridge3") {
    Studio fig("core_ridge3", Theme::paper());
    auto& pane = fig.pane3("ridge3");
    const auto gx = series::linspace(-2.5, 2.5, 26, ScalarType::Float64);
    const auto gy = series::linspace(-2.5, 2.5, 26, ScalarType::Float64);
    const auto gz = make_grid_surface(26, 26);
    pane.ridge3(gx, gy, gz, 9).label("ridge3");
    save(fig);
    return true;
  }
  if (key == "core_isoshell") {
    Studio fig("core_isoshell", Theme::paper());
    auto& pane = fig.pane3("isoshell");
    auto values = make_series(
        std::vector<double>{
            0.0, 0.1, 0.2, 0.12, 0.4, 0.75, 0.25, 0.72, 1.05,
            0.1, 0.22, 0.35, 0.24, 0.65, 1.02, 0.41, 0.98, 1.36,
            0.2, 0.33, 0.48, 0.36, 0.82, 1.2, 0.57, 1.18, 1.62,
        },
        {3, 3, 3});
    pane.isoshell(std::array<double, 3>{0.0, 1.0, 2.0},
                  std::array<double, 3>{0.0, 1.0, 2.0},
                  std::array<double, 3>{0.0, 1.0, 2.0},
                  values,
                  4)
        .label("isoshell");
    save(fig);
    return true;
  }
  if (key == "core_sheet") {
    Studio fig("core_sheet", Theme::paper());
    auto& pane = fig.pane3("sheet");
    const auto gx = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gy = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gz = make_grid_surface(24, 24);
    pane.sheet(gx, gy, gz).label("sheet");
    save(fig);
    return true;
  }
  if (key == "core_lattice") {
    Studio fig("core_lattice", Theme::paper());
    auto& pane = fig.pane3("lattice");
    const auto gx = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gy = series::linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gz = make_grid_surface(24, 24);
    pane.lattice(gx, gy, gz).label("lattice");
    save(fig);
    return true;
  }
  if (key == "core_save_png") {
    Studio fig("core_save_png", Theme::paper());
    auto& pane = fig.pane2("save_png");
    pane.trend(x, y).label("signal");
    const auto png = out_svg.parent_path() / "core_save_png_export.png";
    fig.save_png(png.string(), 144);
    save(fig);
    return true;
  }
  if (key == "core_save_pdf") {
    Studio fig("core_save_pdf", Theme::paper());
    auto& pane = fig.pane2("save_pdf");
    pane.trend(x, y).label("signal");
    const auto pdf = out_svg.parent_path() / "core_save_pdf_export.pdf";
    fig.save_pdf(pdf.string());
    save(fig);
    return true;
  }
  if (key == "core_save_svg") {
    Studio fig("core_save_svg", Theme::paper());
    auto& pane = fig.pane2("save_svg");
    pane.trend(x, y).label("signal");
    const auto svg = out_svg.parent_path() / "core_save_svg_export.svg";
    fig.save_svg(svg.string());
    save(fig);
    return true;
  }
  return false;
}

} // namespace

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Usage: flash_api_capture_demo <method-key> <output.svg>\n";
    return 1;
  }

  const std::string_view key = argv[1];
  const std::filesystem::path out_svg = argv[2];
  if (!render_case(key, out_svg)) {
    std::cerr << "Unknown method key: " << key << "\n";
    return 2;
  }
  return 0;
}
