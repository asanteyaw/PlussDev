#include <pluss/pluss.h>

#include <array>
#include <cmath>
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

  pl::Studio gallery("plusspl-3d", pl::Theme::ocean());
  gallery.size(1320.0, 980.0);
  gallery.mosaic(2, 2, 22.0, 20.0);
  gallery.headline("Pluss 3D Spatial Gallery");
  gallery.trace_key(pl::KeyDock::East)
      .x_caption("terrain-plane", "shared terrain x axis")
      .y_caption("terrain-plane", "shared terrain y axis")
      .z_caption("terrain-plane", "elevation family");

  auto xs = pluss::linspace(-3.0, 3.0, 45, pluss::ScalarType::Float64);
  auto ys = pluss::linspace(-3.0, 3.0, 45, pluss::ScalarType::Float64);
  auto zz = peaks_like(45, 45);
  const auto bands = std::vector<double>{-6.0, -3.0, -1.0, 0.0, 1.5, 3.0, 5.5, 8.0};

  auto& skin = gallery.pane3("Surface field");
  skin.grid(pl::GridStyle::Major)
      .caption("lit sheet with elevation cue planes")
      .place(0, 0, 1, 2)
      .scale_guide();
  skin.align_xy("terrain-plane");
  skin.align_z("terrain-plane");
  skin.lens().look(-37.5, 30.0);
  skin.lights().sun_key();
  skin.rule_z(2.4, "crest plane");
  skin.band_z({-1.5, -0.25}, "basin span");
  skin.sheet(xs, ys, zz)
      .label("surface")
      .shade(pl::ShadeMode::Smooth)
      .lighting(pl::LightingMode::Gouraud)
      .material(pl::MaterialProfile::Shiny)
      .palette(pl::PaletteRamp::lagoon());
  skin.lattice(xs, ys, zz, zz)
      .label("wireframe")
      .palette(pl::PaletteRamp::lagoon())
      .width(0.8F);

  auto& ridge = gallery.pane3("Level ridges");
  ridge.grid(pl::GridStyle::Major)
      .caption("stacked elevation ridges")
      .place(1, 0)
      .scale_guide()
      .color_limits({-6.0, 8.0});
  ridge.align_xy("terrain-plane");
  ridge.align_z("terrain-plane");
  ridge.lens().look(-42.0, 24.0);
  ridge.lights().headlight();
  ridge.rule_z(0.0, "sea level");
  ridge.ridge3(xs, ys, zz, bands)
      .label("ridge stack")
      .width(1.2F)
      .palette(pl::PaletteRamp::thermal())
      .annotate_levels();
  ridge.pin("ridge focus", pl::Vec3d{0.35, -0.45, 1.5}, 42.0, -24.0);

  auto& orbit = gallery.pane3("Orbital trace");
  orbit.grid(pl::GridStyle::Major).caption("trajectory with staging shell").place(1, 1);
  orbit.lens().look(-22.0, 20.0);
  orbit.lights().headlight();
  orbit.band_z({1.2, 2.1}, "transfer shell");
  const auto t = pluss::linspace(0.0, 10.0, 321, pluss::ScalarType::Float64);
  const auto x3 = t.mul(2.0).cos();
  const auto y3 = t.mul(2.0).sin();
  const auto z3 = t.mul(0.35);
  orbit.path3(x3, y3, z3).label("helix").width(2.2F);
  const auto exit = t.to_vec<double>()[240];
  orbit.pin("exit arc", pl::Vec3d{std::cos(2.0 * exit), std::sin(2.0 * exit), 0.35 * exit}, 34.0, -18.0);

  gallery.save_svg((root / "outputs" / "svg" / "plusspl_3d_gallery.svg").string());
  gallery.save_pdf((root / "outputs" / "pdf" / "plusspl_3d_gallery.pdf").string());
  gallery.save_pdf((root / "outputs" / "pdfs" / "plusspl_3d_gallery.pdf").string());
  return 0;
}
