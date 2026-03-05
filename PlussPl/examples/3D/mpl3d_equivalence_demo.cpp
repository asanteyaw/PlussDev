#include <pluss/pluss.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace {

pluss::Series peaks_like(std::int64_t rows, std::int64_t cols) {
  const auto x_axis = pluss::linspace(-3.0, 3.0, cols, pluss::ScalarType::Float64).reshape({1, cols});
  const auto y_axis = pluss::linspace(-3.0, 3.0, rows, pluss::ScalarType::Float64).reshape({rows, 1});
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

  pl::Studio studio("plusspl-mpl3d-equivalence", pl::Theme::ocean());
  studio.size(1440.0, 980.0);
  studio.mosaic(2, 3, 18.0, 16.0);
  studio.headline("Matplotlib 3D Equivalence Gallery (PlussPl)");
  studio.trace_key(pl::KeyDock::East);

  const auto t = pluss::linspace(0.0, 11.0, 260, pluss::ScalarType::Float64);
  const auto x = t.mul(1.6).cos();
  const auto y = t.mul(1.6).sin();
  const auto z = t.mul(0.2);
  const auto gx = pluss::linspace(-3.0, 3.0, 36, pluss::ScalarType::Float64);
  const auto gy = pluss::linspace(-3.0, 3.0, 36, pluss::ScalarType::Float64);
  const auto gz = peaks_like(36, 36);

  auto& p1 = studio.pane3("ax.plot3D");
  p1.place(0, 0).grid(pl::GridStyle::Major).caption("plot3D -> path3");
  p1.path3(x, y, z).label("ax.plot3D").width(2.0F);

  auto& p2 = studio.pane3("ax.scatter3D");
  p2.place(0, 1).grid(pl::GridStyle::Major).caption("scatter3D -> cloud3");
  const auto host_x = x.to_vec<double>();
  const auto host_y = y.to_vec<double>();
  const auto host_z = z.to_vec<double>();
  std::vector<double> sx;
  std::vector<double> sy;
  std::vector<double> sz;
  std::vector<double> ss;
  for (std::size_t i = 0; i < host_x.size(); i += 7) {
    sx.push_back(host_x[i]);
    sy.push_back(host_y[i]);
    sz.push_back(host_z[i]);
    ss.push_back((host_z[i] + 1.0) * 26.0);
  }
  p2.cloud3(sx, sy, sz, ss, sz).label("ax.scatter3D");

  auto& p3 = studio.pane3("ax.plot_surface + wireframe");
  p3.place(0, 2).grid(pl::GridStyle::Major).caption("surface + wireframe");
  p3.sheet(gx, gy, gz)
      .label("ax.plot_surface")
      .shade(pl::ShadeMode::Smooth)
      .lighting(pl::LightingMode::Gouraud)
      .palette(pl::PaletteRamp::lagoon());
  p3.lattice(gx, gy, gz).label("ax.plot_wireframe").width(0.8F);

  auto& p4 = studio.pane3("ax.contour3D");
  p4.place(1, 0).grid(pl::GridStyle::Major).caption("contour3D -> ridge3");
  p4.ridge3(gx, gy, gz, std::vector<double>{-5.0, -3.0, -1.0, 0.5, 2.0, 4.5, 7.0})
      .label("ax.contour3D")
      .annotate_levels();

  auto& p5 = studio.pane3("ax.bar3d");
  p5.place(1, 1).grid(pl::GridStyle::Major).caption("bar3d -> spire3");
  const auto bars = pluss::make_series(
      std::vector<double>{
          0.4, 0.8, 1.1, 1.4,
          0.7, 1.2, 1.8, 2.0,
          0.6, 0.9, 1.5, 1.9,
          0.3, 0.7, 1.2, 1.6,
      },
      {4, 4});
  p5.spire3(std::array<double, 4>{1, 2, 3, 4}, std::array<double, 4>{1, 2, 3, 4}, bars)
      .label("ax.bar3d")
      .palette(pl::PaletteRamp::thermal());

  auto& p6 = studio.pane3("volumetric extension");
  p6.place(1, 2).grid(pl::GridStyle::Major).caption("isoshell extension beyond mpl baseline");
  const auto vx = pluss::linspace(-1.3, 1.3, 22, pluss::ScalarType::Float64);
  const auto vy = pluss::linspace(-1.3, 1.3, 22, pluss::ScalarType::Float64);
  const auto vz = pluss::linspace(-1.2, 1.2, 22, pluss::ScalarType::Float64);
  const auto hx = vx.to_vec<double>();
  const auto hy = vy.to_vec<double>();
  const auto hz = vz.to_vec<double>();
  std::vector<double> field;
  field.reserve(hx.size() * hy.size() * hz.size());
  for (std::size_t i = 0; i < hx.size(); ++i) {
    for (std::size_t j = 0; j < hy.size(); ++j) {
      for (std::size_t k = 0; k < hz.size(); ++k) {
        const double px = hx[i];
        const double py = hy[j];
        const double pz = hz[k];
        field.push_back(std::exp(-(px * px + py * py + pz * pz) * 2.8) + 0.12 * std::cos(3.0 * px + 2.0 * py));
      }
    }
  }
  p6.isoshell(vx, vy, vz, pluss::make_series(field, {22, 22, 22}), std::vector<double>{0.26, 0.42, 0.58})
      .label("isoshell")
      .transparent()
      .shade(pl::ShadeMode::Smooth);

  studio.save_svg((root / "outputs" / "svg" / "plusspl_mpl3d_equivalence.svg").string());
  studio.save_pdf((root / "outputs" / "pdf" / "plusspl_mpl3d_equivalence.pdf").string());
  studio.save_pdf((root / "outputs" / "pdfs" / "plusspl_mpl3d_equivalence.pdf").string());
  return 0;
}
