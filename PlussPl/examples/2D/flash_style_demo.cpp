#include <pluss/pluss.h>

#include <array>
#include <cmath>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

int main() {
  using namespace pluss::pl;
  using namespace pluss::pl::flash;

  const auto output_dir = std::filesystem::path(PLUSSPL_EXAMPLES_SOURCE_DIR) / "outputs" / "2D";
  std::filesystem::create_directories(output_dir);
  const auto out_path = [&output_dir](std::string_view name) {
    return (output_dir / std::string(name)).string();
  };

  auto x = pluss::linspace(0.0, 10.0, 64, pluss::ScalarType::Float64);
  auto y = x.mul(0.65).sin().add(x.mul(1.8).cos().mul(0.25));
  auto y2 = x.mul(0.4).cos().mul(0.6).add(x.mul(1.1).sin().mul(0.35));

  auto cats = pluss::make_note({"A", "B", "A", "C", "B", "A", "C", "A", "B", "C"});
  auto cat_hue = pluss::make_note({"DeskA", "DeskA", "DeskB", "DeskB", "DeskA", "DeskB", "DeskA", "DeskB", "DeskA", "DeskB"});
  auto cat_vals = pluss::make_series(std::vector<double>{0.22, 0.47, 0.35, 0.68, 0.55, 0.31, 0.72, 0.28, 0.52, 0.76}, {10});
  auto cat_err = pluss::make_series(std::vector<double>{0.04, 0.05, 0.03, 0.06, 0.05, 0.04, 0.05, 0.03, 0.04, 0.06}, {10});
  auto hue_numeric = x.mul(0.9).sin().add(1.0).mul(2.0).round();

  Studio fig("flash_flash_catalog", Theme::paper());
  set_theme(fig, "whitegrid", "talk", "deep");
  set_context(fig, FlashContext::Talk, 1520.0, 980.0);
  fig.mosaic(3, 3, 16.0, 16.0);

  auto& relational = fig.pane2("Relational");
  relational.place(0, 0);
  set_style(relational, GridStyle::Major);
  scatterplot(relational, x, y).label("sns.scatterplot");
  lineplot(relational, x, y2).label("sns.lineplot");
  scatterplot(relational, x, y, hue_numeric);
  lineplot(relational, x, y2, hue_numeric);
  relplot(relational, x, y, "line").label("sns.relplot");
  move_legend(relational, KeyPlace::OutsideEast);
  despine(relational);

  auto& dist = fig.pane2("Distribution");
  dist.place(0, 1);
  set_style(dist, GridStyle::Major);
  displot(dist, y, 16).label("sns.displot");
  histplot(dist, y2, 14).label("sns.histplot");
  kdeplot(dist, y, 96).label("sns.kdeplot");
  ecdfplot(dist, y).label("sns.ecdfplot");
  rugplot(dist, y2, 0.06).label("sns.rugplot");

  auto& categorical = fig.pane2("Categorical");
  categorical.place(0, 2);
  set_style(categorical, GridStyle::Major);
  ViolinOptions flash_violin_opts;
  flash_violin_opts.bw_adjust = 0.9;
  flash_violin_opts.cut = 1.3;
  flash_violin_opts.scale = ViolinScale::Area;
  flash_violin_opts.inner = ViolinInner::Quartile;
  stripplot(categorical, cats, cat_vals).label("sns.stripplot");
  swarmplot(categorical, cats, cat_vals).label("sns.swarmplot");
  boxplot(categorical, cats, cat_vals).label("sns.boxplot");
  violinplot(categorical, cats, cat_vals, flash_violin_opts).label("sns.violinplot");
  boxenplot(categorical, cat_vals).label("sns.boxenplot");
  pointplot(categorical, pluss::linspace(0.0, 9.0, 10, pluss::ScalarType::Float64), cat_vals, cat_err).label("sns.pointplot");
  barplot(categorical, pluss::linspace(0.0, 9.0, 10, pluss::ScalarType::Float64), cat_vals).label("sns.barplot");
  countplot(categorical, cats).label("sns.countplot");
  catplot(categorical, cats, cat_vals, "violin").label("sns.catplot");
  move_legend(categorical, KeyPlace::OutsideEast);

  auto& regression = fig.pane2("Regression");
  regression.place(1, 0);
  set_style(regression, GridStyle::Major);
  RegPlotOptions reg_opts;
  reg_opts.order = 3;
  reg_opts.robust = true;
  reg_opts.show_ci = true;
  reg_opts.n_boot = 120;
  regplot(regression, x, y, reg_opts).label("sns.regplot");
  residplot(regression, x, y, reg_opts).label("sns.residplot");

  auto hm_z = pluss::make_series(std::vector<double>{
                                     0.2, 0.35, 0.48, 0.62,
                                     0.28, 0.42, 0.57, 0.71,
                                     0.36, 0.51, 0.66, 0.83,
                                 })
                  .reshape({3, 4});
  auto& matrix = fig.pane2("Matrix");
  matrix.place(1, 1);
  set_style(matrix, GridStyle::Major);
  HeatmapOptions hm_opts;
  hm_opts.annotate_levels = true;
  hm_opts.vmin = 0.25;
  hm_opts.vmax = 0.78;
  hm_opts.scale_guide = true;
  heatmap(matrix,
          std::array<double, 4>{0.0, 1.0, 2.0, 3.0},
          std::array<double, 3>{0.0, 1.0, 2.0},
          hm_z,
          hm_opts).label("sns.heatmap");
  ClusterMapOptions clust_opts;
  clust_opts.cluster_rows = true;
  clust_opts.cluster_cols = true;
  clust_opts.heatmap.scale_guide = false;
  clustermap(matrix,
             std::array<double, 4>{0.0, 1.0, 2.0, 3.0},
             std::array<double, 3>{0.0, 1.0, 2.0},
             hm_z,
             clust_opts).label("sns.clustermap");

  auto& palette = fig.pane2("Palette + Style");
  palette.place(1, 2);
  set_style(palette, FlashStyle::Whitegrid);
  palette.trend(std::array<double, 4>{0.0, 1.0, 2.0, 3.0},
                std::array<double, 4>{0.16, 0.22, 0.18, 0.24})
      .label("deep")
      .palette(color_palette("deep"));
  palette.trend(std::array<double, 4>{0.0, 1.0, 2.0, 3.0},
                std::array<double, 4>{0.30, 0.38, 0.33, 0.44})
      .label("muted")
      .palette(color_palette("muted"));
  palette.trend(std::array<double, 4>{0.0, 1.0, 2.0, 3.0},
                std::array<double, 4>{0.48, 0.55, 0.5, 0.62})
      .label("pastel")
      .palette(color_palette("pastel"));
  palette.trend(std::array<double, 4>{0.0, 1.0, 2.0, 3.0},
                std::array<double, 4>{0.64, 0.72, 0.7, 0.82})
      .label("bright")
      .palette(color_palette("bright"));
  palette.trend(std::array<double, 4>{0.0, 1.0, 2.0, 3.0},
                std::array<double, 4>{0.8, 0.88, 0.86, 0.94})
      .label("dark")
      .palette(color_palette("dark"));
  palette.trend(std::array<double, 4>{0.0, 1.0, 2.0, 3.0},
                std::array<double, 4>{0.96, 1.02, 0.98, 1.06})
      .label("colorblind")
      .palette(color_palette("colorblind"));
  move_legend(palette, KeyPlace::OutsideEast);

  auto names = get_dataset_names();
  auto tips = load_dataset("tips");
  auto& util = fig.pane2("Utility");
  util.place(2, 0);
  util.grid(GridStyle::Major);
  util.caption("sns.get_dataset_names + sns.load_dataset");
  util.trend(std::array<double, 3>{0.0, 1.0, 2.0},
             std::array<double, 3>{static_cast<double>(names.numel()), tips.has_value() ? 1.0 : 0.0, 3.0})
      .label("dataset utilities");

  auto& grid_meta = fig.pane2("Grid API");
  grid_meta.place(2, 1);
  grid_meta.grid(GridStyle::Major);
  grid_meta.caption("sns.FacetGrid / PairGrid / JointGrid in separate studios");
  grid_meta.trend(std::array<double, 3>{0.0, 1.0, 2.0}, std::array<double, 3>{2.0, 3.0, 2.0}).label("grid api");

  auto& lm = fig.pane2("lmplot marker");
  lm.place(2, 2);
  lm.grid(GridStyle::Major);
  lm.trend(std::array<double, 3>{0.0, 1.0, 2.0}, std::array<double, 3>{0.2, 0.5, 0.9}).label("lmplot in separate studio");

  // fig.save_svg(out_path("plusspl_flash_flash_demo.svg"));
  fig.save_pdf(out_path("plusspl_flash_style_demo.pdf"));

  Studio lm_fig("flash_lmplot_demo", Theme::paper());
  set_theme(lm_fig, FlashThemeOptions{
                        .style = FlashStyle::Darkgrid,
                        .context = FlashContext::Notebook,
                        .palette = FlashPalette::Muted,
                        .width = 980.0,
                        .height = 720.0});
  RegPlotOptions lm_opts;
  lm_opts.lowess = true;
  lm_opts.lowess_frac = 0.40;
  lm_opts.show_ci = true;
  lm_opts.n_boot = 120;
  lmplot(lm_fig, x, y2, lm_opts, "sns.lmplot");
  // lm_fig.save_svg(out_path("plusspl_flash_lmplot_demo.svg"));
  lm_fig.save_pdf(out_path("plusspl_flash_lmplot_demo.pdf"));

  Studio catplot_bar_fig("flash_catplot_bar_options_demo", Theme::paper());
  CatPlotOptions cat_bar_opts;
  cat_bar_opts.kind = CatPlotKind::Bar;
  cat_bar_opts.estimator = CatEstimator::Mean;
  cat_bar_opts.errorbar = CatErrorBar::CI;
  cat_bar_opts.confidence_level = 0.95;
  cat_bar_opts.dodge = true;
  cat_bar_opts.order = {"A", "B", "C"};
  cat_bar_opts.hue_order = {"DeskA", "DeskB"};
  auto& cat_bar_ax = catplot_bar_fig.pane2("catplot bar + ci + dodge");
  catplot(cat_bar_ax, cats, cat_vals, cat_hue, cat_bar_opts);
  // catplot_bar_fig.save_svg(out_path("plusspl_flash_catplot_bar_options_demo.svg"));
  catplot_bar_fig.save_pdf(out_path("plusspl_flash_catplot_bar_options_demo.pdf"));

  Studio catplot_violin_fig("flash_catplot_violin_options_demo", Theme::paper());
  CatPlotOptions cat_violin_opts;
  cat_violin_opts.kind = CatPlotKind::Violin;
  cat_violin_opts.split = true;
  cat_violin_opts.order = {"A", "B", "C"};
  cat_violin_opts.hue_order = {"DeskA", "DeskB"};
  cat_violin_opts.violin.bw_adjust = 0.85;
  cat_violin_opts.violin.cut = 1.2;
  cat_violin_opts.violin.inner = ViolinInner::Quartile;
  auto& cat_violin_ax = catplot_violin_fig.pane2("catplot violin + hue split");
  catplot(cat_violin_ax, cats, cat_vals, cat_hue, cat_violin_opts);
  // catplot_violin_fig.save_svg(out_path("plusspl_flash_catplot_violin_options_demo.svg"));
  catplot_violin_fig.save_pdf(out_path("plusspl_flash_catplot_violin_options_demo.pdf"));

  Studio heatmap_borderless_fig("flash_heatmap_borderless_demo", Theme::paper());
  auto& heatmap_borderless_ax = heatmap_borderless_fig.pane2("borderless heatmap");
  HeatmapOptions heatmap_borderless_opts;
  heatmap_borderless_opts.borderless = true;
  heatmap_borderless_opts.scale_guide = true;
  heatmap(heatmap_borderless_ax,
          std::array<double, 4>{0.0, 1.0, 2.0, 3.0},
          std::array<double, 3>{0.0, 1.0, 2.0},
          hm_z,
          heatmap_borderless_opts);
  heatmap_borderless_ax.x_ticks(std::array<double, 4>{0.0, 1.0, 2.0, 3.0}, {"T<56", "56<=T<129", "129<=T<179", "179<=T<=252"});
  heatmap_borderless_ax.y_ticks(std::array<double, 3>{0.0, 1.0, 2.0}, {"[0.8,0.9)", "[0.9,1.0)", "[1.0,1.1)"});
  heatmap_borderless_ax.x_label("Maturity").y_label("Moneyness");
  // heatmap_borderless_fig.save_svg(out_path("plusspl_flash_heatmap_borderless_demo.svg"));
  heatmap_borderless_fig.save_pdf(out_path("plusspl_flash_heatmap_borderless_demo.pdf"));

  Studio pairplot_fig("flash_pairplot_demo", Theme::paper());
  const auto pair_a = pluss::randn({360}, pluss::ScalarType::Float64, 24681357ULL);
  const auto pair_b = pair_a.mul(0.52).add(pluss::randn({360}, pluss::ScalarType::Float64, 31415926ULL).mul(0.82));
  const auto pair_c = pair_a.mul(-0.3)
                          .add(pair_b.mul(0.42))
                          .add(pluss::randn({360}, pluss::ScalarType::Float64, 27182818ULL).mul(0.76));
  std::vector<pluss::Series> pair_cols{
      pair_a,
      pair_b,
      pair_c,
  };
  PairPlotOptions pair_opts;
  pair_opts.kind = PairPlotKind::Reg;
  pair_opts.diag_kind = PairPlotDiagKind::Density;
  pair_opts.corner = true;
  pair_opts.dropna = true;
  pair_opts.labels = {"A", "B", "C"};
  pair_opts.max_points = 320;
  pairplot(pairplot_fig, pair_cols, pair_opts);
  // pairplot_fig.save_svg(out_path("plusspl_flash_pairplot_demo.svg"));
  pairplot_fig.save_pdf(out_path("plusspl_flash_pairplot_demo.pdf"));

  const auto pair_matrix = pair_a
                               .unite({pair_b, pair_c}, 0)
                               .reshape({3, 360})
                               .transpose(0, 1)
                               .contiguous();
  const auto named_pair_data = make_dataset(pair_matrix, {"temp", "humidity", "cnt"});

  Studio pairplot_named_dataset_fig("flash_pairplot_named_dataset_demo", Theme::paper());
  PairPlotOptions pair_named_opts;
  pair_named_opts.kind = PairPlotKind::Scatter;
  pair_named_opts.diag_kind = PairPlotDiagKind::Histogram;
  pair_named_opts.corner = true;
  pair_named_opts.max_points = 320;
  pairplot(pairplot_named_dataset_fig, named_pair_data, pair_named_opts);
  pairplot_named_dataset_fig.save_pdf(out_path("plusspl_flash_pairplot_named_dataset_demo.pdf"));

  Studio pairplot_matrix_fig("flash_pairplot_matrix_demo", Theme::paper());
  PairPlotOptions pair_matrix_opts;
  pair_matrix_opts.kind = PairPlotKind::Reg;
  pair_matrix_opts.diag_kind = PairPlotDiagKind::Density;
  pair_matrix_opts.corner = true;
  pair_matrix_opts.max_points = 320;
  pairplot(
      pairplot_matrix_fig,
      pair_matrix,
      {"temp", "humidity", "cnt"},
      Axis::Cols,
      pair_matrix_opts);
  pairplot_matrix_fig.save_pdf(out_path("plusspl_flash_pairplot_matrix_demo.pdf"));

  Studio jointplot_fig("flash_jointplot_demo", Theme::paper());
  jointplot(jointplot_fig, x, y, "scatter");
  // jointplot_fig.save_svg(out_path("plusspl_flash_jointplot_demo.svg"));
  jointplot_fig.save_pdf(out_path("plusspl_flash_jointplot_demo.pdf"));

  Studio jointplot_named_dataset_fig("flash_jointplot_named_dataset_demo", Theme::paper());
  jointplot(jointplot_named_dataset_fig, named_pair_data, "temp", "cnt", "reg");
  jointplot_named_dataset_fig.save_pdf(out_path("plusspl_flash_jointplot_named_dataset_demo.pdf"));

  Studio jointplot_matrix_fig("flash_jointplot_matrix_demo", Theme::paper());
  MatrixJointOptions matrix_joint_opts;
  matrix_joint_opts.feature_axis = Axis::Cols;
  matrix_joint_opts.x_feature = 0;
  matrix_joint_opts.y_feature = 2;
  jointplot(jointplot_matrix_fig, pair_matrix, matrix_joint_opts, "hex");
  jointplot_matrix_fig.save_pdf(out_path("plusspl_flash_jointplot_matrix_demo.pdf"));

  Studio parallelplot_dataset_fig("flash_parallelplot_dataset_demo", Theme::paper());
  ParallelCoordinatesOptions parallel_opts;
  parallel_opts.max_points = 320;
  parallelplot(parallelplot_dataset_fig, named_pair_data, parallel_opts);
  parallelplot_dataset_fig.save_pdf(out_path("plusspl_flash_parallelplot_dataset_demo.pdf"));

  Studio andrews_dataset_fig("flash_andrews_dataset_demo", Theme::paper());
  AndrewsCurvesOptions andrews_opts;
  andrews_opts.max_curves = 320;
  andrews_curves(andrews_dataset_fig, named_pair_data, andrews_opts);
  andrews_dataset_fig.save_pdf(out_path("plusspl_flash_andrews_dataset_demo.pdf"));

  Studio radviz_dataset_fig("flash_radviz_dataset_demo", Theme::paper());
  RadvizOptions radviz_opts;
  radviz_opts.max_points = 320;
  radviz(radviz_dataset_fig, named_pair_data, radviz_opts);
  radviz_dataset_fig.save_pdf(out_path("plusspl_flash_radviz_dataset_demo.pdf"));

  Studio axis_style_fig("flash_axis_style_demo", Theme::paper());
  set_theme(axis_style_fig, FlashThemeOptions{
                              .style = FlashStyle::Whitegrid,
                              .context = FlashContext::Notebook,
                              .palette = FlashPalette::Deep,
                              .width = 1120.0,
                              .height = 760.0});
  axis_style_fig.mosaic(1, 2, 14.0, 10.0);
  auto& horizontal_grid_ax = axis_style_fig.pane2("horizontal grid only");
  horizontal_grid_ax.place(0, 0);
  horizontal_grid(horizontal_grid_ax, true);
  horizontal_grid_ax.x_label("temperature").y_label("height");
  lineplot(horizontal_grid_ax, x, y).label("horizontal grid");
  auto& tickless_ax = axis_style_fig.pane2("tickless");
  tickless_ax.place(0, 1);
  set_style(tickless_ax, FlashStyle::Whitegrid);
  tickless(tickless_ax, true);
  tickless_ax.x_label("temperature").y_label("height");
  lineplot(tickless_ax, x, y2).label("tickless chart");
  // axis_style_fig.save_svg(out_path("plusspl_flash_axis_style_demo.svg"));
  axis_style_fig.save_pdf(out_path("plusspl_flash_axis_style_demo.pdf"));

  const std::array<FlashContext, 4> contexts{
      FlashContext::Paper,
      FlashContext::Notebook,
      FlashContext::Talk,
      FlashContext::Poster,
  };
  const std::array<std::string_view, 4> context_names{
      "paper",
      "notebook",
      "talk",
      "poster",
  };
  for (std::size_t i = 0; i < contexts.size(); ++i) {
    Studio context_fig("flash_context_demo", Theme::paper());
    set_theme(context_fig, FlashThemeOptions{
                              .style = FlashStyle::Whitegrid,
                              .context = contexts[i],
                              .palette = FlashPalette::Deep});
    auto& pane = context_fig.pane2(std::string(context_names[i]) + " context");
    lineplot(pane, x, y).label("line");
    pane.x_label("x").y_label("y");
    // context_fig.save_svg(out_path("plusspl_flash_context_" + std::string(context_names[i]) + "_demo.svg"));
    context_fig.save_pdf(out_path("plusspl_flash_context_" + std::string(context_names[i]) + "_demo.pdf"));
  }

  Studio style_gallery_fig("flash_style_gallery_demo", Theme::paper());
  set_theme(style_gallery_fig, FlashThemeOptions{
                                 .style = FlashStyle::Whitegrid,
                                 .context = FlashContext::Notebook,
                                 .palette = FlashPalette::Deep,
                                 .width = 1740.0,
                                 .height = 460.0});
  style_gallery_fig.mosaic(1, 5, 8.0, 8.0);
  const std::array<FlashStyle, 5> styles{
      FlashStyle::Darkgrid,
      FlashStyle::Whitegrid,
      FlashStyle::Dark,
      FlashStyle::White,
      FlashStyle::Ticks,
  };
  const std::array<std::string_view, 5> style_names{
      "darkgrid",
      "whitegrid",
      "dark",
      "white",
      "ticks",
  };
  for (std::size_t i = 0; i < styles.size(); ++i) {
    auto& pane = style_gallery_fig.pane2(std::string(style_names[i]));
    pane.place(0, static_cast<std::int64_t>(i));
    set_style(pane, styles[i]);
    lineplot(pane, x, y).label(std::string(style_names[i]));
    pane.x_label("x").y_label("y");
  }
  // style_gallery_fig.save_svg(out_path("plusspl_flash_style_gallery_demo.svg"));
  style_gallery_fig.save_pdf(out_path("plusspl_flash_style_gallery_demo.pdf"));

  Studio palette_gallery_fig("flash_palette_gallery_demo", Theme::paper());
  set_theme(palette_gallery_fig, FlashThemeOptions{
                                   .style = FlashStyle::Whitegrid,
                                   .context = FlashContext::Notebook,
                                   .palette = FlashPalette::Deep,
                                   .width = 1580.0,
                                   .height = 900.0});
  palette_gallery_fig.mosaic(2, 3, 8.0, 8.0);
  const std::array<std::string_view, 6> palette_names{
      "deep",
      "muted",
      "pastel",
      "bright",
      "dark",
      "colorblind",
  };
  for (std::size_t i = 0; i < palette_names.size(); ++i) {
    auto& pane = palette_gallery_fig.pane2(std::string(palette_names[i]));
    pane.place(static_cast<std::int64_t>(i / 3), static_cast<std::int64_t>(i % 3));
    set_style(pane, FlashStyle::Whitegrid);
    lineplot(pane, x, y).label("line").palette(color_palette(palette_names[i]));
    pane.x_label("x").y_label("y");
  }
  // palette_gallery_fig.save_svg(out_path("plusspl_flash_palette_gallery_demo.svg"));
  palette_gallery_fig.save_pdf(out_path("plusspl_flash_palette_gallery_demo.pdf"));

  Studio jointplot_hex_fig("flash_jointplot_hex_demo", Theme::paper());
  JointPlotOptions joint_hex_opts;
  joint_hex_opts.kind = JointPlotKind::Hex;
  joint_hex_opts.marginal = JointMarginalKind::Hist;
  joint_hex_opts.hex_bins_x = 24;
  joint_hex_opts.hex_bins_y = 20;
  jointplot(jointplot_hex_fig, pair_a, pair_b, joint_hex_opts);
  // jointplot_hex_fig.save_svg(out_path("plusspl_flash_jointplot_hex_demo.svg"));
  jointplot_hex_fig.save_pdf(out_path("plusspl_flash_jointplot_hex_demo.pdf"));

  Studio jointplot_kde_fig("flash_jointplot_kde_demo", Theme::paper());
  JointPlotOptions joint_kde_opts;
  joint_kde_opts.kind = JointPlotKind::Kde;
  joint_kde_opts.marginal = JointMarginalKind::Kde;
  joint_kde_opts.kde_bins_x = 30;
  joint_kde_opts.kde_bins_y = 24;
  joint_kde_opts.kde_smooth_passes = 2;
  joint_kde_opts.kde_levels = 10;
  jointplot(jointplot_kde_fig, pair_a, pair_c, joint_kde_opts);
  // jointplot_kde_fig.save_svg(out_path("plusspl_flash_jointplot_kde_demo.svg"));
  jointplot_kde_fig.save_pdf(out_path("plusspl_flash_jointplot_kde_demo.pdf"));

  Studio acf_fig("flash_acfplot_demo", Theme::paper());
  AcfPlotOptions acf_opts;
  acf_opts.max_lag = 36;
  acf_opts.include_zero_lag = false;
  acfplot(acf_fig, y, acf_opts);
  // acf_fig.save_svg(out_path("plusspl_flash_acfplot_demo.svg"));
  acf_fig.save_pdf(out_path("plusspl_flash_acfplot_demo.pdf"));

  Studio pacf_fig("flash_pacfplot_demo", Theme::paper());
  PacfPlotOptions pacf_opts;
  pacf_opts.max_lag = 36;
  pacf_opts.include_zero_lag = false;
  pacfplot(pacf_fig, y, pacf_opts);
  // pacf_fig.save_svg(out_path("plusspl_flash_pacfplot_demo.svg"));
  pacf_fig.save_pdf(out_path("plusspl_flash_pacfplot_demo.pdf"));

  Studio ccf_fig("flash_ccfplot_demo", Theme::paper());
  CcfPlotOptions ccf_opts;
  ccf_opts.max_lag = 36;
  ccfplot(ccf_fig, pair_a, pair_b, ccf_opts);
  // ccf_fig.save_svg(out_path("plusspl_flash_ccfplot_demo.svg"));
  ccf_fig.save_pdf(out_path("plusspl_flash_ccfplot_demo.pdf"));

  Studio qq_fig("flash_qqplot_demo", Theme::paper());
  qqplot(qq_fig, pair_c, QqPlotOptions{}, "qq for pair_c");
  // qq_fig.save_svg(out_path("plusspl_flash_qqplot_demo.svg"));
  qq_fig.save_pdf(out_path("plusspl_flash_qqplot_demo.pdf"));

  Studio resid_hist_fig("flash_resid_histplot_demo", Theme::paper());
  ResidHistPlotOptions rh_opts;
  rh_opts.bins = 20;
  rh_opts.reg = reg_opts;
  resid_histplot(resid_hist_fig, x, y, rh_opts, "residual histogram");
  // resid_hist_fig.save_svg(out_path("plusspl_flash_resid_histplot_demo.svg"));
  resid_hist_fig.save_pdf(out_path("plusspl_flash_resid_histplot_demo.pdf"));

  Studio resid_acf_fig("flash_resid_acfplot_demo", Theme::paper());
  ResidAcfPlotOptions ra_opts;
  ra_opts.reg = reg_opts;
  ra_opts.acf.max_lag = 28;
  resid_acfplot(resid_acf_fig, x, y, ra_opts);
  // resid_acf_fig.save_svg(out_path("plusspl_flash_resid_acfplot_demo.svg"));
  resid_acf_fig.save_pdf(out_path("plusspl_flash_resid_acfplot_demo.pdf"));

  std::vector<pluss::Series> rel_xs{
      x,
      x,
      x,
      x,
  };
  std::vector<pluss::Series> rel_ys{
      y,
      y2,
      x.mul(0.35).sin().add(y.mul(0.25)),
      x.mul(0.21).cos().add(y2.mul(0.30)),
  };
  Studio rel_grid_fig("flash_relgridplot_demo", Theme::paper());
  RelGridPlotOptions rel_grid_opts;
  rel_grid_opts.kind = RelGridKind::Reg;
  rel_grid_opts.rows = 2;
  rel_grid_opts.cols = 2;
  rel_grid_opts.titles = {"reg facet 1", "reg facet 2", "reg facet 3", "reg facet 4"};
  rel_grid_opts.x_labels = {"x", "x", "x", "x"};
  rel_grid_opts.y_labels = {"y1", "y2", "y3", "y4"};
  relgridplot(rel_grid_fig, rel_xs, rel_ys, rel_grid_opts);
  // rel_grid_fig.save_svg(out_path("plusspl_flash_relgridplot_demo.svg"));
  rel_grid_fig.save_pdf(out_path("plusspl_flash_relgridplot_demo.pdf"));

  std::vector<pluss::Note> cat_groups{
      cats,
      cats,
      cats,
      cats,
  };
  std::vector<pluss::Series> cat_values{
      cat_vals,
      cat_vals.add(0.09),
      cat_vals.mul(0.85).add(0.06),
      cat_vals.mul(1.10).sub(0.03),
  };
  std::vector<pluss::Note> cat_hues{
      cat_hue,
      cat_hue,
      cat_hue,
      cat_hue,
  };
  Studio cat_grid_fig("flash_catgridplot_demo", Theme::paper());
  CatGridPlotOptions cat_grid_opts;
  cat_grid_opts.rows = 2;
  cat_grid_opts.cols = 2;
  cat_grid_opts.titles = {"desk split q1", "desk split q2", "desk split q3", "desk split q4"};
  cat_grid_opts.cat.kind = CatPlotKind::Bar;
  cat_grid_opts.cat.estimator = CatEstimator::Mean;
  cat_grid_opts.cat.errorbar = CatErrorBar::SE;
  cat_grid_opts.cat.dodge = true;
  catgridplot(cat_grid_fig, cat_groups, cat_values, cat_hues, cat_grid_opts);
  // cat_grid_fig.save_svg(out_path("plusspl_flash_catgridplot_demo.svg"));
  cat_grid_fig.save_pdf(out_path("plusspl_flash_catgridplot_demo.pdf"));

  std::vector<pluss::Series> dis_values{
      pair_a,
      pair_b,
      pair_c,
      pair_a.add(pair_c.mul(0.25)),
  };
  Studio dis_grid_fig("flash_disgridplot_demo", Theme::paper());
  DisGridPlotOptions dis_grid_opts;
  dis_grid_opts.kind = DisGridKind::Density;
  dis_grid_opts.rows = 2;
  dis_grid_opts.cols = 2;
  dis_grid_opts.titles = {"density A", "density B", "density C", "density A+0.25C"};
  disgridplot(dis_grid_fig, dis_values, dis_grid_opts);
  // dis_grid_fig.save_svg(out_path("plusspl_flash_disgridplot_demo.svg"));
  dis_grid_fig.save_pdf(out_path("plusspl_flash_disgridplot_demo.pdf"));

  Studio facet_map_fig("flash_facet_map_plot_demo", Theme::paper());
  facet_map_plot(
      facet_map_fig,
      2,
      2,
      [&](Pane& pane, std::int64_t row, std::int64_t col) {
        const double slope = 0.20 + 0.10 * static_cast<double>(row);
        const double freq = 0.80 + 0.20 * static_cast<double>(col);
        auto y_local = x.mul(slope).sin().add(x.mul(freq).cos().mul(0.22));
        pane.grid(GridStyle::Major).x_label("x").y_label("y");
        pane.trend(x, y_local).label("r" + std::to_string(row) + " c" + std::to_string(col));
      });
  // facet_map_fig.save_svg(out_path("plusspl_flash_facet_map_plot_demo.svg"));
  facet_map_fig.save_pdf(out_path("plusspl_flash_facet_map_plot_demo.pdf"));

  Studio pair_map_fig("flash_pairgrid_map_plot_demo", Theme::paper());
  std::vector<pluss::Series> map_cols{
      pair_a,
      pair_b,
      pair_c,
  };
  pairgrid_map_plot(
      pair_map_fig,
      3,
      [&](Pane& pane, std::int64_t row, std::int64_t col, bool diagonal) {
        pane.grid(GridStyle::Major);
        if (diagonal) {
          histplot(pane, map_cols[static_cast<std::size_t>(row)], 16);
          return;
        }
        scatterplot(
            pane,
            map_cols[static_cast<std::size_t>(col)],
            map_cols[static_cast<std::size_t>(row)]);
      });
  // pair_map_fig.save_svg(out_path("plusspl_flash_pairgrid_map_plot_demo.svg"));
  pair_map_fig.save_pdf(out_path("plusspl_flash_pairgrid_map_plot_demo.pdf"));

  Studio joint_map_fig("flash_jointgrid_map_plot_demo", Theme::paper());
  jointgrid_map_plot(
      joint_map_fig,
      [&](Pane& pane) {
        pane.grid(GridStyle::Major).x_label("A").y_label("B");
        regplot(pane, pair_a, pair_b);
      },
      [&](Pane& pane, bool top) {
        pane.grid(GridStyle::Major);
        if (top) {
          histplot(pane, pair_a, 20);
          pane.y_label("");
          return;
        }
        histplot(pane, pair_b, 20);
        pane.x_label("");
      });
  // joint_map_fig.save_svg(out_path("plusspl_flash_jointgrid_map_plot_demo.svg"));
  joint_map_fig.save_pdf(out_path("plusspl_flash_jointgrid_map_plot_demo.pdf"));

//   auto figz = pluss::pl::studio("demo");
//   auto& ax = figz.pane2();
//   ax.trend(std::array<double,3>{0,1,2}, std::array<double,3>{1,2,1});
//   figz.show("Demo Plot", 1280, 820, true); // opens popup/viewer

  return 0;
}
