#include <pluss/pluss.h>

#include <filesystem>
#include <string>
#include <vector>

int main() {
  namespace pl = pluss::pl;
  const std::filesystem::path root = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
  std::filesystem::create_directories(root / "outputs" / "svg");
  std::filesystem::create_directories(root / "outputs" / "pdf");

  pl::Studio studio("plusspl-discrete", pl::Theme::paper());
  studio.size(1180.0, 720.0);
  studio.mosaic(1, 2, 22.0, 18.0);
  studio.headline("Pluss Discrete Showcase");

  auto& pulse = studio.pane2("Spike train");
  pulse.place(0, 0)
      .grid(pl::GridStyle::Major)
      .x_label("sample")
      .y_label("amplitude")
      .caption("stem-like discrete sequence");
  auto x = pluss::linspace(1.0, 12.0, 12, pluss::ScalarType::Float64);
  auto y = x.mul(0.75).sin().abs().mul(1.4).add(x.mul(0.18).cos().mul(0.25));
  pulse.spike(x, y, 0.0).label("sequence").stroke(pl::Rgba::from_u8(25, 98, 156));

  auto& share = studio.pane2("Wedge share");
  share.place(0, 1).caption("pie-like allocation view");
  share.wedge(
           std::vector<double>{18.0, 27.0, 14.0, 22.0, 19.0},
           std::vector<double>{0.0, 1.0, 0.0, 0.0, 0.7},
           std::vector<std::string>{"Infra", "Models", "Data", "Ops", "QA"})
      .label("budget mix")
      .palette(pl::PaletteRamp::thermal());

  studio.save_svg((root / "outputs" / "svg" / "plusspl_discrete_showcase.svg").string());
  studio.save_pdf((root / "outputs" / "pdf" / "plusspl_discrete_showcase.pdf").string());
  return 0;
}
