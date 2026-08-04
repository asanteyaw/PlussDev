#include <pluss/pluss.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace {

pluss::Series peaks_like(std::int64_t rows, std::int64_t cols) {
  const auto x_axis = pluss::series::linspace(-3.0, 3.0, cols, pluss::ScalarType::Float64).reshape({1, cols});
  const auto y_axis = pluss::series::linspace(-3.0, 3.0, rows, pluss::ScalarType::Float64).reshape({rows, 1});
  const std::array<std::int64_t, 2> x_tile{rows, 1};
  const std::array<std::int64_t, 2> y_tile{1, cols};
  const auto x = x_axis.tile(std::span<const std::int64_t>(x_tile.data(), x_tile.size()));
  const auto y = y_axis.tile(std::span<const std::int64_t>(y_tile.data(), y_tile.size()));
  const auto crest = (1.0 - x).pow(2.0).mul(3.0).mul(x.pow(2.0).neg().sub(y.add(1.0).pow(2.0)).exp());
  const auto saddle = x.div(5.0).sub(x.pow(3.0)).sub(y.pow(5.0)).mul(-10.0).mul(x.pow(2.0).add(y.pow(2.0)).neg().exp());
  const auto tail = x.add(1.0).pow(2.0).neg().sub(y.pow(2.0)).exp().mul(-1.0 / 3.0);
  return crest.add(saddle).add(tail);
}

} // namespace

