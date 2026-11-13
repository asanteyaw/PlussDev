#!/usr/bin/env python3
"""Method specs of the volumetric Pane trace-creating methods into tools/method_specs/pane/.
Run, then `python3 tools/plotdocs.py --methods pane`."""
import re, sys, pathlib
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from gen_axis_specs import write as _write
from gen_pane_plots_specs import BUMP, GRID, ex, errors, wrong_kind, trace_notes

def clean_signature(sig):
    out = []
    for line in sig.split("\n"):
        if line.startswith("template <"):
            continue
        line = re.sub(r"\b(?:Levels|Explode)&& (\w+)", r"std::vector<double> \1", line)
        line = re.sub(r"\b[A-Z]\w*&& (\w+)", r"Series \1", line)
        out.append(line)
    return "\n".join(out)


def write(slug, page):
    page['signature'] = clean_signature(page['signature'])
    _write(slug, page)


HELIX = """std::vector<double> hx, hy, hz;
for (int i = 0; i <= 40; ++i) {
  const double t = 0.25 * i;
  hx.push_back(std::cos(t));
  hy.push_back(std::sin(t));
  hz.push_back(0.1 * t);
}"""
FIELD3 = """const std::int64_t n = 12;
std::vector<double> axis(static_cast<std::size_t>(n)), values(static_cast<std::size_t>(n * n * n));
for (std::int64_t i = 0; i < n; ++i) axis[static_cast<std::size_t>(i)] = -1.0 + 2.0 * i / (n - 1);
for (std::int64_t i = 0; i < n; ++i) {
  for (std::int64_t j = 0; j < n; ++j) {
    for (std::int64_t k = 0; k < n; ++k) {
      const double x = axis[static_cast<std::size_t>(i)], y = axis[static_cast<std::size_t>(j)], z = axis[static_cast<std::size_t>(k)];
      values[static_cast<std::size_t>((i * n + j) * n + k)] = x * x + y * y + z * z;
    }
  }
}
auto field = pluss::make_series(values, {n, n, n});"""


def head3(name):
    return f'auto fig = studio("{name}");\nauto& pane = fig.pane3("p");'


def path3():
    ex_ = [
        ex("helix", "A helix: 41 points along a turning path that rises. The points are joined in the order given.",
           f"""{head3('path3_helix')}
{HELIX}
pane.path3(hx, hy, hz);
fig.save_svg("figure.svg");"""),
        ex("broadcast", "A single value is repeated: this path stays in the plane z = 0.5.",
           f"""{head3('path3_broadcast')}
pane.path3(std::vector<double>{{0, 1, 1, 0, 0}}, std::vector<double>{{0, 0, 1, 1, 0}}, std::vector<double>{{0.5}});
fig.save_svg("figure.svg");"""),
        ex("two", "Several paths on one pane, each with its own call and its own colour.",
           f"""{head3('path3_two')}
{HELIX}
pane.path3(hx, hy, hz).label("helix");
std::vector<double> sx, sy, sz;
for (std::size_t i = 0; i < hx.size(); ++i) {{ sx.push_back(0.5 * hx[i]); sy.push_back(0.5 * hy[i]); sz.push_back(hz[i]); }}
pane.path3(sx, sy, sz).label("inner");
pane.legend(KeyPlace::NorthWest);
fig.save_svg("figure.svg");"""),
        ex("view", "The view is the pane's [`lens`](lens.md). The same path seen from the side and from above.",
           f"""auto fig = studio("path3_view");
fig.mosaic(1, 2);
{HELIX}
auto& side = fig.pane3("side");
side.path3(hx, hy, hz);
side.lens().look(0.0, 5.0);
auto& above = fig.pane3("above");
above.path3(hx, hy, hz);
above.lens().look(0.0, 85.0);
fig.save_svg("figure.svg");"""),
        errors("path3", [("lengths differ", "pane.path3(std::vector<double>{1, 2, 3}, std::vector<double>{1, 2, 3}, std::vector<double>{1, 2})")], planar=False),
        wrong_kind("path3", "pane.path3(std::vector<double>{0, 1}, std::vector<double>{0, 1}, std::vector<double>{0, 1})", planar=False),
    ]
    write("path3", dict(
        slug="path3", title="Pane::path3",
        signature="template <class X, class Y, class Z>\nTrace& Pane::path3(X&& x, Y&& y, Z&& z)",
        intro="""Adds a **line in three dimensions**: the points `(x[i], y[i], z[i])` joined in the order given. It is the three-dimensional [`trend`](trend.md), for a trajectory, a curve in space or a track. The three series have the same length, or any of them length 1.

The pane must be volumetric ([`pane3`](../studio/pane3.md)); a planar pane takes [`trend`](trend.md). The view of the path is set by the pane's [`lens`](lens.md).""",
        parameters="""**x**, **y**, **z** (`Series`-convertible)
    The coordinates of the points. Equal lengths, or length 1.""",
        returns="""`Trace&`
    The new trace.""",
        examples=ex_, notes=trace_notes(), see=["[`Pane::trend`](trend.md): the line in a plane.", "[`Pane::cloud3`](cloud3.md): points in space.", "[`Pane::lens`](lens.md): the view."]))


