import sys, pathlib
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[2] / "specs"))
from _common import SURFACE
PAGE = dict(
    slug="material",
    title="Trace::material",
    signature="""MaterialProfile Trace::material_profile() const noexcept

Trace& Trace::material(MaterialProfile profile) noexcept""",
    intro="""Chooses how a lit 3D surface responds to light, and `material_profile()` reads the choice back. The four profiles, `Default`, `Dull`, `Shiny` and `Metal`, are described and compared on the [`MaterialProfile`](../types/MaterialProfile.md) page; the point to know here is that `Default` is exactly `Shiny`, so a surface that never calls `material` already has a glossy highlight, and `Dull` is how to remove it.

A material only shows on a lit surface: the trace needs a [lighting mode](lighting.md) other than `None` and the pane needs lights. Without them every profile draws the same figure. It applies to the surface kinds `sheet`, `spire3` and `isoshell`.""",
    parameters="""**profile** (`MaterialProfile`)
    `Default`, `Dull`, `Shiny` or `Metal`.""",
    returns="""`MaterialProfile` (getter)
    The current profile, `Default` by default.

`Trace&` (setter)
    The same trace, for chaining.""",
    examples=[
        dict(id="gallery", text="The four profiles under one sun-like rig with Gouraud lighting. `Default` and `Shiny` are the same; `Dull` has no highlight and `Metal` a tight one on a darker surface.",
             code=SURFACE + """

const MaterialProfile profiles[] = {MaterialProfile::Default, MaterialProfile::Dull, MaterialProfile::Shiny, MaterialProfile::Metal};
const char* names[] = {"Default", "Dull", "Shiny", "Metal"};
auto fig = studio("material_gallery");
fig.mosaic(1, 4);
for (int i = 0; i < 4; ++i) {
  auto& pane = fig.pane3(names[i]);
  pane.lights().sun_key();
  pane.sheet(coord, coord, z).shade(ShadeMode::Smooth).lighting(LightingMode::Gouraud).material(profiles[i]);
}
fig.save_svg("figure.svg");"""),
        dict(id="unlit", text="A material needs light. The four profiles on a surface with no lighting are the same figure: all three comparisons with the first are true.",
             code=SURFACE + """

std::string svg[4];
const MaterialProfile profiles[] = {MaterialProfile::Default, MaterialProfile::Dull, MaterialProfile::Shiny, MaterialProfile::Metal};
for (int i = 0; i < 4; ++i) {
  auto fig = studio("material_unlit");
  fig.pane3("Unlit").sheet(coord, coord, z).material(profiles[i]);
  svg[i] = fig.to_svg();
}
std::cout << "Default == Dull:  " << (svg[0] == svg[1]) << "\\n";
std::cout << "Default == Shiny: " << (svg[0] == svg[2]) << "\\n";
std::cout << "Default == Metal: " << (svg[0] == svg[3]) << "\\n";
auto fig = studio("material_unlit_out");
fig.pane3("Unlit, Metal").sheet(coord, coord, z).material(MaterialProfile::Metal);
fig.save_svg("figure.svg");"""),
        dict(id="default_is_shiny", text="`Default` is `Shiny`. Under light the two give the same figure (the first comparison is true), while `Dull` and `Metal` differ from `Default` (false). To make a surface matt, ask for `Dull`.",
             code=SURFACE + """

std::string svg[4];
const MaterialProfile profiles[] = {MaterialProfile::Default, MaterialProfile::Shiny, MaterialProfile::Dull, MaterialProfile::Metal};
for (int i = 0; i < 4; ++i) {
  auto fig = studio("material_default");
  auto& pane = fig.pane3("p");
  pane.lights().sun_key();
  pane.sheet(coord, coord, z).shade(ShadeMode::Smooth).lighting(LightingMode::Gouraud).material(profiles[i]);
  svg[i] = fig.to_svg();
}
std::cout << "Default == Shiny: " << (svg[0] == svg[1]) << "\\n";
std::cout << "Default == Dull:  " << (svg[0] == svg[2]) << "\\n";
std::cout << "Default == Metal: " << (svg[0] == svg[3]) << "\\n";
auto fig = studio("material_default_out");
auto& pane = fig.pane3("Dull");
pane.lights().sun_key();
pane.sheet(coord, coord, z).shade(ShadeMode::Smooth).lighting(LightingMode::Gouraud).material(MaterialProfile::Dull);
fig.save_svg("figure.svg");"""),
        dict(id="readback", text="Reading the profile back. The default reads `Default`; each call stores its profile and returns the trace.",
             code=SURFACE + """

auto fig = studio("material_readback");
auto& pane = fig.pane3("Material");
Trace& t = pane.sheet(coord, coord, z);
std::cout << "default: " << static_cast<int>(t.material_profile()) << "  (Default = 0, Dull = 1, Shiny = 2, Metal = 3)\\n";
for (MaterialProfile m : {MaterialProfile::Dull, MaterialProfile::Shiny, MaterialProfile::Metal}) {
  t.material(m);
  std::cout << static_cast<int>(m) << " -> " << static_cast<int>(t.material_profile()) << "\\n";
}
fig.save_svg("figure.svg");"""),
        dict(id="rigs", text="The same material under two rigs. A `Metal` surface lit by a headlight (a light at the viewer) shows little of its character, and the same surface under the angled `sun_key` shows the highlight: the choice of material and the choice of lights are made together.",
             code=SURFACE + """

auto fig = studio("material_rigs");
fig.mosaic(1, 2);
auto& head = fig.pane3("Metal, headlight");
head.lights().headlight();
head.sheet(coord, coord, z).shade(ShadeMode::Smooth).lighting(LightingMode::Gouraud).material(MaterialProfile::Metal);
auto& sun = fig.pane3("Metal, sun_key");
sun.lights().sun_key();
sun.sheet(coord, coord, z).shade(ShadeMode::Smooth).lighting(LightingMode::Gouraud).material(MaterialProfile::Metal);
fig.save_svg("figure.svg");"""),
    ],
    notes="""- `material` is `noexcept` and accepts every value.
- Material and lighting are per trace; the lights are per pane ([`LightRig`](../types/LightRig.md)).""",
    see=["[`MaterialProfile`](../types/MaterialProfile.md): the four profiles, compared.", "[`Trace::lighting`](lighting.md), [`Trace::shade`](shade.md): the other appearance options.", "[`LightRig`](../types/LightRig.md): the lights a material responds to."],
)
