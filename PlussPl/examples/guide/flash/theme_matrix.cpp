// The flash style system: three independent choices (style, palette, context) plus a few layout switches, applied to the same chart.
#include <pluss/pl/plusspl.h>
#include <pluss/pluss.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>

using namespace pluss::pl;
using flash::FlashContext;
using flash::FlashPalette;
using flash::FlashStyle;

static std::string out(const char *name) {
  const char *dir = std::getenv("PLUSSMA_GUIDE_ASSETS");
  return ((dir ? std::filesystem::path(dir) : std::filesystem::path(".")) / name).string();
}

static void draw(Pane &pane, const char *title) {
  auto x = pluss::series::linspace(0.0, 6.28, 60, pluss::ScalarType::Float64);
  pane.trend(x, x.sin()).label("sin");
  pane.trend(x, x.cos()).label("cos");
  pane.trend(x, (x * 0.5).sin()).label("half");
  pane.title(title);
}

int main() {
  // 1. Six styles on one chart: dark/light/sand, each with or without a grid.
  const std::vector<std::pair<const char *, FlashStyle>> styles{{"Darkgrid", FlashStyle::Darkgrid}, {"Lightgrid", FlashStyle::Lightgrid}, {"Sandgrid", FlashStyle::Sandgrid},
                                                                {"Dark", FlashStyle::Dark},         {"Light", FlashStyle::Light},         {"Sand", FlashStyle::Sand}};
  auto fig = studio("flash_styles");
  fig.mosaic(2, 3);
  for (const auto &[name, style] : styles) {
    auto &pane = fig.pane2(name);
    draw(pane, name);
    flash::set_theme(pane, style);
  }
  fig.show();

  // 2. Six palettes: the colours a theme assigns to successive traces.
  const std::vector<std::pair<const char *, FlashPalette>> palettes{{"Deep", FlashPalette::Deep},     {"Muted", FlashPalette::Muted},   {"Pastel", FlashPalette::Pastel},
                                                                    {"Bright", FlashPalette::Bright}, {"Dark", FlashPalette::Dark},     {"Colorblind", FlashPalette::Colorblind}};
  auto pal = studio("flash_palettes");
  pal.mosaic(2, 3);
  for (const auto &[name, palette] : palettes) {
    flash::set_theme(pal, flash::flash_theme(FlashStyle::Lightgrid, palette));
    auto &pane = pal.pane2(name);
    draw(pane, name);
    const auto ramp = flash::flash_palette(palette);
    const auto c = ramp.sample(0.0);
    std::cout << name << " first colour (" << static_cast<int>(c.red * 255) << ", " << static_cast<int>(c.green * 255) << ", " << static_cast<int>(c.blue * 255) << ")\n";
  }
  pal.show();

  // 3. One options struct for style, context and layout: a talk-sized figure with a horizontal grid, no border and the legend outside.
  flash::FlashThemeOptions options;
  options.style = FlashStyle::Sandgrid;
  options.context = FlashContext::Talk;
  options.borderless = true;
  options.horizontal_grid = true;
  options.legend_place = KeyPlace::OutsideEast;
  auto talk = studio("flash_talk");
  flash::set_theme(talk, options);
  auto &pane = talk.pane2("Talk");
  draw(pane, "Talk context");
  talk.show();

  // 4. Contexts change only the scale: draw the same chart in each and print the canvas it picked.
  for (const auto &[name, context] : std::vector<std::pair<const char *, FlashContext>>{{"paper", FlashContext::Paper}, {"notebook", FlashContext::Notebook}, {"talk", FlashContext::Talk}, {"poster", FlashContext::Poster}}) {
    auto s = studio(std::string("flash_context_") + name);
    flash::set_context(s, context);
    auto &p = s.pane2(name);
    draw(p, name);
    const auto extent = s.extent();
    std::cout << name << " canvas " << extent.width << " x " << extent.height << "\n";
    s.show();
  }
}