def cloud3():
    ex_ = [
        ex("plain", "A marker at each point of the helix.",
           f"""{head3('cloud3_plain')}
{HELIX}
pane.cloud3(hx, hy, hz);
fig.save_svg("figure.svg");"""),
        ex("size", "A fourth series gives each marker its own size: here they grow along the helix.",
           f"""{head3('cloud3_size')}
{HELIX}
std::vector<double> size;
for (std::size_t i = 0; i < hx.size(); ++i) size.push_back(10.0 + 2.0 * i);
pane.cloud3(hx, hy, hz, size);
fig.save_svg("figure.svg");"""),
        ex("color", "A fifth series colours the markers through the palette, with the guide to say what the colours mean. A single size keeps the markers equal.",
           f"""{head3('cloud3_color')}
{HELIX}
pane.cloud3(hx, hy, hz, std::vector<double>{{3}}, hz);
pane.scale_guide(true);
fig.save_svg("figure.svg");"""),
        ex("scalar", "A single size or colour is repeated for every point.",
           f"""auto build = [&](double size) {{
  auto fig = studio("cloud3_scalar");
  auto& pane = fig.pane3("p");
  pane.cloud3(std::vector<double>{{0, 1, 2}}, std::vector<double>{{0, 1, 0}}, std::vector<double>{{0, 1, 2}}, std::vector<double>{{size}});
  return fig.to_svg();
}};
std::cout << "size 50 != size 150: " << (build(50.0) != build(150.0)) << "\\n";
{head3('cloud3_scalar_out')}
pane.cloud3(std::vector<double>{{0, 1, 2}}, std::vector<double>{{0, 1, 0}}, std::vector<double>{{0, 1, 2}}, std::vector<double>{{150}});
fig.save_svg("figure.svg");"""),
        errors("cloud3", [("z differs", "pane.cloud3(std::vector<double>{1, 2, 3}, std::vector<double>{1, 2, 3}, std::vector<double>{1, 2})"),
                          ("size differs", "pane.cloud3(std::vector<double>{1, 2, 3}, std::vector<double>{1, 2, 3}, std::vector<double>{1, 2, 3}, std::vector<double>{1, 2})")], planar=False),
        wrong_kind("cloud3", "pane.cloud3(std::vector<double>{0, 1}, std::vector<double>{0, 1}, std::vector<double>{0, 1})", planar=False),
    ]
    write("cloud3", dict(
        slug="cloud3", title="Pane::cloud3",
        signature="template <class X, class Y, class Z>\nTrace& Pane::cloud3(X&& x, Y&& y, Z&& z)\ntemplate <class X, class Y, class Z, class Size>\nTrace& Pane::cloud3(X&& x, Y&& y, Z&& z, Size&& size)\ntemplate <class X, class Y, class Z, class Size, class Color>\nTrace& Pane::cloud3(X&& x, Y&& y, Z&& z, Size&& size, Color&& color)",
        intro="""Adds a **cloud in three dimensions**: a marker at each point `(x[i], y[i], z[i])`, with nothing joining them. It is the three-dimensional [`cloud`](cloud.md), a scatter plot in space, and with the optional series it carries a size and a colour for each marker.

| Call | Adds |
|---|---|
| `cloud3(x, y, z)` | Markers of one size and colour. |
| `cloud3(x, y, z, size)` | A size for each marker. |
| `cloud3(x, y, z, size, color)` | And a colour for each, from the palette over the range of `color`. |

As for `cloud`, there is no call with a colour and no size. All series have the same length, or length 1.""",
        parameters="""**x**, **y**, **z** (`Series`-convertible)
    The positions.

**size**, **color** (`Series`-convertible, optional)
    Size and colour value of each marker, or one for all.""",
        returns="""`Trace&`
    The new trace.""",
        examples=ex_, notes=trace_notes(), see=["[`Pane::cloud`](cloud.md): the planar form.", "[`Pane::path3`](path3.md): a line through points.", "[`Pane::scale_guide`](scale_guide.md): the key to the colours."]))


