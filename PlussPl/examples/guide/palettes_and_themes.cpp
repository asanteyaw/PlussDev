// Colour and style as data: the palette generators, applying a palette to a field, the themes and contexts that restyle a whole figure, and the
// small helpers that tidy a pane.
#include <pluss/pl/plusspl.h>
#include <pluss/pluss.h>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>

using namespace pluss::pl;

static std::string out(const char *name) {
  const char *dir = std::getenv("PLUSSMA_GUIDE_ASSETS");
  return ((dir ? std::filesystem::path(dir) : std::filesystem::path(".")) / name).string();
}


int main() {
  // 1. Every palette generator returns a PaletteRamp, a list of stops you can sample between 0 and 1.
  const std::vector<std::pair<std::string, PaletteRamp>> palettes{
      {"flash deep", flash::flash_palette()},         {"color_palette ocean", flash::color_palette("ocean")}, {"hls", flash::hls_palette(6)},
      {"husl", flash::husl_palette(6)},               {"cubehelix", flash::cubehelix_palette(6)},               {"viridis", flash::mpl_palette("viridis")},
      {"light blue", flash::light_palette(Rgba::from_u8(40, 90, 170), 6)}, {"dark red", flash::dark_palette(Rgba::from_u8(180, 40, 40), 6)},
      {"diverging", flash::diverging_palette(Rgba::from_u8(40, 90, 170), Rgba::from_u8(190, 60, 40), 7)},
      {"blend", flash::blend_palette({Rgba::from_u8(255, 255, 200), Rgba::from_u8(200, 60, 60)})}, {"crayon", flash::crayon_palette()}, {"xkcd", flash::xkcd_palette()}};
  for (const auto &[name, ramp] : palettes) {
    const auto first = ramp.sample(0.0), last = ramp.sample(1.0);
    std::cout << name << ": first (" << static_cast<int>(first.red * 255) << ", " << static_cast<int>(first.green * 255) << ", " << static_cast<int>(first.blue * 255) << ") last ("
              << static_cast<int>(last.red * 255) << ", " << static_cast<int>(last.green * 255) << ", " << static_cast<int>(last.blue * 255) << ")\n";
  }

  // 2. Apply them: the same ramp of values drawn as a strip with each palette.
  auto fig = studio("palettes");
  fig.mosaic(4, 3);
  fig.headline("Palette generators");
  auto x = pluss::series::linspace(0.0, 1.0, 24, pluss::ScalarType::Float64);
  auto y = pluss::series::zeros({1}, pluss::ScalarType::Float64);
  auto z = x.unsqueeze(0);
  for (const auto &[name, ramp] : palettes) {
    auto &pane = fig.pane2(name);
    pane.fieldmap(x, y, z).palette(ramp);
    flash::tickless(pane);
  }
  fig.show();

  // 3. Themes and contexts restyle the whole figure at once; the same chart in three styles.
  auto data = pluss::series::linspace(0.0, 6.28, 60, pluss::ScalarType::Float64);
  auto styled = studio("themes");
  styled.mosaic(1, 3);
  for (const auto &[label, theme] : std::vector<std::pair<std::string, Theme>>{{"paper", Theme::paper()}, {"ocean", Theme::ocean()}, {"flash lightgrid + deep", flash::flash_theme()}}) {
    flash::set_theme(styled, theme);
    auto &pane = styled.pane2(label);
    pane.trend(data, data.sin());
    pane.trend(data, data.cos());
    flash::despine(pane);
    flash::horizontal_grid(pane);
  }
  flash::set_context(styled, flash::FlashContext::Talk);
  flash::tight_layout(styled);
  styled.show();

  // 4. The datasets are plain data: Dataset::from_table wraps any DataFrame so the flash functions can take it by column name.
  auto frame = pluss::table::DataFrame({{"x", pluss::make_series({1.0, 2.0, 3.0, 4.0})}, {"y", pluss::make_series({2.0, 3.5, 3.0, 5.0})}});
  const auto wrapped = flash::Dataset::from_table(frame);
  std::cout << "wrapped dataset: " << wrapped.nrows() << " rows, " << wrapped.ncols() << " columns; size() reports (" << wrapped.size().first << ", " << wrapped.size().second << ")\n";
  std::cout << "bundled datasets: " << flash::get_dataset_names().numel() << "\n";
}
