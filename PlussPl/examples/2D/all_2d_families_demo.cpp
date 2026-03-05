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

  pl::Studio studio("plusspl-all-2d-families", pl::Theme::paper());
  studio.size(1540.0, 1140.0);
  studio.mosaic(3, 3, 20.0, 18.0);
  studio.headline("PlussPl All 2D Families");
  studio.legend(pl::KeyDock::East);

  const auto axis = pluss::linspace(1.0, 20.0, 20, pluss::ScalarType::Float64);
  const auto wave = axis.mul(0.45).sin().add(axis.mul(0.15).cos().mul(0.4));

  auto& pane1 = studio.pane2("trend + cloud");
  pane1.place(0, 0).grid(pl::GridStyle::Major).x_label("x").y_label("y").caption("line and scatter families");
  pane1.trend(axis, wave).label("trend").width(2.0F);
  pane1.cloud(axis, wave.add(0.08), wave.abs().mul(30.0).add(12.0), wave).label("cloud");

  auto& pane2 = studio.pane2("columns + spike");
  pane2.place(0, 1).grid(pl::GridStyle::Major).caption("bar and stem-like families");
  pane2.columns(std::array<double, 6>{1, 2, 3, 4, 5, 6}, std::array<double, 6>{2.0, 2.8, 2.1, 3.4, 3.0, 3.8})
      .label("columns")
      .palette(pl::PaletteRamp::meadow());
  pane2.spike(std::array<double, 6>{1, 2, 3, 4, 5, 6}, std::array<double, 6>{1.2, 2.1, 1.9, 2.8, 2.5, 3.3})
      .label("spike")
      .stroke(pl::Rgba::from_u8(35, 76, 121));

  auto& pane3 = studio.pane2("bucket + quartet + violin");
  pane3.place(0, 2).grid(pl::GridStyle::Major).caption("histogram and distribution families");
  pane3.bucket(std::array<double, 16>{-1.8, -1.2, -0.9, -0.5, -0.3, -0.2, 0.0, 0.1, 0.2, 0.4, 0.7, 0.9, 1.1, 1.4, 1.7, 2.0}, 10)
      .label("bucket")
      .palette(pl::PaletteRamp::lagoon());
  pane3.quartet(std::array<double, 8>{1, 1, 1, 1, 2, 2, 2, 2},
                std::array<double, 8>{0.25, 0.3, 0.42, 0.7, 0.6, 0.74, 0.95, 1.22})
      .label("quartet")
      .stroke(pl::Rgba::from_u8(40, 50, 66));
  pane3.violin(std::array<double, 8>{1, 1, 1, 1, 2, 2, 2, 2},
               std::array<double, 8>{0.25, 0.3, 0.42, 0.7, 0.6, 0.74, 0.95, 1.22})
      .label("violin")
      .palette(pl::PaletteRamp::thermal());

  auto& pane4 = studio.pane2("wedge");
  pane4.place(1, 0).caption("pie-like allocation family");
  pane4.wedge(
           std::vector<double>{21.0, 18.0, 15.0, 24.0, 22.0},
           std::vector<double>{0.0, 0.8, 0.0, 0.0, 0.5},
           std::vector<std::string>{"infra", "models", "data", "ops", "qa"})
      .label("wedge")
      .palette(pl::PaletteRamp::thermal());

  auto& pane5 = studio.pane2("ribbon + whisk");
  pane5.place(1, 1).grid(pl::GridStyle::Major).caption("band and uncertainty families");
  const auto trial = pluss::linspace(1.0, 10.0, 10, pluss::ScalarType::Float64);
  const auto center = trial.mul(0.45).sin().mul(0.5).add(1.6);
  pane5.ribbon(trial, center, 0.2).label("ribbon");
  pane5.whisk(trial,
              center,
              trial.mul(0.4).cos().abs().mul(0.15).add(0.08),
              trial.mul(0.3).sin().abs().mul(0.18).add(0.1))
      .label("whisk");

  auto& pane6 = studio.pane2("radial");
  pane6.place(1, 2).grid(pl::GridStyle::Major).caption("polar line family");
  const auto theta = pluss::linspace(0.0, 6.283185307179586, 180, pluss::ScalarType::Float64);
  const auto radius = theta.mul(2.5).cos().mul(0.4).add(1.1);
  pane6.radial(theta, radius).label("radial").width(1.7F);

  auto& pane7 = studio.pane2("fieldmap");
  pane7.place(2, 0).grid(pl::GridStyle::Major).caption("heatmap family");
  const auto gx = pluss::linspace(-3.0, 3.0, 36, pluss::ScalarType::Float64);
  const auto gy = pluss::linspace(-3.0, 3.0, 36, pluss::ScalarType::Float64);
  const auto gz = peaks_like(36, 36);
  pane7.scale_guide();
  pane7.fieldmap(gx, gy, gz).label("fieldmap").palette(pl::PaletteRamp::lagoon());

  auto& pane8 = studio.pane2("contour");
  pane8.place(2, 1).grid(pl::GridStyle::Major).caption("iso-line family");
  pane8.contour(gx, gy, gz, std::vector<double>{-4.0, -2.0, -1.0, 0.0, 1.0, 2.0, 4.0})
      .label("contour")
      .annotate_levels();

  auto& pane9 = studio.pane2("isoband");
  pane9.place(2, 2).grid(pl::GridStyle::Major).caption("filled contour family");
  pane9.isoband(gx, gy, gz, std::vector<double>{-4.0, -2.0, -1.0, 0.0, 1.0, 2.0, 4.0})
      .label("isoband")
      .palette(pl::PaletteRamp::ocean());

  studio.save_svg((root / "outputs" / "svg" / "plusspl_all_2d_families.svg").string());
  studio.save_pdf((root / "outputs" / "pdf" / "plusspl_all_2d_families.pdf").string());
  studio.save_pdf((root / "outputs" / "pdfs" / "plusspl_all_2d_families.pdf").string());
  return 0;
}