def spire3():
    sp = "auto z = pluss::series::linspace(0.0, 4.0, 9, pluss::ScalarType::Float64).reshape({3, 3});"
    ex_ = [
        ex("z", "Only the heights: one bar for each cell of a 3 by 3 matrix, standing at the positions 1, 2, 3 along x (the columns) and 1, 2, 3 along y (the rows).",
           f"""{head3('spire3_z')}
{sp}
pane.spire3(z);
fig.save_svg("figure.svg");"""),
        ex("yz", "With `y` given, the rows stand at those positions; the columns keep the positions 1, 2, 3.",
           f"""{head3('spire3_yz')}
{sp}
pane.spire3(std::vector<double>{{0, 5, 10}}, z);
fig.save_svg("figure.svg");"""),
        ex("xyz", "Both coordinates given: bars on a grid of your own spacing.",
           f"""{head3('spire3_xyz')}
{sp}
pane.spire3(std::vector<double>{{0, 1, 4}}, std::vector<double>{{0, 2, 3}}, z);
fig.save_svg("figure.svg");"""),
        ex("color", "A fourth argument colours the bars by another quantity, a matrix of the same shape as `z`. Here the bars are coloured by a matrix running the other way from their heights.",
           f"""{head3('spire3_color')}
{sp}
auto c = pluss::series::linspace(1.0, 0.0, 9, pluss::ScalarType::Float64).reshape({{3, 3}});
pane.spire3(std::vector<double>{{0, 1, 2}}, std::vector<double>{{0, 1, 2}}, z, c);
pane.scale_guide(true);
fig.save_svg("figure.svg");"""),
        ex("negative", "Bars go down from zero for negative values.",
           f"""{head3('spire3_negative')}
auto z = pluss::make_series(std::vector<double>{{2, -1, 3, -2, 1, 2, 0, 3, -3}}, {{3, 3}});
pane.spire3(z);
fig.save_svg("figure.svg");"""),
        errors("spire3", [("z is not a matrix", "pane.spire3(std::vector<double>{1, 2, 3})"),
                          ("coordinates do not match z", "auto z = pluss::series::linspace(0.0, 4.0, 9, pluss::ScalarType::Float64).reshape({3, 3}); pane.spire3(std::vector<double>{1, 2}, std::vector<double>{1, 2, 3}, z)")], planar=False),
        wrong_kind("spire3", "pane.spire3(pluss::series::linspace(0.0, 1.0, 4, pluss::ScalarType::Float64).reshape({2, 2}))", planar=False),
    ]
    write("spire3", dict(
        slug="spire3", title="Pane::spire3",
        signature="template <class Z>\nTrace& Pane::spire3(Z&& z)\ntemplate <class Y, class Z>\nTrace& Pane::spire3(Y&& y, Z&& z)\ntemplate <class X, class Y, class Z>\nTrace& Pane::spire3(X&& x, Y&& y, Z&& z)\ntemplate <class X, class Y, class Z, class C>\nTrace& Pane::spire3(X&& x, Y&& y, Z&& z, C&& c)",
        intro="""Adds a **bar chart in three dimensions**: a vertical bar for each cell of a matrix `z`, standing on the grid of the coordinates with the height of the cell. It is the trace for a table of values over two categories, a histogram of two variables, or a surface shown as bars.

`z` is a rank-2 matrix: its columns run along x and its rows along y. The calls differ in how much of the grid you give:

| Call | Coordinates |
|---|---|
| `spire3(z)` | x and y are the indices 1, 2, 3, ... |
| `spire3(y, z)` | y given, x the indices. |
| `spire3(x, y, z)` | Both given. |
| `spire3(x, y, z, c)` | Both given, and the bars are coloured by `c`, a matrix of the shape of `z`, instead of by their height. |

`x` has as many entries as `z` has columns and `y` as many as it has rows, or both are full matrices of the shape of `z`. `z` must be rank 2 in every call: a rank-1 series throws.""",
        parameters="""**x**, **y** (`Series`-convertible, optional)
    The coordinates of the columns and the rows.

**z** (`Series`-convertible, rank 2)
    The heights.

**c** (`Series`-convertible, optional)
    The values that colour the bars.""",
        returns="""`Trace&`
    The new trace.""",
        examples=ex_, notes=trace_notes(), see=["[`Pane::sheet`](sheet.md): the same data as a surface.", "[`Pane::lattice`](lattice.md): as a wire grid.", "[`Pane::columns`](columns.md): bars in the plane."]))


