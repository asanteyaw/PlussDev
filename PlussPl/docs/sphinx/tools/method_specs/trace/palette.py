PAGE = dict(
    slug="palette",
    title="Trace::palette",
    signature="""const PaletteRamp& Trace::palette_ramp() const noexcept

Trace& Trace::palette(PaletteRamp palette)""",
    intro="""Sets the colour ramp a trace maps its values through, and `palette_ramp()` reads it back. A new trace starts with the pane's theme surface palette (`thermal` for the default `paper` theme). The ramp is a [`PaletteRamp`](../types/PaletteRamp.md): five presets and any custom list of colours.

The palette colours whatever the kind derives its colour from a value: the cells of a `fieldmap`, the lines and fills of `contour`, `isoband` and `ridge3`, a surface (`sheet`, `lattice`, `spire3`, `isoshell`), a `cloud` that was given a colour series, and the fills of `columns`, `bucket`, `ribbon`, `wedge`, `quartet` and `violin`, which sample the ramp at a fixed position until the trace's colour is set with [`Trace::stroke`](stroke.md), after which the first three take that colour (a pie's slices and a multi-band ribbon keep their palette colours). It has no effect on traces whose colour is the plain [`stroke`](stroke.md): a `trend`, a `spike`, a `whisk`, a `radial` curve, a `path3`, a `cloud3`, and a `cloud` with no colour series. Which values map to the ends of the ramp is set by [`Trace::color_limits`](color_limits.md).""",
    parameters="""**palette** (`PaletteRamp`)
    The ramp to use. It is copied into the trace.""",
    returns="""`const PaletteRamp&` (getter)
    The trace's current ramp.

`Trace&` (setter)
    The same trace, for chaining.""",
    examples=[
        dict(id="field", text="The five presets on one field map. Each pane draws the same grid of values with a different palette; the data and the limits are the same, so only the colours change.",
             code="""constexpr std::int64_t n = 24;
auto coord = pluss::series::linspace(-2.0, 2.0, n, pluss::ScalarType::Float64);
std::vector<double> field(static_cast<std::size_t>(n * n));
for (std::int64_t i = 0; i < n; ++i)
  for (std::int64_t j = 0; j < n; ++j) {
    const double x = -2.0 + 4.0 * i / (n - 1), y = -2.0 + 4.0 * j / (n - 1);
    field[static_cast<std::size_t>(i * n + j)] = std::exp(-(x * x + y * y)) - 0.4 * std::exp(-((x - 1) * (x - 1) + (y + 1) * (y + 1)));
  }
auto z = pluss::make_series(field, {n, n});
const char* names[] = {"thermal", "neutral", "lagoon", "ocean", "meadow"};
auto fig = studio("palette_field");
fig.mosaic(1, 5);
for (int i = 0; i < 5; ++i) {
  PaletteRamp ramp = i == 0 ? PaletteRamp::thermal() : i == 1 ? PaletteRamp::neutral() : i == 2 ? PaletteRamp::lagoon() : i == 3 ? PaletteRamp::ocean() : PaletteRamp::meadow();
  fig.pane2(names[i]).fieldmap(coord, coord, z).palette(ramp);
}
fig.save_svg("figure.svg");"""),
        dict(id="default", text="The default and a custom ramp. A new trace carries the theme's surface palette, read back with `palette_ramp()`; a ramp of three colours replaces it and is read back by name.",
             code="""auto fig = studio("palette_default");
auto& pane = fig.pane2("Palette");
Trace& t = pane.cloud(std::vector<double>{1, 2, 3, 4, 5}, std::vector<double>{1, 3, 2, 5, 4}, std::vector<double>(5, 9.0), std::vector<double>{1, 2, 3, 4, 5});
std::cout << "default: " << t.palette_ramp().name() << " (the theme's is " << pane.theme_tokens().surface_palette().name() << ")\\n";
t.palette(PaletteRamp("duo", {Rgba::from_u8(20, 40, 160), Rgba::from_u8(250, 200, 40)}));
std::cout << "after palette(): " << t.palette_ramp().name() << ", " << t.palette_ramp().stops().size() << " stops\\n";
fig.save_svg("figure.svg");"""),
        dict(id="colored_cloud", text="A cloud coloured by data. The points of a `cloud` made with a colour series are mapped through the palette: the same points with `thermal` and with `ocean`. A cloud with no colour series ignores the palette, which the equality shows.",
             code="""const std::vector<double> x{1, 2, 3, 4, 5, 6, 7, 8}, y{2, 4, 3, 5, 4, 6, 5, 7}, size(8, 9.0), c{1, 2, 3, 4, 5, 6, 7, 8};
auto fig = studio("palette_cloud");
fig.mosaic(1, 2);
fig.pane2("thermal").cloud(x, y, size, c);
fig.pane2("ocean").cloud(x, y, size, c).palette(PaletteRamp::ocean());
std::string svg[2];
for (int i = 0; i < 2; ++i) {
  auto probe = studio("palette_cloud_plain");
  Trace& t = probe.pane2("plain").cloud(x, y);
  if (i == 1) t.palette(PaletteRamp::ocean());
  svg[i] = probe.to_svg();
}
std::cout << "a cloud without colour data ignores the palette: " << (svg[0] == svg[1]) << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="fills", text="Filled kinds sample the palette at a fixed point. Columns, a histogram and a pie each take their fill from the ramp: the same three traces with `thermal` and `meadow`. A single colour for each trace is chosen from the ramp, so a palette with a contrast between its ends gives distinct fills for different kinds.",
             code="""const std::vector<double> x{1, 2, 3, 4, 5}, y{2, 4, 3, 5, 4};
auto fig = studio("palette_fills");
fig.mosaic(2, 3);
for (int i = 0; i < 2; ++i) {
  const PaletteRamp ramp = i == 0 ? PaletteRamp::thermal() : PaletteRamp::meadow();
  const std::string name = i == 0 ? "thermal" : "meadow";
  fig.pane2(name + ": columns").columns(x, y).palette(ramp);
  fig.pane2(name + ": bucket").bucket(std::vector<double>{1, 2, 2, 3, 3, 3, 4, 4, 5}).palette(ramp);
  fig.pane2(name + ": wedge").wedge(std::vector<double>{3, 2, 1, 2}).palette(ramp);
}
fig.save_svg("figure.svg");"""),
        dict(id="surface", text="A palette on a surface, together with its limits. The surface is coloured by height with `lagoon`, and with `lagoon` restricted to the upper half of the heights, so the lower half is all one colour.",
             code="""constexpr std::int64_t n = 16;
auto coord = pluss::series::linspace(-1.0, 1.0, n, pluss::ScalarType::Float64);
std::vector<double> peak(static_cast<std::size_t>(n * n));
for (std::int64_t i = 0; i < n; ++i)
  for (std::int64_t j = 0; j < n; ++j) {
    const double x = -1.0 + 2.0 * i / (n - 1), y = -1.0 + 2.0 * j / (n - 1);
    peak[static_cast<std::size_t>(i * n + j)] = std::exp(-3.0 * (x * x + y * y));
  }
auto z = pluss::make_series(peak, {n, n});
auto fig = studio("palette_surface");
fig.mosaic(1, 2);
fig.pane3("lagoon").sheet(coord, coord, z).shade(ShadeMode::Smooth).palette(PaletteRamp::lagoon());
fig.pane3("lagoon, limits 0.5 to 1").sheet(coord, coord, z).shade(ShadeMode::Smooth).palette(PaletteRamp::lagoon()).color_limits(Range1d{0.5, 1.0});
fig.save_svg("figure.svg");"""),
    ],
    notes="""- A palette is copied into the trace, so changing the original `PaletteRamp` afterwards does not change traces that have already used it.
- Ramps have no direction built in: to colour high values dark instead of light, build a ramp whose stops run the other way.
- For a figure with several panes that must be compared, give every trace the same palette and the same [`color_limits`](color_limits.md).""",
    see=["[`PaletteRamp`](../types/PaletteRamp.md): the five presets and custom ramps, with a rendered comparison.", "[`Trace::color_limits`](color_limits.md): which values map to the ends of the ramp.", "[`Trace::stroke`](stroke.md): the plain colour used by the kinds that ignore the palette."],
)
