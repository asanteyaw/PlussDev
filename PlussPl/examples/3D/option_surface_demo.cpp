#include <pluss/pluss.h>

#include <filesystem>
#include <string>
#include <vector>

int main() {
  namespace pl = pluss::pl;
  const std::filesystem::path root = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
  std::filesystem::create_directories(root / "outputs" / "svg");
  std::filesystem::create_directories(root / "outputs" / "pdf");

  const auto maturity_bins = pluss::linspace(1.0, 4.0, 4, pluss::ScalarType::Float64);
  const auto moneyness_bins = pluss::linspace(1.0, 12.0, 12, pluss::ScalarType::Float64);
  const auto avg_price = pluss::make_series(
                             std::vector<double>{
                                 1.0, 1.8, 3.0, 5.5, 10.0, 14.0, 7.0, 3.8, 2.0, 1.1, 0.6, 0.3,
                                 3.0, 4.5, 7.5, 12.0, 20.0, 28.0, 17.0, 9.0, 4.8, 2.5, 1.2, 0.7,
                                 7.0, 9.5, 14.0, 21.0, 36.0, 48.0, 31.0, 18.0, 10.0, 5.2, 2.4, 1.2,
                                 12.0, 16.0, 22.0, 31.0, 50.0, 73.0, 38.0, 20.0, 11.5, 5.5, 2.8, 1.4,
                             },
                             {4, 12})
                             .transpose(0, 1);

  pl::Studio studio("plusspl-option-surface", pl::Theme::paper());
  studio.size(1180.0, 780.0);
  studio.headline("Surface of Option Prices");

  auto& pane = studio.pane3("Option surface");
  pane.grid(pl::GridStyle::Major)
      .x_label("Maturity")
      .y_label("Moneyness")
      .z_label("Average Option Price")
      .y_limit_method(pl::LimitMethod::Tight)
      .z_limits({0.0, 120.0})
      .x_ticks(maturity_bins,
               std::vector<std::string>{"179<=T<=252", "129<=T<179", "56<=T<129", "T<56"})
      .y_ticks(moneyness_bins,
               std::vector<std::string>{"[0.70, 0.75)", "[0.75, 0.80)", "[0.80, 0.85)", "[0.85, 0.90)",
                                        "[0.90, 0.95)", "[0.95, 1.00)", "[1.00, 1.05)", "[1.05, 1.10)",
                                        "[1.10, 1.15)", "[1.15, 1.20)", "[1.20, 1.25)", "[1.25, 1.30)"})
      .z_ticks(std::vector<double>{0.0, 20.0, 40.0, 60.0, 80.0, 100.0, 120.0},
               std::vector<std::string>{"0", "20", "40", "60", "80", "100", "120"});
  pane.lens().look(-37.5, 30.0).project(pl::ProjectionKind::Orthographic);
  pane.sheet(maturity_bins, moneyness_bins, avg_price)
      .label("option surface")
      .shade(pl::ShadeMode::Smooth)
      .lighting(pl::LightingMode::None)
      .transparent(false)
      .palette(pl::PaletteRamp::lagoon());

  studio.save_svg((root / "outputs" / "svg" / "plusspl_option_surface.svg").string());
  studio.save_pdf((root / "outputs" / "pdf" / "plusspl_option_surface.pdf").string());
  return 0;
}