def surface(slug):
    sheet = slug == "sheet"
    ex_ = [
        ex("bump", f"A field `z(x, y)` over a grid of 25 by 25 points, drawn as {'a surface' if sheet else 'a wire grid'}. `x` and `y` give the coordinates of the columns and rows.",
           f"""{head3(slug + '_bump')}
{BUMP}
pane.{slug}(axis, axis, z);
fig.save_svg("figure.svg");"""),
        ex("color", f"A fourth argument colours {'the surface' if sheet else 'the lines'} by another quantity: here the field is drawn as it is and coloured by a matrix that runs the other way, so high ground is shown in low colours. The colours of the two figures differ.",
           f"""auto build = [&](bool with_color) {{
  auto fig = studio("{slug}_color");
  auto& pane = fig.pane3("p");
  {BUMP}
  auto c = pluss::series::linspace(1.0, 0.0, n * n, pluss::ScalarType::Float64).reshape({{n, n}});
  if (with_color) pane.{slug}(axis, axis, z, c);
  else pane.{slug}(axis, axis, z);
  return fig.to_svg();
}};
std::cout << "colour series changes the figure: " << (build(true) != build(false)) << "\\n";
{head3(slug + '_color_out')}
{BUMP}
auto c = pluss::series::linspace(1.0, 0.0, n * n, pluss::ScalarType::Float64).reshape({{n, n}});
pane.{slug}(axis, axis, z, c);
pane.scale_guide(true);
fig.save_svg("figure.svg");"""),
        ex("coarse", f"A small grid shows the structure: a 3 by 3 field is four cells {'of a mesh' if sheet else 'of wire'}.",
           f"""{head3(slug + '_coarse')}
{GRID}
pane.{slug}(std::vector<double>{{0, 1, 2}}, std::vector<double>{{0, 1, 2}}, z);
fig.save_svg("figure.svg");"""),
        ex("matrices", "Coordinates can be full matrices, one value for each grid point, for grids that are not rectangular. Here the grid is sheared: each row is shifted along x.",
           f"""{head3(slug + '_matrices')}
{GRID}
auto x = pluss::make_series(std::vector<double>{{0, 1, 2, 0.5, 1.5, 2.5, 1, 2, 3}}, {{3, 3}});
auto y = pluss::make_series(std::vector<double>{{0, 0, 0, 1, 1, 1, 2, 2, 2}}, {{3, 3}});
pane.{slug}(x, y, z);
fig.save_svg("figure.svg");"""),
        ex("view", "Seen from other sides: the lens turns the picture and nothing else.",
           f"""auto fig = studio("{slug}_view");
fig.mosaic(1, 3);
{BUMP}
const double views[3][2] = {{{{-37.5, 30.0}}, {{-90.0, 10.0}}, {{0.0, 80.0}}}};
for (int i = 0; i < 3; ++i) {{
  auto& pane = fig.pane3("az " + std::to_string(static_cast<int>(views[i][0])) + ", el " + std::to_string(static_cast<int>(views[i][1])));
  pane.{slug}(axis, axis, z);
  pane.lens().look(views[i][0], views[i][1]);
}}
fig.save_svg("figure.svg");"""),
        errors(slug, [("coordinates do not match z", f"auto z = pluss::series::linspace(0.0, 1.0, 9, pluss::ScalarType::Float64).reshape({{3, 3}}); pane.{slug}(std::vector<double>{{1, 2}}, std::vector<double>{{1, 2, 3}}, z)"),
                      ("colour has another shape", f"auto z = pluss::series::linspace(0.0, 1.0, 9, pluss::ScalarType::Float64).reshape({{3, 3}}); auto c = pluss::series::linspace(0.0, 1.0, 4, pluss::ScalarType::Float64).reshape({{2, 2}}); pane.{slug}(std::vector<double>{{1, 2, 3}}, std::vector<double>{{1, 2, 3}}, z, c)")], planar=False),
        wrong_kind(slug, f"pane.{slug}(std::vector<double>{{0, 1}}, std::vector<double>{{0, 1}}, pluss::series::linspace(0.0, 1.0, 4, pluss::ScalarType::Float64).reshape({{2, 2}}))", planar=False),
    ]
    if sheet:
        ex_.insert(3, ex("shade", "How the surface is shaded is a property of the trace: `ShadeMode::Smooth` blends the colour across each cell, and a [light](lights.md) with `lighting` gives it relief.",
                         f"""{head3('sheet_shade')}
{BUMP}
pane.sheet(axis, axis, z).shade(ShadeMode::Smooth).lighting(LightingMode::Flat);
pane.lights().lamp_right();
fig.save_svg("figure.svg");"""))
    else:
        ex_.insert(2, ex("color_reversed", "The colour matrix really does colour the wires: reversing it reverses the colours of the lines, so the two figures differ. (A colour matrix that is a linear rescaling of `z` gives the same colours as none, because colours depend on where a value sits in its own range.)",
                         f"""auto build = [&](double from, double to) {{
  auto fig = studio("lattice_reversed");
  auto& pane = fig.pane3("p");
  {GRID}
  auto c = pluss::series::linspace(from, to, 9, pluss::ScalarType::Float64).reshape({{3, 3}});
  pane.lattice(std::vector<double>{{0, 1, 2}}, std::vector<double>{{0, 1, 2}}, z, c);
  return fig.to_svg();
}};
std::cout << "rescaled colours == same colours: " << (build(0.0, 5000.0) == build(0.0, 1.0)) << "\\n";
std::cout << "reversed colours differ: " << (build(5000.0, 0.0) != build(0.0, 1.0)) << "\\n";
{head3('lattice_reversed_out')}
{GRID}
pane.lattice(std::vector<double>{{0, 1, 2}}, std::vector<double>{{0, 1, 2}}, z, pluss::series::linspace(1.0, 0.0, 9, pluss::ScalarType::Float64).reshape({{3, 3}}));
fig.save_svg("figure.svg");"""))
    write(slug, dict(
        slug=slug, title=f"Pane::{slug}",
        signature=f"template <class X, class Y, class Z>\nTrace& Pane::{slug}(X&& x, Y&& y, Z&& z)\ntemplate <class X, class Y, class Z, class C>\nTrace& Pane::{slug}(X&& x, Y&& y, Z&& z, C&& c)",
        intro=f"""Adds {'a **surface**: a continuous sheet through the points' if sheet else 'a **wire grid**: the lines of the grid'} `(x[j], y[i], z(i, j))` of a field `z(x, y)`{', each cell filled and shaded' if sheet else ', drawn through the points with nothing filled between them, so what lies behind stays visible'}. It shows a quantity that depends on two others as {'a surface' if sheet else 'a mesh'}.

`z` is a rank-2 matrix with a row for each `y` and a column for each `x`; `x` and `y` give the coordinates of the columns and rows, or are full matrices with a value for each point (for a grid that is not rectangular).

| Call | Colours from |
|---|---|
| `{slug}(x, y, z)` | The height `z`, through the palette. |
| `{slug}(x, y, z, c)` | `c`, a matrix of the shape of `z`. |

The range of the colours is the data's, or the pane's [`color_limits`](color_limits.md); [`scale_guide`](scale_guide.md) shows it. {'Shading and lighting are set on the trace ([`shade`](../trace/shade.md), [`lighting`](../trace/lighting.md), [`material`](../trace/material.md)).' if sheet else 'The same data as a filled surface is [`sheet`](sheet.md).'}""",
        parameters="""**x**, **y** (`Series`-convertible)
    The coordinates of the columns and the rows, or full matrices.

**z** (`Series`-convertible, rank 2)
    The field.

**c** (`Series`-convertible, optional)
    The values that colour the surface, with as many elements as `z`.""",
        returns="""`Trace&`
    The new trace.""",
        examples=ex_, notes=trace_notes(), see=[f"[`Pane::{'lattice' if sheet else 'sheet'}`]({'lattice' if sheet else 'sheet'}.md): the {'wire grid' if sheet else 'filled surface'}.", "[`Pane::spire3`](spire3.md): bars on the grid.", "[`Pane::ridge3`](ridge3.md): contour lines on the surface.", "[`Pane::lens`](lens.md): the view."]))


