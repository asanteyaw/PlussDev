import sys, pathlib
sys.path.insert(0, str(pathlib.Path(__file__).parent))
from _common import *
PAGE = dict(
    slug="Theme",
    summary="The colours a figure is drawn in.",
    title="Theme",
    signature="""class Theme {
 public:
  Theme() = default;
  Theme(std::string name, std::vector<Rgba> accents, Rgba background, Rgba foreground, PaletteRamp surface);

  const std::string& name() const noexcept;
  const std::vector<Rgba>& accents() const noexcept;
  const Rgba& background() const noexcept;
  const Rgba& foreground() const noexcept;
  const PaletteRamp& surface_palette() const noexcept;
  Rgba accent(std::size_t index) const noexcept;

  static Theme ocean();
  static Theme paper();
};

namespace themes {
Theme ocean();
Theme paper();
}""",
    intro="""A `Theme` is the set of colours that a figure is drawn in. It bundles a name, a list of **accent** colours that successive traces take in turn, a **background**, a **foreground** that
axes, labels and text are drawn in, and a **surface palette**, a [`PaletteRamp`](PaletteRamp.md) that surfaces and fields start with. Every colour a figure draws that you have not set yourself comes from its theme.

A theme is given to a `Studio` when it is made (`Studio fig("name", Theme::ocean())`) or later with [`Studio::theme`](../studio/theme.md). It is copied into each pane when the pane is created, so a change of the studio's theme
affects only the panes created afterwards, and each pane's copy is reachable and editable through `theme_tokens()` on the pane and on the studio. Two presets ship with the library, `Theme::paper()` (the default) and `Theme::ocean()`; the
free functions `themes::paper()` and `themes::ocean()` return the same values.

`accent(index)` returns the accent for the index-th series. It wraps: an index beyond the end of the accents starts again at the first, so any number of series can be coloured. A trace's own [`stroke`](../trace/stroke.md) overrides its accent.""",
    table="""| Member | Meaning |
|---|---|
| `name()` | The theme's name. `paper` and `ocean` for the presets. |
| `accents()`, `accent(i)` | The accent colours, and the colour for series `i` (wrapping). |
| `background()` | The colour of the figure's background. |
| `foreground()` | The colour of axes, tick labels, titles and other text. |
| `surface_palette()` | The [`PaletteRamp`](PaletteRamp.md) that surface traces start with. |
| `Theme::paper()`, `Theme::ocean()` | The two presets. |""",
    examples=[
        dict(id="gallery", text="The two presets on the same figure, each drawn as its own studio: five lines (so the accents show in order) and a surface (so the surface palette shows). Only the theme differs.",
             code=SURFACE + """

const Theme themes_to_show[] = {Theme::paper(), Theme::ocean()};
for (int i = 0; i < 2; ++i) {
  Studio fig(themes_to_show[i].name(), themes_to_show[i]);
  fig.mosaic(1, 2);
  auto& lines = fig.pane2(themes_to_show[i].name() + ": accents");
  for (int k = 0; k < 5; ++k) {
    std::vector<double> x{1, 2, 3, 4, 5}, y;
    for (double v : x) y.push_back(k + 0.3 * std::sin(v + k));
    lines.trend(x, y).label("series " + std::to_string(k + 1));
  }
  lines.legend(KeyPlace::OutsideEast);
  auto& surface = fig.pane3(themes_to_show[i].name() + ": surface");
  surface.sheet(coord, coord, z).shade(ShadeMode::Smooth);
  if (i == 1) fig.save_svg("figure.svg");
}"""),
        dict(id="paper", text="The same figure with the default `paper` theme, to compare with Example 1. The figure is the one that example saves for `ocean`: five lines and a surface.",
             code=SURFACE + """

auto fig = studio("theme_paper");
fig.mosaic(1, 2);
auto& lines = fig.pane2("paper: accents");
for (int k = 0; k < 5; ++k) {
  std::vector<double> x{1, 2, 3, 4, 5}, y;
  for (double v : x) y.push_back(k + 0.3 * std::sin(v + k));
  lines.trend(x, y).label("series " + std::to_string(k + 1));
}
lines.legend(KeyPlace::OutsideEast);
auto& surface = fig.pane3("paper: surface");
surface.sheet(coord, coord, z).shade(ShadeMode::Smooth);
fig.save_svg("figure.svg");"""),
        dict(id="read", text="What a theme holds. For each preset the name, the number of accents, the background and foreground and the first accents; and `accent` wrapping: for a theme with N accents, index N is index 0 again.",
             code="""auto c = [](const Rgba& v) { return std::to_string(static_cast<int>(v.red * 255 + 0.5)) + "," + std::to_string(static_cast<int>(v.green * 255 + 0.5)) + "," + std::to_string(static_cast<int>(v.blue * 255 + 0.5)); };
for (const Theme& t : {Theme::paper(), Theme::ocean()}) {
  std::cout << t.name() << ": " << t.accents().size() << " accents, background (" << c(t.background()) << "), foreground (" << c(t.foreground()) << "), surface palette " << t.surface_palette().name() << "\\n";
  for (std::size_t i : {std::size_t{0}, std::size_t{1}, t.accents().size()})
    std::cout << "  accent(" << i << ") = (" << c(t.accent(i)) << ")\\n";
}
std::cout << "themes::paper() has the same name: " << (themes::paper().name() == Theme::paper().name()) << "\\n";
auto fig = studio("theme_read");
fig.pane2("theme").trend(std::vector<double>{1, 2}, std::vector<double>{1, 2});
fig.save_svg("figure.svg");"""),
        dict(id="custom", text="A theme built by hand: a dark one. Four accents, a near-black background, a light foreground and the `lagoon` palette. Passed to the studio constructor, it governs every pane created afterwards.",
             code=SURFACE + """

const Theme dark("dark",
                 {Rgba::from_u8(255, 170, 60), Rgba::from_u8(90, 200, 250), Rgba::from_u8(150, 230, 120), Rgba::from_u8(240, 120, 170)},
                 Rgba::from_u8(24, 26, 32), Rgba::from_u8(225, 228, 235), PaletteRamp::lagoon());
Studio fig("theme_custom", dark);
fig.mosaic(1, 2);
auto& lines = fig.pane2("dark: accents");
for (int k = 0; k < 4; ++k) {
  std::vector<double> x{1, 2, 3, 4, 5}, y;
  for (double v : x) y.push_back(k + 0.3 * std::sin(v + k));
  lines.trend(x, y).label("series " + std::to_string(k + 1));
}
lines.legend(KeyPlace::OutsideEast);
auto& surface = fig.pane3("dark: surface");
surface.sheet(coord, coord, z).shade(ShadeMode::Smooth);
std::cout << fig.theme_tokens().name() << " with " << fig.theme_tokens().accents().size() << " accents\\n";
fig.save_svg("figure.svg");"""),
        dict(id="after", text="A theme applies to panes created afterwards. The first pane is created under `paper`; the studio is switched to `ocean`; the second pane is created under `ocean`. The two panes report different themes, and the first keeps its own.",
             code="""auto fig = studio("theme_after");
fig.mosaic(1, 2);
{
  auto& first = fig.pane2("first");
  first.trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
  std::cout << "first pane:  " << first.theme_tokens().name() << "\\n";
}
fig.theme(Theme::ocean());
{
  auto& second = fig.pane2("second");
  second.trend(std::vector<double>{1, 2, 3}, std::vector<double>{3, 1, 4});
  std::cout << "second pane: " << second.theme_tokens().name() << "\\n";
}
std::cout << "the studio:  " << fig.theme_tokens().name() << ", the first pane still: " << fig.panes()[0].theme_tokens().name() << "\\n";
fig.save_svg("figure.svg");"""),
    ],
    notes="""- Change the theme before creating panes. To restyle an existing pane, edit its `theme_tokens()`, or rebuild the figure.
- A `Theme` is a value, so one can be built once and passed to many studios, which is how a house style is shared by every figure of a report.
- The accents are used in order, so put the colours you want for the first series first. A palette of four accents repeats every four series.""",
    see=["[`Studio::theme`](../studio/theme.md), [`Studio::theme_tokens`](../studio/theme_tokens.md): set and reach the studio's theme.", "[`Pane::theme_tokens`](../pane/theme_tokens.md): the theme of one pane.", "[`PaletteRamp`](PaletteRamp.md), [`Rgba`](Rgba.md): the pieces a theme is built from."],
)
