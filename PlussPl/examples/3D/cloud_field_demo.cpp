#include <pluss/pluss.h>

#include <filesystem>

int main() {
  namespace pl = pluss::pl;
  const std::filesystem::path root = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
  std::filesystem::create_directories(root / "outputs" / "svg");
  std::filesystem::create_directories(root / "outputs" / "pdf");

  auto t = pluss::series::linspace(-2.6, 2.6, 84, pluss::ScalarType::Float64);
  auto x = t.cos().mul(1.7).add(t.mul(0.6).sin().mul(0.4));
  auto y = t.sin().mul(1.35).add(t.mul(1.2).cos().mul(0.25));
  auto z = t.mul(0.8).sin().mul(1.6).add(t.mul(0.35));
  auto size = z.abs().mul(38.0).add(12.0);

  pl::Studio studio("plusspl-cloud-field", pl::Theme::ocean());
  studio.size(980.0, 720.0);
  studio.headline("Pluss 3D Cloud");

  auto& pane = studio.pane3("Spatial cloud");
  pane.grid(pl::GridStyle::Major)
      .parallel_xy_labels(true)
      .x_label("long term average of pragmatic approach")
      .y_label("cross-sectional moneyness contour alignment")
      .z_label("Short term median of the random approach")
      .scale_guide()
      .caption("3D scatter-like point field");
  pane.lens().look(-38.0, 24.0);
  pane.cloud3(x, y, z, size, z)
      .label("cloud field")
      .palette(pl::PaletteRamp::thermal())
      .color_limits({-2.5, 2.5});

  studio.save_svg((root / "outputs" / "svg" / "plusspl_cloud_field.svg").string());
  studio.save_pdf((root / "outputs" / "pdf" / "plusspl_cloud_field.pdf").string());
  return 0;
}
