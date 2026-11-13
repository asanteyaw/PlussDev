import sys, pathlib
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[2] / "specs"))
from _common import SURFACE
FIELD = """constexpr std::int64_t n = 24;
auto coord = pluss::series::linspace(-2.0, 2.0, n, pluss::ScalarType::Float64);
std::vector<double> field(static_cast<std::size_t>(n * n));
for (std::int64_t i = 0; i < n; ++i)
  for (std::int64_t j = 0; j < n; ++j) {
    const double x = -2.0 + 4.0 * i / (n - 1), y = -2.0 + 4.0 * j / (n - 1);
    field[static_cast<std::size_t>(i * n + j)] = std::exp(-(x * x + y * y)) - 0.4 * std::exp(-((x - 1) * (x - 1) + (y + 1) * (y + 1)));
  }
auto z = pluss::make_series(field, {n, n});"""
PAGE = dict(
    slug="annotate_levels",
    title="Trace::annotate_levels",
    signature="""bool Trace::annotate_levels() const noexcept

Trace& Trace::annotate_levels(bool enable) noexcept""",
    intro="""Draws the numeric value of each level on a contour-style trace, and the getter reports the setting. It is off by default. When it is on, every contour line, band or ridge level is labelled with its value (three significant digits) where the line is long enough to carry the text, so the figure can be read without a colour bar.

It applies to the level-based kinds: `contour`, `isoband` and `ridge3`, and to a `fieldmap`, whose labels show the values of the cells. On other kinds the call is stored and has no effect. The setter takes its argument without a default: `annotate_levels()` with no argument is always the getter, and writing `annotate_levels(true)` is how to turn the labels on.""",
    parameters="""**enable** (`bool`, setter only)
    True to draw the level labels, false to remove them.""",
    returns="""`bool` (getter)
    Whether level labels are drawn, false by default.

`Trace&` (setter)
    The same trace, for chaining.""",
    examples=[
        dict(id="contour", text="Level labels on a contour plot. The same contour lines without labels and with `annotate_levels(true)`: each ring carries the value of its level.",
             code=FIELD + """

auto fig = studio("annotate_contour");
fig.mosaic(1, 2);
fig.pane2("no labels").contour(coord, coord, z);
fig.pane2("annotate_levels(true)").contour(coord, coord, z).annotate_levels(true);
fig.save_svg("figure.svg");"""),
        dict(id="readback", text="Reading the setting back. It is false by default; the setter turns it on and returns the trace; calling the getter with no argument never changes anything.",
             code=FIELD + """

auto fig = studio("annotate_readback");
auto& pane = fig.pane2("Annotate");
Trace& t = pane.contour(coord, coord, z);
std::cout << "default: " << t.annotate_levels() << "\\n";
t.annotate_levels(true);
std::cout << "after annotate_levels(true): " << t.annotate_levels() << "\\n";
t.annotate_levels(false);
std::cout << "after annotate_levels(false): " << t.annotate_levels() << "\\n";
t.annotate_levels(true);
fig.save_svg("figure.svg");"""),
        dict(id="bands", text="Level labels on filled bands. An `isoband` fills the regions between levels, and `annotate_levels(true)` writes the level values on its boundaries, which replaces a colour bar.",
             code=FIELD + """

auto fig = studio("annotate_isoband");
fig.mosaic(1, 2);
fig.pane2("isoband").isoband(coord, coord, z);
fig.pane2("isoband, annotated").isoband(coord, coord, z).annotate_levels(true);
fig.save_svg("figure.svg");"""),
        dict(id="kinds", text="Which kinds draw labels. Each is drawn with and without the labels and the SVG text compared: a contour, an isoband, a ridge surface and a field map change; a line and a surface do not.",
             code=FIELD + """

auto changes = [&](auto make, bool is3d) {
  std::string svg[2];
  for (int i = 0; i < 2; ++i) {
    auto fig = studio("annotate_kinds");
    Pane& pane = is3d ? fig.pane3("p") : fig.pane2("p");
    Trace& t = make(pane);
    if (i == 1) t.annotate_levels(true);
    svg[i] = fig.to_svg();
  }
  return svg[0] != svg[1];
};
std::cout << "contour:  " << changes([&](Pane& p) -> Trace& { return p.contour(coord, coord, z); }, false) << "\\n";
std::cout << "isoband:  " << changes([&](Pane& p) -> Trace& { return p.isoband(coord, coord, z); }, false) << "\\n";
std::cout << "fieldmap: " << changes([&](Pane& p) -> Trace& { return p.fieldmap(coord, coord, z); }, false) << "\\n";
std::cout << "ridge3:   " << changes([&](Pane& p) -> Trace& { return p.ridge3(coord, coord, z); }, true) << "\\n";
std::cout << "sheet:    " << changes([&](Pane& p) -> Trace& { return p.sheet(coord, coord, z); }, true) << "\\n";
std::cout << "trend:    " << changes([&](Pane& p) -> Trace& { return p.trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3}); }, false) << "\\n";
auto fig = studio("annotate_kinds_out");
fig.pane2("fieldmap, annotated").fieldmap(coord, coord, z).annotate_levels(true);
fig.save_svg("figure.svg");"""),
    ],
    notes="""- Labels are drawn only where a contour line is long enough to hold the text, so a tiny closed ring carries no label.
- The labelled values are the contour levels, which are chosen automatically from the data or given when the trace is made.
- `annotate_levels` is `noexcept`.""",
    see=["[`Pane::contour`](../pane/contour.md), [`Pane::isoband`](../pane/isoband.md), [`Pane::ridge3`](../pane/ridge3.md): the kinds with levels.", "[`Trace::palette`](palette.md), [`Trace::color_limits`](color_limits.md): how the levels are coloured."],
)
