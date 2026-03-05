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
  auto cat_vals = pluss::make_series(std::vector<double>{0.22, 0.47, 0.35, 0.68, 0.55, 0.31, 0.72, 0.28, 0.52, 0.76}, {10});
  auto cat_err = pluss::make_series(std::vector<double>{0.04, 0.05, 0.03, 0.06, 0.05, 0.04, 0.05, 0.03, 0.04, 0.06}, {10});
  auto hue_numeric = x.mul(0.9).sin().add(1.0).mul(2.0).round();

  Studio fig("flash_seaborn_catalog", Theme::paper());
  set_theme(fig, Theme::paper());
  set_context(fig, 1520.0, 980.0, 1.08, 1.05);
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
  stripplot(categorical, cats, cat_vals).label("sns.stripplot");
  swarmplot(categorical, cats, cat_vals).label("sns.swarmplot");
  boxplot(categorical, cats, cat_vals).label("sns.boxplot");
  violinplot(categorical, cats, cat_vals).label("sns.violinplot");
  boxenplot(categorical, cat_vals).label("sns.boxenplot");
  pointplot(categorical, pluss::linspace(0.0, 9.0, 10, pluss::ScalarType::Float64), cat_vals, cat_err).label("sns.pointplot");
  barplot(categorical, pluss::linspace(0.0, 9.0, 10, pluss::ScalarType::Float64), cat_vals).label("sns.barplot");
  countplot(categorical, cats).label("sns.countplot");
  catplot(categorical, cats, cat_vals, "violin").label("sns.catplot");
  move_legend(categorical, KeyPlace::OutsideEast);

  auto& regression = fig.pane2("Regression");
  regression.place(1, 0);
  set_style(regression, GridStyle::Major);
  regplot(regression, x, y).label("sns.regplot");
  residplot(regression, x, y).label("sns.residplot");

  auto hm_z = pluss::make_series(
      std::vector<double>{
          0.2, 0.35, 0.48, 0.62,
          0.28, 0.42, 0.57, 0.71,
          0.36, 0.51, 0.66, 0.83,
      },
      {3, 4});
  auto& matrix = fig.pane2("Matrix");
  matrix.place(1, 1);
  set_style(matrix, GridStyle::Major);
  heatmap(matrix,
          std::array<double, 4>{0.0, 1.0, 2.0, 3.0},
          std::array<double, 3>{0.0, 1.0, 2.0},
          hm_z).label("sns.heatmap");
  clustermap(matrix,
             std::array<double, 4>{0.0, 1.0, 2.0, 3.0},
             std::array<double, 3>{0.0, 1.0, 2.0},
             hm_z).label("sns.clustermap");

  auto& palette = fig.pane2("Palette + Style");
  palette.place(1, 2);
  palette.grid(GridStyle::Major);
  palette.trend(std::array<double, 4>{0.0, 1.0, 2.0, 3.0},
                std::array<double, 4>{0.2, 0.5, 0.35, 0.72})
      .label("sns.color_palette")
      .palette(color_palette("lagoon"));
  palette.trend(std::array<double, 4>{0.0, 1.0, 2.0, 3.0},
                std::array<double, 4>{0.1, 0.26, 0.44, 0.64})
      .label("sns.hls/husl/cubehelix")
      .palette(hls_palette(6));
  palette.trend(std::array<double, 4>{0.0, 1.0, 2.0, 3.0},
                std::array<double, 4>{0.3, 0.18, 0.58, 0.42})
      .label("sns.light/dark/diverging")
      .palette(diverging_palette(Rgba::from_u8(55, 110, 190), Rgba::from_u8(198, 74, 65), 7));
  palette.trend(std::array<double, 4>{0.0, 1.0, 2.0, 3.0},
                std::array<double, 4>{0.15, 0.38, 0.22, 0.54})
      .label("sns.blend/mpl/xkcd/crayon")
      .palette(blend_palette({Rgba::from_u8(36, 126, 190), Rgba::from_u8(238, 156, 31)}, "blend"));
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

  fig.save_svg(out_path("plusspl_flash_seaborn_demo.svg"));
  fig.save_pdf(out_path("plusspl_flash_seaborn_demo.pdf"));

  Studio lm_fig("flash_lmplot_demo", Theme::paper());
  set_context(lm_fig, 980.0, 720.0, 1.05, 1.05);
  lmplot(lm_fig, x, y2, "sns.lmplot");
  lm_fig.save_svg(out_path("plusspl_flash_lmplot_demo.svg"));
  lm_fig.save_pdf(out_path("plusspl_flash_lmplot_demo.pdf"));

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
  pairplot(pairplot_fig, pair_cols);
  pairplot_fig.save_svg(out_path("plusspl_flash_pairplot_demo.svg"));
  pairplot_fig.save_pdf(out_path("plusspl_flash_pairplot_demo.pdf"));

  Studio jointplot_fig("flash_jointplot_demo", Theme::paper());
  jointplot(jointplot_fig, x, y, "scatter");
  jointplot_fig.save_svg(out_path("plusspl_flash_jointplot_demo.svg"));
  jointplot_fig.save_pdf(out_path("plusspl_flash_jointplot_demo.pdf"));

  Studio facet_fig("flash_facetgrid_demo", Theme::paper());
  FacetGrid(facet_fig, 2, 2);
  auto& f0 = facet_fig.pane2("facet");
  f0.place(0, 0).trend(std::array<double, 3>{0.0, 1.0, 2.0}, std::array<double, 3>{0.2, 0.4, 0.6});
  facet_fig.save_svg(out_path("plusspl_flash_facetgrid_demo.svg"));

  Studio pair_grid_fig("flash_pairgrid_demo", Theme::paper());
  PairGrid(pair_grid_fig, 2);
  auto& pg0 = pair_grid_fig.pane2("pair");
  pg0.place(0, 0).trend(std::array<double, 3>{0.0, 1.0, 2.0}, std::array<double, 3>{0.3, 0.5, 0.7});
  pair_grid_fig.save_svg(out_path("plusspl_flash_pairgrid_demo.svg"));

  Studio joint_grid_fig("flash_jointgrid_demo", Theme::paper());
  JointGrid(joint_grid_fig);
  auto& jg0 = joint_grid_fig.pane2("joint-grid");
  jg0.place(0, 0).trend(std::array<double, 3>{0.0, 1.0, 2.0}, std::array<double, 3>{0.25, 0.45, 0.65});
  joint_grid_fig.save_svg(out_path("plusspl_flash_jointgrid_demo.svg"));

  auto figz = pluss::pl::studio("demo");
  auto& ax = figz.pane2();
  ax.trend(std::array<double,3>{0,1,2}, std::array<double,3>{1,2,1});
  figz.show("Demo Plot", 1280, 820, true); // opens popup/viewer

  return 0;
}
