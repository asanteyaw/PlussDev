#include <pluss/pluss.h>

#include <cmath>
#include <filesystem>
#include <vector>

namespace {

pluss::Series basin_field(std::int64_t rows, std::int64_t cols) {
  std::vector<double> values;
  values.reserve(static_cast<std::size_t>(rows * cols));
  for (std::int64_t i = 0; i < rows; ++i) {
    const double y = -2.5 + 5.0 * static_cast<double>(i) / static_cast<double>(rows - 1);
    for (std::int64_t j = 0; j < cols; ++j) {
      const double x = -3.0 + 6.0 * static_cast<double>(j) / static_cast<double>(cols - 1);
      const double z = 1.8 * std::sin(1.2 * x) * std::cos(0.9 * y) +
                       0.9 * std::exp(-0.35 * ((x - 1.1) * (x - 1.1) + (y + 0.4) * (y + 0.4))) -
                       1.2 * std::exp(-0.55 * ((x + 1.4) * (x + 1.4) + (y - 0.8) * (y - 0.8)));
      values.push_back(z);
    }
  }
  return pluss::make_series(values, {rows, cols});
}

} // namespace

int main() {
  namespace pl = pluss::pl;
  const std::filesystem::path root = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
  std::filesystem::create_directories(root / "outputs" / "svg");
  std::filesystem::create_directories(root / "outputs" / "pdf");

  const auto x = pluss::linspace(-3.0, 3.0, 15, pluss::ScalarType::Float64);
  const auto y = pluss::linspace(-2.5, 2.5, 12, pluss::ScalarType::Float64);
  const auto z = basin_field(12, 15);

  pl::Studio studio("plusspl-fieldmap", pl::Theme::ocean());
  studio.size(980.0, 720.0);
  studio.headline("Pluss Field Map");

  auto& pane = studio.pane2("Scalar field");
  pane.grid(pl::GridStyle::Major)
      .x_label("x")
      .y_label("y")
      .scale_guide()
      .caption("palette-mapped planar field")
      .color_limits({-2.4, 2.4});
  pane.fieldmap(x, y, z)
      .label("field energy")
      .palette(pl::PaletteRamp::thermal())
      .color_limits({-2.4, 2.4});
  pane.contour(x, y, z, std::vector<double>{-1.5, -0.5, 0.5, 1.5})
      .label("ridges")
      .width(1.0F)
      .annotate_levels();

  studio.save_svg((root / "outputs" / "svg" / "plusspl_fieldmap.svg").string());
  studio.save_pdf((root / "outputs" / "pdf" / "plusspl_fieldmap.pdf").string());
  return 0;
}
