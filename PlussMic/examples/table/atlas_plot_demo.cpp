#include <pluss/pluss.h>

#include <filesystem>
#include <string_view>
#include <algorithm>
#include <vector>

int main() {
  using pluss::table::Atlas;
  using pluss::table::DataFrame;
  using pluss::table::AtlasDepthStatistic;
  using pluss::table::AtlasPlotOptions;

  const auto output_dir = std::filesystem::path(PLUSSMIC_SOURCE_DIR) / "examples" / "outputs" / "atlas_plot";
  std::filesystem::create_directories(output_dir);
  const auto out_path = [&output_dir](std::string_view name) {
    return (output_dir / std::string(name)).string();
  };

  const auto cases = pluss::make_note({"C1", "C2", "C3", "C4", "C5", "C6"});
  auto make_frame = [&](const std::vector<double>& close, const std::vector<double>& volume) {
    std::vector<double> range(close.size(), 0.0);
    const double base = close.empty() ? 0.0 : close.front();
    std::transform(close.begin(), close.end(), range.begin(), [base](double value) { return value - base; });
    auto frame = DataFrame({
        {"Close", pluss::make_series(close)},
        {"Volume", pluss::make_series(volume)},
        {"Range", pluss::make_series(range)},
    });
    return frame.labels().set(pluss::table::Slate(cases));
  };

  auto f0 = make_frame({102.0, 105.0, 108.0, 111.0, 109.0, 113.0},
                       {2.1e6, 2.4e6, 2.3e6, 2.8e6, 2.6e6, 2.9e6});
  auto f1 = make_frame({104.0, 107.0, 110.0, 114.0, 112.0, 116.0},
                       {2.3e6, 2.5e6, 2.6e6, 3.0e6, 2.8e6, 3.1e6});
  auto f2 = make_frame({101.0, 106.0, 109.0, 113.0, 111.0, 118.0},
                       {2.0e6, 2.6e6, 2.5e6, 2.9e6, 2.7e6, 3.2e6});
  auto f3 = make_frame({103.0, 108.0, 112.0, 116.0, 114.0, 121.0},
                       {2.2e6, 2.7e6, 2.8e6, 3.2e6, 3.0e6, 3.4e6});

  Atlas atlas(
      {f0, f1, f2, f3},
      pluss::table::Slate(pluss::make_note({"W1", "W2", "W3", "W4"})));

  AtlasPlotOptions plot_opts;
  plot_opts.figure.width = 1240.0;
  plot_opts.figure.height = 860.0;
  plot_opts.figure.use_tight_layout = true;
  plot_opts.figure.tight_layout_padding = 6.0;
  plot_opts.frames.mode = pluss::table::AtlasFrameSelectionMode::Overlay;
  plot_opts.frames.cols = 2;
  plot_opts.depth_axis.order = pluss::table::AtlasDepthOrder::Ascending;
  plot_opts.depth_axis.step = 1;
  plot_opts.surface.scene.preset = pluss::pl::flash::ScenePreset3D::Survey;
  plot_opts.wireframe.scene.preset = pluss::pl::flash::ScenePreset3D::Presentation;
  plot_opts.cloud3.scene.preset = pluss::pl::flash::ScenePreset3D::Studio;
  plot_opts.depth.center = AtlasDepthStatistic::Mean;
  plot_opts.depth.lower = AtlasDepthStatistic::Min;
  plot_opts.depth.upper = AtlasDepthStatistic::Max;
  plot_opts.depth_compare.statistic = AtlasDepthStatistic::Mean;
  plot_opts.depth_compare.line_width = 1.6F;
  plot_opts.depth_compare.max_features = 3;
  plot_opts.depth_facet.statistic = AtlasDepthStatistic::Median;
  plot_opts.depth_facet.cols = 2;
  plot_opts.depth_facet.max_features = 3;
  plot_opts.depth_rolling.statistic = AtlasDepthStatistic::Mean;
  plot_opts.depth_rolling.window = 2;
  plot_opts.depth_rolling.show_raw = true;
  plot_opts.triplot.kind = pluss::pl::flash::TriplotKind::Cloud;
  plot_opts.triplot.max_panels = 1;
  plot_opts.triplot.max_points = 2000;

  auto atlas_plot = atlas.plot(plot_opts);

  auto fig_frame = atlas_plot.frames(1).line("Close");
  fig_frame.headline("Atlas frame(1): Close");
  fig_frame.save_pdf(out_path("atlas_frame_line_close.pdf"));

  auto fig_frames_overlay = atlas_plot.frames({0, 1, 3}, "Close");
  fig_frames_overlay.headline("Atlas frames overlay: Close");
  fig_frames_overlay.save_pdf(out_path("atlas_frames_overlay_close.pdf"));

  pluss::table::AtlasFramesPlotOptions frame_facet_opts;
  frame_facet_opts.mode = pluss::table::AtlasFrameSelectionMode::Facet;
  frame_facet_opts.cols = 2;
  auto fig_frames_facet = atlas_plot.frames({0, 1, 2, 3}, "Close", std::nullopt, frame_facet_opts);
  fig_frames_facet.headline("Atlas frames facet: Close");
  fig_frames_facet.save_pdf(out_path("atlas_frames_facet_close.pdf"));

  auto fig_surface = atlas_plot.surface("Close");
  fig_surface.headline("Atlas depth surface: Close");
  fig_surface.save_pdf(out_path("atlas_depth_surface_close.pdf"));

  auto fig_wireframe = atlas_plot.wireframe("Volume");
  fig_wireframe.headline("Atlas depth wireframe: Volume");
  fig_wireframe.save_pdf(out_path("atlas_depth_wireframe_volume.pdf"));

  auto fig_cloud = atlas_plot.cloud3("Close", "Volume", "Range");
  fig_cloud.headline("Atlas depth cloud3: Close vs Volume vs Range");
  fig_cloud.save_pdf(out_path("atlas_depth_cloud3_close_volume_range.pdf"));

  auto fig_depth = atlas_plot.depth("Close");
  fig_depth.headline("Atlas depth aggregation: Close (mean with min/max)");
  fig_depth.save_pdf(out_path("atlas_depth_aggregate_close.pdf"));

  auto fig_depth_compare = atlas_plot.depth_compare({"Close", "Volume", "Range"});
  fig_depth_compare.headline("Atlas depth compare: Close / Volume / Range");
  fig_depth_compare.save_pdf(out_path("atlas_depth_compare_close_volume_range.pdf"));

  auto fig_depth_facet = atlas_plot.depth_facet({"Close", "Volume", "Range"});
  fig_depth_facet.headline("Atlas depth facet: median per feature");
  fig_depth_facet.save_pdf(out_path("atlas_depth_facet_close_volume_range.pdf"));

  auto fig_depth_rolling = atlas_plot.depth_rolling("Close");
  fig_depth_rolling.headline("Atlas depth rolling: Close");
  fig_depth_rolling.save_pdf(out_path("atlas_depth_rolling_close.pdf"));

  auto fig_triplot = atlas_plot.triplot({"Close", "Volume", "Range"});
  fig_triplot.headline("Atlas triplot: Close / Volume / Range");
  fig_triplot.save_pdf(out_path("atlas_triplot_close_volume_range.pdf"));

  return 0;
}