def ridge3():
    ex_ = [
        ex("default", "Contour lines lifted to their own height: each curve is drawn at the value of its level, so the picture is a set of rings stacked in the air over the grid.",
           f"""{head3('ridge3_default')}
{BUMP}
pane.ridge3(axis, axis, z);
fig.save_svg("figure.svg");"""),
        ex("count", "An integer is a number of levels: three give a coarse picture.",
           f"""{head3('ridge3_count')}
{BUMP}
pane.ridge3(axis, axis, z, 3);
fig.save_svg("figure.svg");"""),
        ex("levels", "A container is a list of levels: here the lines at 0.2, 0.6 and 1.0.",
           f"""{head3('ridge3_levels')}
{BUMP}
pane.ridge3(axis, axis, z, std::vector<double>{{0.2, 0.6, 1.0}});
fig.save_svg("figure.svg");"""),
        ex("with_sheet", "On the surface they came from: a [`sheet`](sheet.md) and its contour lines on one pane. The lines follow the surface because each is drawn at its own height.",
           f"""{head3('ridge3_with_sheet')}
{BUMP}
pane.sheet(axis, axis, z).transparent(true);
pane.ridge3(axis, axis, z, 6);
fig.save_svg("figure.svg");"""),
        ex("annotate", "The value of each level can be written on its line with [`Trace::annotate_levels`](../trace/annotate_levels.md).",
           f"""{head3('ridge3_annotate')}
{BUMP}
pane.ridge3(axis, axis, z, 4).annotate_levels(true);
fig.save_svg("figure.svg");"""),
        errors("ridge3", [("no levels", "auto z = pluss::series::linspace(0.0, 1.0, 9, pluss::ScalarType::Float64).reshape({3, 3}); pane.ridge3(std::vector<double>{1, 2, 3}, std::vector<double>{1, 2, 3}, z, 0)"),
                          ("z is not a matrix", "pane.ridge3(std::vector<double>{1, 2, 3}, std::vector<double>{1, 2, 3}, std::vector<double>{1, 2, 3})")], planar=False),
        wrong_kind("ridge3", "pane.ridge3(std::vector<double>{0, 1}, std::vector<double>{0, 1}, pluss::series::linspace(0.0, 1.0, 4, pluss::ScalarType::Float64).reshape({2, 2}))", planar=False),
    ]
    write("ridge3", dict(
        slug="ridge3", title="Pane::ridge3",
        signature="template <class X, class Y, class Z>\nTrace& Pane::ridge3(X&& x, Y&& y, Z&& z)\ntemplate <class X, class Y, class Z, class Levels>\nTrace& Pane::ridge3(X&& x, Y&& y, Z&& z, Levels&& levels)\ntemplate <class X, class Y, class Z>\nTrace& Pane::ridge3(X&& x, Y&& y, Z&& z, int level_count)",
        intro="""Adds **contour lines in three dimensions**: the curves along which a field `z(x, y)` has a given value, drawn at that value's height instead of flattened onto the plane. It is [`contour`](contour.md) on a volumetric pane, and shows the shape of the field and its levels in one picture.

`z` is a rank-2 matrix, as for [`sheet`](sheet.md). The levels are chosen as for `contour`:

| Call | Levels |
|---|---|
| `ridge3(x, y, z)` | 8, evenly spaced between the lowest and highest value. |
| `ridge3(x, y, z, level_count)` | `level_count` (an integer, positive), evenly spaced. |
| `ridge3(x, y, z, levels)` | Exactly the values of the container `levels`. |

An integer is always a count; one level at the value 4 is `std::vector<double>{4.0}`. The colours come from the palette over the range of the field, or one colour for a single level.""",
        parameters="""**x**, **y** (`Series`-convertible)
    The coordinates of the columns and rows.

**z** (`Series`-convertible, rank 2)
    The field.

**level_count** (`int`) or **levels** (container)
    How many levels, or which.""",
        returns="""`Trace&`
    The new trace.""",
        examples=ex_, notes=trace_notes(), see=["[`Pane::contour`](contour.md): the lines on a plane.", "[`Pane::sheet`](sheet.md): the surface.", "[`Trace::annotate_levels`](../trace/annotate_levels.md): write the levels."]))


