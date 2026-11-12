import sys, pathlib
sys.path.insert(0, str(pathlib.Path(__file__).parent))
from _common import *
PAGE = dict(
    slug="LightingMode",
    summary="Whether and how a surface is lit.",
    title="LightingMode",
    signature="enum class LightingMode : std::uint8_t { None, Flat, Gouraud };",
    intro="""`LightingMode` decides whether, and how, a 3D surface trace is lit. It is the argument of [`Trace::lighting`](../trace/lighting.md) and
the value `Trace::lighting_mode` reads back. The lights themselves belong to the pane ([`LightRig`](LightRig.md)): a lighting mode
with no lights in the rig has nothing to compute, so it needs both. The surface's response to the light is the separate
[`MaterialProfile`](MaterialProfile.md).

The two lit modes differ in where the light is evaluated. `Flat` evaluates it once per cell and gives the whole cell one brightness, so
the surface keeps a visibly faceted look even when the colouring is smooth. `Gouraud` evaluates it at the corners of each cell and
interpolates, so the brightness varies continuously across the surface.""",
    table="""| Value | Lighting | Look |
|---|---|---|
| `None` | No lighting: the surface shows its data colour only. This is the default. | Flat colour; shape is read from the colour and the outlines. |
| `Flat` | One brightness per cell. | Lit, with each cell a separate tone. |
| `Gouraud` | Brightness interpolated across each cell. | Lit and smooth. |""",
    examples=[
        dict(id="gallery", text="The three modes on the same surface with the same sun-like rig. `None` ignores the rig; `Flat` shades each cell by its own angle to the light; `Gouraud` blends the shading across the surface.",
             code=SURFACE + """

const LightingMode modes[] = {LightingMode::None, LightingMode::Flat, LightingMode::Gouraud};
const char* names[] = {"None", "Flat", "Gouraud"};
auto fig = studio("lighting_mode_gallery");
fig.mosaic(1, 3);
for (int i = 0; i < 3; ++i) {
  auto& pane = fig.pane3(names[i]);
  pane.lights().sun_key();
  pane.sheet(coord, coord, z).shade(ShadeMode::Smooth).lighting(modes[i]);
}
fig.save_svg("figure.svg");"""),
        dict(id="no_lights", text="A lighting mode needs lights. With an empty rig `Gouraud` has nothing to compute, and the figure is the same as with `None`: the SVG text of the two is equal.",
             code=SURFACE + """

std::string svg[2];
const LightingMode modes[] = {LightingMode::None, LightingMode::Gouraud};
for (int i = 0; i < 2; ++i) {
  auto fig = studio("lighting_mode_no_lights");
  auto& pane = fig.pane3("No lights");
  std::cout << "lights in the rig: " << pane.lights().lights().size() << "\\n";
  pane.sheet(coord, coord, z).lighting(modes[i]);
  svg[i] = fig.to_svg();
}
std::cout << "None == Gouraud with an empty rig: " << (svg[0] == svg[1]) << "\\n";"""),
        dict(id="rig", text="The same mode with different rigs. `Gouraud` under a single `headlight` (a light at the camera) and under `sun_key` (a stronger, angled key light) give different pictures, because the mode reads whatever the rig contains.",
             code=SURFACE + """

auto fig = studio("lighting_mode_rig");
fig.mosaic(1, 2);
auto& head = fig.pane3("headlight");
head.lights().headlight();
head.sheet(coord, coord, z).shade(ShadeMode::Smooth).lighting(LightingMode::Gouraud);
auto& sun = fig.pane3("sun_key");
sun.lights().sun_key();
sun.sheet(coord, coord, z).shade(ShadeMode::Smooth).lighting(LightingMode::Gouraud);
fig.save_svg("figure.svg");"""),
        dict(id="readback", text="`lighting` returns the trace; `lighting_mode` reads it back. A new trace's default is `None`.",
             code=SURFACE + """

auto fig = studio("lighting_mode_readback");
auto& pane = fig.pane3("Readback");
Trace& trace = pane.sheet(coord, coord, z);
std::cout << "default: " << static_cast<int>(trace.lighting_mode()) << "\\n";
for (LightingMode m : {LightingMode::None, LightingMode::Flat, LightingMode::Gouraud}) {
  trace.lighting(m);
  std::cout << static_cast<int>(m) << " -> " << static_cast<int>(trace.lighting_mode()) << "\\n";
}
fig.save_svg("figure.svg");"""),
    ],
    notes="""- Lighting is per trace and the rig is per pane: two traces in one pane share the lights but can have different modes and materials.
- `Gouraud` costs more to evaluate than `Flat` on very large grids, but is the usual choice for a smooth surface.""",
    see=["[`Trace::lighting`](../trace/lighting.md): sets the mode.", "[`LightRig`](LightRig.md): the lights a mode uses.", "[`MaterialProfile`](MaterialProfile.md), [`ShadeMode`](ShadeMode.md): the other two appearance options of a surface."],
)
