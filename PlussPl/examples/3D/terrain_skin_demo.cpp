#include <pluss/pluss.h>

#include <array>
#include <filesystem>
#include <span>

namespace {

pluss::Series peaks_skin(std::int64_t rows, std::int64_t cols) {
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

  const auto x = pluss::linspace(-3.0, 3.0, 41, pluss::ScalarType::Float64);
  const auto y = pluss::linspace(-3.0, 3.0, 41, pluss::ScalarType::Float64);
  const auto z = peaks_skin(41, 41);

  pl::Studio studio("plusspl-terrain-skin", pl::Theme::paper());
  studio.size(1180.0, 760.0);
  studio.headline("Pluss Terrain Skin");

  auto& pane = studio.pane3("Terrain skin");
  pane.grid(pl::GridStyle::Major)
      .x_label("x")
      .y_label("y")
      .z_label("z")
      .caption("sheet plus hidden-front lattice");
  pane.lens().look(-37.5, 30.0);
  pane.sheet(x, y, z)
      .label("surface")
      .shade(pl::ShadeMode::Smooth)
      .lighting(pl::LightingMode::None)
      .palette(pl::PaletteRamp::lagoon());
  pane.lattice(x, y, z, z)
      .label("mesh")
      .palette(pl::PaletteRamp::lagoon())
      .width(0.95F);

  studio.save_svg((root / "outputs" / "svg" / "plusspl_terrain_skin.svg").string());
  studio.save_pdf((root / "outputs" / "pdf" / "plusspl_terrain_skin.pdf").string());
  return 0;
}
