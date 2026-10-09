#include <pluss/pluss.h>

#include <array>
#include <filesystem>
#include <span>

namespace {

pluss::Series wave_field(std::int64_t rows, std::int64_t cols) {
  const auto x_axis = pluss::series::linspace(0.0, 25.0, cols, pluss::ScalarType::Float64).reshape({1, cols});
  const auto y_axis = pluss::series::linspace(0.0, 30.0, rows, pluss::ScalarType::Float64).reshape({rows, 1});
  const auto x = x_axis.repeat({rows, 1});
  const auto y = y_axis.repeat({1, cols});

  const auto envelope = x.sub(11.0).pow(2.0).add(y.sub(10.0).pow(2.0).mul(0.7)).mul(-0.04).exp();
  const auto ripple = x.mul(0.48).sin().mul(y.mul(0.28).cos()).mul(8.0)
                          .add(x.mul(0.9).sub(y.mul(0.25)).cos().mul(2.2));
  return envelope.mul(ripple);
}

} // namespace

int main() {
  namespace pl = pluss::pl;
  const std::filesystem::path root = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
  std::filesystem::create_directories(root / "outputs" / "svg");
  std::filesystem::create_directories(root / "outputs" / "pdf");

  const auto x = pluss::series::linspace(0.0, 25.0, 26, pluss::ScalarType::Float64);
  const auto y = pluss::series::linspace(0.0, 30.0, 25, pluss::ScalarType::Float64);
  const auto z = wave_field(25, 26);

  pl::Studio studio("plusspl-wave-mesh", pl::Theme::paper());
  studio.size(1180.0, 720.0);
  studio.headline("Pluss Wire Surface");

  auto& pane = studio.pane3("Wave mesh");
  pane.grid(pl::GridStyle::Major)
      .x_label("x")
      .y_label("y")
      .z_label("z")
      .caption("front-facing wireframe with MATLAB-like view defaults");
  pane.lens().look(-37.5, 30.0);
  pane.lattice(x, y, z, z)
      .label("wave mesh")
      .palette(pl::PaletteRamp::lagoon())
      .width(1.0F);

  studio.save_svg((root / "outputs" / "svg" / "plusspl_wave_mesh.svg").string());
  studio.save_pdf((root / "outputs" / "pdf" / "plusspl_wave_mesh.pdf").string());
  return 0;
}