def isoshell():
    ex_ = [
        ex("default", "A field with a value at each point of a 12 by 12 by 12 grid, here the squared distance from the centre. By default four levels are chosen, and a nested shell is drawn for each: shells of equal distance around the centre.",
           f"""{head3('isoshell_default')}
{FIELD3}
pane.isoshell(field);
fig.save_svg("figure.svg");"""),
        ex("count", "A count of levels: two shells show the inside of the nest.",
           f"""{head3('isoshell_count')}
{FIELD3}
pane.isoshell(field, 2);
fig.save_svg("figure.svg");"""),
        ex("coordinates", "With coordinates of your own the field is placed in space: the axes run from -1 to 1 here, so the shells are placed around the origin and the axes are labelled in those units.",
           f"""{head3('isoshell_coordinates')}
{FIELD3}
pane.isoshell(axis, axis, axis, field);
fig.save_svg("figure.svg");"""),
        ex("coordinates_count", "Coordinates and a count of levels.",
           f"""{head3('isoshell_coordinates_count')}
{FIELD3}
pane.isoshell(axis, axis, axis, field, 3);
fig.save_svg("figure.svg");"""),
        ex("levels", "Explicit levels, in a container: the shells at the squared distances 0.2 and 0.8.",
           f"""{head3('isoshell_levels')}
{FIELD3}
pane.isoshell(axis, axis, axis, field, std::vector<double>{{0.2, 0.8}});
fig.save_svg("figure.svg");"""),
        ex("transparent", "The shells hide one another. A transparent trace shows the inner ones through the outer.",
           f"""{head3('isoshell_transparent')}
{FIELD3}
pane.isoshell(field, 3).transparent(true);
fig.save_svg("figure.svg");"""),
        errors("isoshell", [("values are not a field", "pane.isoshell(pluss::series::linspace(0.0, 1.0, 9, pluss::ScalarType::Float64).reshape({3, 3}))"),
                            ("no levels", "pane.isoshell(pluss::series::linspace(0.0, 1.0, 27, pluss::ScalarType::Float64).reshape({3, 3, 3}), 0)")], planar=False),
        wrong_kind("isoshell", "pane.isoshell(pluss::series::linspace(0.0, 1.0, 8, pluss::ScalarType::Float64).reshape({2, 2, 2}))", planar=False),
    ]
    write("isoshell", dict(
        slug="isoshell", title="Pane::isoshell",
        signature="template <class Values>\nTrace& Pane::isoshell(Values&& values, int level_count = 4)\ntemplate <class X, class Y, class Z, class Values>\nTrace& Pane::isoshell(X&& x, Y&& y, Z&& z, Values&& values, int level_count = 4)\ntemplate <class X, class Y, class Z, class Values, class Levels>\nTrace& Pane::isoshell(X&& x, Y&& y, Z&& z, Values&& values, Levels&& levels)",
        intro="""Adds **isosurfaces**: for a field with a value at every point of a three-dimensional grid, the surfaces on which the field takes given values, drawn as nested shells. It is [`contour`](contour.md) one dimension up, and shows the shape of a volume of data, such as a density, a temperature or an electric potential.

`values` is a rank-3 field, indexed along x, y and z.

| Call | Coordinates | Levels |
|---|---|---|
| `isoshell(values, level_count = 4)` | The indices 1, 2, 3, ... | `level_count` evenly spaced. |
| `isoshell(x, y, z, values, level_count = 4)` | Given: one series for each axis, with the extent of `values` along it. | `level_count` evenly spaced. |
| `isoshell(x, y, z, values, levels)` | Given. | Exactly the container `levels`. |

The levels are spread between the lowest and highest value of the field, and the count must be positive. A shell is coloured by its level. Inner shells are hidden by outer ones unless the trace is [transparent](../trace/transparent.md).""",
        parameters="""**values** (`Series`-convertible, rank 3)
    The field.

**x**, **y**, **z** (`Series`-convertible, optional)
    The coordinates along each axis.

**level_count** (`int`, default 4) or **levels** (container)
    How many shells, or at which values.""",
        returns="""`Trace&`
    The new trace.""",
        examples=ex_, notes=trace_notes(), see=["[`Pane::contour`](contour.md): contour lines of a two-dimensional field.", "[`Pane::sheet`](sheet.md): a surface over a grid.", "[`Trace::transparent`](../trace/transparent.md): see through the shells."]))


def main():
    path3(); cloud3(); spire3(); surface("sheet"); surface("lattice"); ridge3(); isoshell()
    print("wrote volumetric plot specs")


if __name__ == "__main__":
    main()
