import sys, pathlib
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[2] / "specs"))
from _common import SURFACE
PAGE = dict(
    slug="lighting",
    title="Trace::lighting",
    signature="""LightingMode Trace::lighting_mode() const noexcept

Trace& Trace::lighting(LightingMode mode) noexcept""",
    intro="""Chooses whether a 3D surface is lit, and how, and `lighting_mode()` reads the choice back. The default is `LightingMode::None`: the surface shows its data colour only. `Flat` lights each cell as a whole, `Gouraud` interpolates the light across each cell; they are compared on the [`LightingMode`](../types/LightingMode.md) page.

Lighting needs two things: a mode other than `None` on the trace, and at least one light in the pane's [`LightRig`](../types/LightRig.md). A new pane has no lights, so lighting on its own changes nothing (Example 2). It applies to the surface kinds `sheet`, `spire3` and `isoshell`, and the look of the lit surface is set by [`Trace::material`](material.md).""",
    parameters="""**mode** (`LightingMode`)
    `None`, `Flat` or `Gouraud`.""",
    returns="""`LightingMode` (getter)
    The current mode, `None` by default.

`Trace&` (setter)
    The same trace, for chaining.""",
    examples=[
        dict(id="gallery", text="The three modes under one sun-like rig. `None` ignores the lights; `Flat` and `Gouraud` light the surface and differ in how the light is carried across each cell.",
             code=SURFACE + """

const LightingMode modes[] = {LightingMode::None, LightingMode::Flat, LightingMode::Gouraud};
const char* names[] = {"None", "Flat", "Gouraud"};
auto fig = studio("lighting_gallery");
fig.mosaic(1, 3);
for (int i = 0; i < 3; ++i) {
  auto& pane = fig.pane3(names[i]);
  pane.lights().sun_key();
  pane.sheet(coord, coord, z).shade(ShadeMode::Smooth).lighting(modes[i]);
}
fig.save_svg("figure.svg");"""),
        dict(id="no_lights", text="Lighting without lights does nothing. With an empty rig a surface with `Gouraud` lighting is the same figure as one with `None`: the two SVG texts are equal. Adding a light is what makes the mode visible.",
             code=SURFACE + """

std::string svg[3];
for (int i = 0; i < 3; ++i) {
  auto fig = studio("lighting_no_lights");
  auto& pane = fig.pane3("p");
  if (i == 2) pane.lights().headlight();
  pane.sheet(coord, coord, z).lighting(i == 0 ? LightingMode::None : LightingMode::Gouraud);
  svg[i] = fig.to_svg();
}
std::cout << "empty rig, None == Gouraud: " << (svg[0] == svg[1]) << "\\n";
std::cout << "with a headlight, Gouraud == the unlit figure: " << (svg[2] == svg[0]) << "\\n";
auto fig = studio("lighting_no_lights_out");
auto& pane = fig.pane3("headlight, Gouraud");
pane.lights().headlight();
pane.sheet(coord, coord, z).lighting(LightingMode::Gouraud);
fig.save_svg("figure.svg");"""),
        dict(id="readback", text="Reading the mode back. The default is `None`; each call stores its mode and returns the trace.",
             code=SURFACE + """

auto fig = studio("lighting_readback");
auto& pane = fig.pane3("Lighting");
Trace& t = pane.sheet(coord, coord, z);
std::cout << "default: " << static_cast<int>(t.lighting_mode()) << "  (None = 0, Flat = 1, Gouraud = 2)\\n";
t.lighting(LightingMode::Flat);
std::cout << "after lighting(Flat):    " << static_cast<int>(t.lighting_mode()) << "\\n";
t.lighting(LightingMode::Gouraud).label("lit");
std::cout << "after lighting(Gouraud): " << static_cast<int>(t.lighting_mode()) << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="two_traces", text="Lighting is per trace and the rig is per pane. Two surfaces in one pane share the lights; only the one with a lighting mode is lit. The second surface is shifted so that both can be seen.",
             code=SURFACE + """

auto fig = studio("lighting_two_traces");
auto& pane = fig.pane3("one lit, one unlit");
pane.lights().sun_key();
pane.sheet(coord, coord, z).shade(ShadeMode::Smooth).lighting(LightingMode::Gouraud);
auto shifted = pluss::series::linspace(1.5, 3.5, n, pluss::ScalarType::Float64);
pane.sheet(shifted, coord, z).shade(ShadeMode::Smooth);
fig.save_svg("figure.svg");"""),
        dict(id="kinds", text="Which kinds are lit. Each is drawn under a sun-like rig with `None` and with `Gouraud`, and the SVG text compared: the surface, the pillars and the level surfaces change; the wireframe and a line do not.",
             code=SURFACE + """

std::vector<double> volume(static_cast<std::size_t>(n * n * n));
for (std::size_t i = 0; i < volume.size(); ++i) volume[i] = std::sin(0.05 * static_cast<double>(i));
auto vol = pluss::make_series(volume, {n, n, n});
auto changes = [&](auto make, bool is3d) {
  std::string svg[2];
  for (int i = 0; i < 2; ++i) {
    auto fig = studio("lighting_kinds");
    Pane& pane = is3d ? fig.pane3("p") : fig.pane2("p");
    if (is3d) pane.lights().sun_key();
    Trace& t = make(pane);
    t.lighting(i == 0 ? LightingMode::None : LightingMode::Gouraud);
    svg[i] = fig.to_svg();
  }
  return svg[0] != svg[1];
};
std::cout << "sheet:    " << changes([&](Pane& p) -> Trace& { return p.sheet(coord, coord, z); }, true) << "\\n";
std::cout << "spire3:   " << changes([&](Pane& p) -> Trace& { return p.spire3(coord, coord, z); }, true) << "\\n";
std::cout << "isoshell: " << changes([&](Pane& p) -> Trace& { return p.isoshell(coord, coord, coord, vol); }, true) << "\\n";
std::cout << "lattice:  " << changes([&](Pane& p) -> Trace& { return p.lattice(coord, coord, z); }, true) << "\\n";
auto fig = studio("lighting_kinds_out");
auto& pane = fig.pane3("spire3, lit");
pane.lights().sun_key();
pane.spire3(coord, coord, z).lighting(LightingMode::Gouraud);
fig.save_svg("figure.svg");"""),
    ],
    notes="""- `lighting` is `noexcept` and accepts every value; no light or an unlit kind simply makes it invisible, never an error.
- A pane's lights can be fixed to the camera or to the data ([`LightRig`](../types/LightRig.md)), which decides whether turning the view moves the light across the surface.""",
    see=["[`LightingMode`](../types/LightingMode.md): the three modes, compared.", "[`LightRig`](../types/LightRig.md), [`Pane::lights`](../pane/lights.md): the lights a mode needs.", "[`Trace::material`](material.md), [`Trace::shade`](shade.md): the other appearance options of a surface."],
)
