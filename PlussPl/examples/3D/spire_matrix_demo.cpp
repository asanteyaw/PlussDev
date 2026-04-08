#include <pluss/pluss.h>

#include <filesystem>
#include <vector>

int main() {
  namespace pl = pluss::pl;
  const std::filesystem::path root = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
  std::filesystem::create_directories(root / "outputs" / "svg");
  std::filesystem::create_directories(root / "outputs" / "pdf");

  const auto sectors = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0}, {5});
  const auto horizons = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4});
  const auto z = pluss::make_series(
      std::vector<double>{
          4.0, 7.0, 6.0, 5.0, 3.5,
          6.5, 10.0, 9.0, 7.0, 4.5,
          8.0, 12.5, 11.0, 8.5, 5.0,
          9.5, 14.0, 12.0, 9.0, 6.0,
      },
      {4, 5});

  pl::Studio studio("plusspl-spire", pl::Theme::paper());
  studio.size(1040.0, 760.0);
  studio.headline("Pluss Spire Matrix");

  auto& pane = studio.pane3("Spire field");
  pane.grid(pl::GridStyle::Major)
      .x_label("Sector")
      .y_label("Horizon")
      .z_label("Intensity")
      .x_ticks(sectors, {"A", "B", "C", "D", "E"})
      .y_ticks(horizons, {"Q1", "Q2", "Q3", "Q4"})
      .scale_guide()
      .caption("3D prism family");
  pane.lens().look(-126.0, 22.0);
  pane.lights().sun_key();
  pane.spire3(sectors, horizons, z, z)
      .label("spires")
      .palette(pl::PaletteRamp::meadow())
      .color_limits({3.0, 14.0})
      .shade(pl::ShadeMode::Flat)
      .lighting(pl::LightingMode::Flat);

  studio.save_svg((root / "outputs" / "svg" / "plusspl_spire_matrix.svg").string());
  studio.save_pdf((root / "outputs" / "pdf" / "plusspl_spire_matrix.pdf").string());
  return 0;
}