int main() {
  namespace pl = pluss::pl;
  const std::filesystem::path root = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
  std::filesystem::create_directories(root / "outputs" / "svg");
  std::filesystem::create_directories(root / "outputs" / "pdf");
  std::filesystem::create_directories(root / "outputs" / "pdfs");

  pl::Studio gallery("plusspl-flash-style", pl::Theme::paper());
  gallery.size(1680.0, 1240.0);
  gallery.mosaic(4, 4, 16.0, 16.0);
  gallery.headline("Flash-Style Plot Gallery (PlussPl Mappings)");
  gallery.legend(pl::KeyDock::East);

  const auto x = pluss::series::linspace(0.0, 12.0, 180, pluss::ScalarType::Float64);
  const auto y = x.mul(0.72).sin().add(x.mul(0.17).cos().mul(0.28)).add(x.mul(0.04));
  const auto yhat = x.mul(0.37).add(0.18);
  const auto residual = y.sub(yhat);

  auto& rel = gallery.pane2("relplot / scatterplot / lineplot");
  rel.place(0, 0).grid(pl::GridStyle::Major).caption("relational families");
  rel.trend(x, y).label("lineplot");
  rel.cloud(x, y, y.abs().mul(16.0).add(8.0), y).label("scatterplot");

  auto& dist = gallery.pane2("displot / histplot");
  dist.place(0, 1).grid(pl::GridStyle::Major).caption("distribution hist families");
  dist.bucket(y, 14).label("histplot");
  dist.bucket(residual, 12).label("displot");

  auto& kde = gallery.pane2("kdeplot");
  kde.place(0, 2).grid(pl::GridStyle::Major).caption("density estimate");
  const auto kde_x = pluss::series::linspace(-2.5, 2.5, 160, pluss::ScalarType::Float64);
  const auto kde_y = kde_x.pow(2.0).mul(-0.9).exp().mul(0.7).add(kde_x.sub(0.9).pow(2.0).mul(-2.1).exp().mul(0.42));
  kde.trend(kde_x, kde_y).label("kdeplot").width(2.0F);

  auto& ecdf = gallery.pane2("ecdfplot / rugplot");
  ecdf.place(0, 3).grid(pl::GridStyle::Major).caption("cumulative + rug");
  auto sorted = y.to_vec<double>();
  std::sort(sorted.begin(), sorted.end());
  std::vector<double> cdf(sorted.size(), 0.0);
  for (std::size_t i = 0; i < sorted.size(); ++i) {
    cdf[i] = static_cast<double>(i + 1) / static_cast<double>(sorted.size());
  }
  ecdf.trend(sorted, cdf).label("ecdfplot").width(1.8F);
  ecdf.spike(sorted, std::vector<double>(sorted.size(), 0.04), 0.0).label("rugplot");

  auto& cat_a = gallery.pane2("catplot / stripplot / swarmplot");
  cat_a.place(1, 0).grid(pl::GridStyle::Major).caption("categorical scatter");
  const std::vector<double> g{
      1,1,1,1,1,1,1,1, 2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3
  };
  const std::vector<double> v{
      0.2,0.3,0.4,0.5,0.55,0.62,0.7,0.76,
      0.3,0.34,0.45,0.52,0.6,0.72,0.82,0.92,
      0.18,0.25,0.36,0.48,0.53,0.58,0.66,0.8
  };
  std::vector<double> jitter(g.size(), 0.0);
  for (std::size_t i = 0; i < g.size(); ++i) {
    jitter[i] = g[i] + 0.11 * std::sin(static_cast<double>(i) * 1.9);
  }
  cat_a.cloud(jitter, v, std::vector<double>(g.size(), 13.0), v).label("strip/swarm");

  auto& cat_b = gallery.pane2("boxplot / violinplot / boxenplot");
  cat_b.place(1, 1).grid(pl::GridStyle::Major).caption("distribution categories");
  cat_b.quartet(g, v).label("boxplot");
  cat_b.violin(g, v).label("violinplot");
  cat_b.quartet(g, v).label("boxenplot-like").width(0.45F);

  auto& cat_c = gallery.pane2("pointplot / barplot / countplot");
  cat_c.place(1, 2).grid(pl::GridStyle::Major).caption("summary categories");
  cat_c.columns(std::array<double, 3>{1, 2, 3}, std::array<double, 3>{8, 12, 10}).label("countplot");
  cat_c.columns(std::array<double, 3>{1.25, 2.25, 3.25}, std::array<double, 3>{0.52, 0.66, 0.58}).label("barplot");
  cat_c.trend(std::array<double, 3>{1, 2, 3}, std::array<double, 3>{0.48, 0.61, 0.56}).label("pointplot");

  auto& reg = gallery.pane2("regplot / lmplot");
  reg.place(1, 3).grid(pl::GridStyle::Major).caption("regression overlays");
  reg.cloud(x, y).label("regplot points");
  reg.trend(x, yhat).label("lmplot fit").stroke(pl::Rgba::from_u8(171, 73, 57)).width(2.0F);

  auto& resid = gallery.pane2("residplot");
  resid.place(2, 0).grid(pl::GridStyle::Major).caption("residual diagnostics");
  resid.cloud(yhat, residual).label("residuals");
  resid.rule_y(0.0, "zero residual");

  const auto gx = pluss::series::linspace(-3.0, 3.0, 32, pluss::ScalarType::Float64);
  const auto gy = pluss::series::linspace(-3.0, 3.0, 32, pluss::ScalarType::Float64);
  const auto gz = peaks_like(32, 32);
  auto& heat = gallery.pane2("heatmap");
  heat.place(2, 1).grid(pl::GridStyle::Major).caption("matrix heatmap");
  heat.scale_guide();
  heat.fieldmap(gx, gy, gz).label("heatmap");

  auto& cluster = gallery.pane2("clustermap-like");
  cluster.place(2, 2).grid(pl::GridStyle::Major).caption("sorted heatmap proxy");
  cluster.fieldmap(gx, gy, gz.mul(-1.0)).label("clustermap-like");

  auto& joint = gallery.pane2("JointGrid / jointplot");
  joint.place(2, 3).grid(pl::GridStyle::Major).caption("joint relation proxy");
  joint.cloud(x, y).label("jointplot cloud");
  joint.contour(gx, gy, gz, std::vector<double>{-4.0, -2.0, 0.0, 2.0, 4.0}).label("JointGrid contour");

  auto& facet_a = gallery.pane2("FacetGrid A");
  facet_a.place(3, 0).grid(pl::GridStyle::Major).caption("facet subset A");
  facet_a.trend(x, y).label("facet A");

  auto& facet_b = gallery.pane2("FacetGrid B");
  facet_b.place(3, 1).grid(pl::GridStyle::Major).caption("facet subset B");
  facet_b.trend(x, y.mul(0.8).add(0.12)).label("facet B");

  auto& pair = gallery.pane2("PairGrid / pairplot");
  pair.place(3, 2).grid(pl::GridStyle::Major).caption("pairwise relation proxy");
  pair.cloud(y, yhat).label("pair 1");
  pair.cloud(y, residual).label("pair 2");

  auto& util = gallery.pane2("palette / style / utility");
  util.place(3, 3).grid(pl::GridStyle::Major).caption("palette and theme utilities");
  util.columns(std::array<double, 5>{1, 2, 3, 4, 5}, std::array<double, 5>{0.9, 1.0, 1.05, 1.1, 1.15})
      .label("set_theme / palette mapping")
      .palette(pl::PaletteRamp::ocean());

  gallery.save_svg((root / "outputs" / "svg" / "plusspl_style_gallery.svg").string());
  gallery.save_pdf((root / "outputs" / "pdf" / "plusspl_style_gallery.pdf").string());
  gallery.save_pdf((root / "outputs" / "pdfs" / "plusspl_style_gallery.pdf").string());
  return 0;
}
