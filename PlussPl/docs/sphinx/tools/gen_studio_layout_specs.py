#!/usr/bin/env python3
"""Specs for Studio::mosaic, tight_layout and context_scales into tools/method_specs/studio/.
Run, then `python3 tools/plotdocs.py --methods studio`."""
import sys, pathlib
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import gen_axis_specs as g
from gen_pane_settings_specs import AT, SIZE

OUT = pathlib.Path(__file__).resolve().parent / "method_specs" / "studio"


def write(slug, page):
    OUT.joinpath(f"{slug}.py").write_text("PAGE = " + g.repr_page(page) + "\n")


def ex(id, text, code):
    return dict(id=id, text=text, code=code)


LINE = "std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0}"


def two_titles(name, setup="", n=2, rows=1, cols=2):
    return f"""auto fig = studio("{name}");
fig.mosaic({rows}, {cols});
{setup}
for (int i = 0; i < {n}; ++i) fig.pane2("pane " + std::to_string(i)).trend({LINE});"""


def mosaic():
    ex_ = [
        ex("single", "Without a `mosaic()` call a figure has one grid cell, and a single pane fills the canvas. `mosaic_layout().active` is false.",
           f"""auto fig = studio("mosaic_single");
fig.pane2("p").trend({LINE});
std::cout << "active: " << fig.mosaic_layout().active << "\\n";
fig.save_svg("figure.svg");"""),
        ex("autoflow", "A 2x2 grid filled by four `pane2` calls. Each pane takes the next free cell, row by row, in the order it was added: the titles' positions show the order.",
           f"""auto fig = studio("mosaic_2x2");
fig.mosaic(2, 2);
for (int i = 0; i < 4; ++i) fig.pane2("cell " + std::to_string(i)).trend({LINE});
const std::string svg = fig.to_svg();
{AT}
for (int i = 0; i < 4; ++i) std::cout << "cell " << i << " title at " << at(svg, "cell " + std::to_string(i)) << "\\n";
std::cout << fig.mosaic_layout().rows << "x" << fig.mosaic_layout().cols << ", active " << fig.mosaic_layout().active << "\\n";
fig.save_svg("figure.svg");"""),
        ex("auto_gutter", "The gutter between cells. By default (`auto_gutter`) it is a share of a cell, so it grows and shrinks with the figure: in a figure twice as wide the gap is twice as wide.",
           f"""auto build = [&](double width) {{
  auto fig = studio("mosaic_auto");
  fig.size(width, 600.0);
  fig.mosaic(1, 2);
  fig.pane2("left").trend({LINE});
  fig.pane2("right").trend({LINE});
  return fig.to_svg();
}};
{AT}
for (double width : {{800.0, 1600.0}}) {{
  const std::string svg = build(width);
  std::cout << "width " << width << ": left title " << at(svg, "left") << ", right title " << at(svg, "right") << "\\n";
}}
auto fig = studio("mosaic_auto_out");
fig.mosaic(1, 2);
fig.pane2("left").trend({LINE});
fig.pane2("right").trend({LINE});
fig.save_svg("figure.svg");"""),
        ex("gutter", "A number is a distance in drawing units. A gutter of 0 puts the cells side by side, and 120 leaves a wide gap: the right-hand pane moves with it.",
           f"""auto build = [&](double gutter) {{
  auto fig = studio("mosaic_gutter");
  fig.mosaic(1, 2, gutter, gutter);
  fig.pane2("left").trend({LINE});
  fig.pane2("right").trend({LINE});
  return fig.to_svg();
}};
{AT}
for (double gutter : {{0.0, 40.0, 120.0}}) std::cout << "gutter " << gutter << ": right title " << at(build(gutter), "right") << "\\n";
auto fig = studio("mosaic_gutter_out");
fig.mosaic(1, 2, 120.0, 120.0);
fig.pane2("left").trend({LINE});
fig.pane2("right").trend({LINE});
fig.save_svg("figure.svg");"""),
        ex("axes_apart", "The two gutters are independent: here the columns are close and the rows far apart.",
           f"""auto fig = studio("mosaic_gutters_xy");
fig.mosaic(2, 2, 8.0, 80.0);
for (int i = 0; i < 4; ++i) fig.pane2("cell " + std::to_string(i)).trend({LINE});
fig.save_svg("figure.svg");"""),
        ex("claimed", "Panes placed by [`Pane::place`](../pane/place.md) take their cells first, and the other panes fill the cells that are left, still row by row. The first pane added is placed at the bottom right; the next three take the three cells that are free.",
           f"""auto fig = studio("mosaic_claimed");
fig.mosaic(2, 2);
auto& corner = fig.pane2("placed bottom right");
corner.trend({LINE});
corner.place(1, 1);
for (int i = 0; i < 3; ++i) fig.pane2("auto " + std::to_string(i)).trend({LINE});
const std::string svg = fig.to_svg();
{AT}
std::cout << "placed: " << at(svg, "placed bottom right") << "\\n";
for (int i = 0; i < 3; ++i) std::cout << "auto " << i << ": " << at(svg, "auto " + std::to_string(i)) << "\\n";
fig.save_svg("figure.svg");"""),
        ex("replace", "Calling `mosaic` again replaces the grid: the later call decides how the panes are laid out.",
           f"""auto fig = studio("mosaic_replace");
fig.mosaic(2, 2).mosaic(1, 4);
for (int i = 0; i < 4; ++i) fig.pane2("cell " + std::to_string(i)).trend({LINE});
std::cout << fig.mosaic_layout().rows << "x" << fig.mosaic_layout().cols << "\\n";
fig.save_svg("figure.svg");"""),
        ex("errors", "A grid needs at least one row and one column, and a gutter is `auto_gutter` or a finite, non-negative distance. Anything else is refused at once and the previous grid is kept.",
           f"""auto fig = studio("mosaic_errors");
fig.mosaic(1, 2);
auto report = [&](const char* what, auto&& call) {{
  try {{
    call();
  }} catch (const pluss::Error& e) {{
    std::cout << what << ": " << e.message() << "\\n";
  }}
}};
report("zero rows", [&] {{ fig.mosaic(0, 2); }});
report("negative columns", [&] {{ fig.mosaic(2, -1); }});
report("negative gutter", [&] {{ fig.mosaic(2, 2, -5.0, 10.0); }});
report("NaN gutter", [&] {{ fig.mosaic(2, 2, 10.0, std::numeric_limits<double>::quiet_NaN()); }});
std::cout << "grid kept: " << fig.mosaic_layout().rows << "x" << fig.mosaic_layout().cols << "\\n";
fig.pane2("a").trend({LINE});
fig.pane2("b").trend({LINE});
fig.save_svg("figure.svg");"""),
        ex("overflow", "`mosaic` cannot know how many panes will come, so too many panes is found when the figure is drawn: five panes for four cells.",
           f"""auto fig = studio("mosaic_overflow");
fig.mosaic(2, 2);
for (int i = 0; i < 5; ++i) fig.pane2().trend({LINE});
std::cout << "panes added: " << fig.pane_count() << "\\n";
try {{
  fig.to_svg();
}} catch (const pluss::Error& e) {{
  std::cout << e.message() << "\\n";
}}
auto ok = studio("mosaic_overflow_out");
ok.mosaic(2, 2);
for (int i = 0; i < 4; ++i) ok.pane2().trend({LINE});
ok.save_svg("figure.svg");"""),
        ex("tight", "With the compact layout ([`tight_layout`](tight_layout.md)) the gutters are smaller too: both kinds of gutter go to 62% of their size (a given distance is not reduced below 6).",
           f"""auto build = [&](bool tight) {{
  auto fig = studio("mosaic_tight");
  fig.mosaic(1, 2, 60.0, 60.0);
  if (tight) fig.tight_layout();
  fig.pane2("left").trend({LINE});
  fig.pane2("right").trend({LINE});
  return fig.to_svg();
}};
{AT}
std::cout << "relaxed: right title " << at(build(false), "right") << ", tight: " << at(build(true), "right") << "\\n";
auto fig = studio("mosaic_tight_out");
fig.mosaic(1, 2, 60.0, 60.0).tight_layout();
fig.pane2("left").trend({LINE});
fig.pane2("right").trend({LINE});
fig.save_svg("figure.svg");"""),
    ]
    write("mosaic", dict(
        slug="mosaic", title="Studio::mosaic",
        signature="Studio& Studio::mosaic(std::int64_t rows, std::int64_t cols, double gutter_x = auto_gutter, double gutter_y = auto_gutter)",
        intro="""Divides the figure into a grid of `rows` by `cols` equal cells, with a gutter between them, and lays the panes out in it. Without a `mosaic` call a figure has a single cell, which a single pane fills.

**Which cell a pane gets.** A pane made by [`pane2`](pane2.md) or [`pane3`](pane3.md) takes the next free cell, row by row from the top left, in the order the panes were added. A pane placed with [`Pane::place`](../pane/place.md) takes the cell (or block of cells) it asks for, and a pane made by [`subpane2`](subpane2.md) or [`subpane3`](subpane3.md) is laid out inside another pane. Placed panes claim their cells first; the others fill what is left.

**Gutters.** `gutter_x` is the gap between columns and `gutter_y` between rows. `auto_gutter`, the default, is a share of a cell, so the gaps follow the size of the figure. A number is a distance in drawing units, used as it is: 0 puts cells edge to edge. (In the compact layout, [`tight_layout`](tight_layout.md), both kinds are reduced.)

**Checks.** `rows` and `cols` must be positive, and each gutter `auto_gutter` or a finite non-negative number; otherwise the call throws at once and the previous grid is kept. Whether the panes fit the grid cannot be known here, since more may be added; that is checked when the figure is drawn, and the error says what went wrong (see Example 9).""",
        parameters="""**rows**, **cols** (`std::int64_t`)
    The number of rows and columns. Positive.

**gutter_x**, **gutter_y** (`double`, default `auto_gutter`)
    The gap between columns and between rows: `auto_gutter` for a share of a cell, or a distance. Finite and not negative.""",
        returns="""`Studio&`
    The same figure, for chaining.""",
        examples=ex_,
        notes="- Calling `mosaic` again replaces the grid; the panes keep the order they were added in.\n- [`mosaic_layout`](mosaic_layout.md) reads the grid back.\n- Overflow and overlapping placed panes are reported when the figure is drawn, by `to_svg`, `show` or a save.",
        see=["[`Studio::mosaic_layout`](mosaic_layout.md): read the grid back.", "[`Pane::place`](../pane/place.md): put a pane in a chosen cell or block.", "[`Studio::subpane2`](subpane2.md), [`subpane3`](subpane3.md): a grid inside a pane.", "[`Studio::tight_layout`](tight_layout.md): the compact layout.", "[`Studio::size`](size.md): the canvas the grid divides."]))


def tight_layout():
    ex_ = [
        ex("default", "A new figure uses the relaxed layout.",
           f"""auto fig = studio("tight_default");
fig.pane2("p").trend({LINE});
std::cout << "enabled: " << fig.tight_layout_enabled() << ", padding " << fig.tight_layout_padding() << "\\n";
fig.save_svg("figure.svg");"""),
        ex("on", "With the compact layout the plot area is larger: the tick labels of the y axis, which end at the axis, stand further to the left, and the data is drawn over more of the canvas.",
           f"""auto build = [&](bool tight) {{
  auto fig = studio("tight_on");
  if (tight) fig.tight_layout();
  fig.pane2("p").trend(std::vector<double>{{0, 1, 2}}, std::vector<double>{{0, 10, 0}});
  return fig.to_svg();
}};
{AT}
std::cout << "tick '10' at " << at(build(false), "10") << " relaxed, " << at(build(true), "10") << " compact\\n";
auto fig = studio("tight_on_out");
fig.tight_layout();
fig.pane2("p").trend(std::vector<double>{{0, 1, 2}}, std::vector<double>{{0, 10, 0}});
fig.save_svg("figure.svg");"""),
        ex("padding", "The padding is room given back around the edges of the figure. 0 is the most compact, and a larger padding moves everything in from the edge.",
           f"""auto build = [&](double padding) {{
  auto fig = studio("tight_padding");
  fig.tight_layout(true, padding);
  fig.pane2("p").trend(std::vector<double>{{0, 1, 2}}, std::vector<double>{{0, 10, 0}});
  return fig.to_svg();
}};
{AT}
for (double padding : {{0.0, 6.0, 24.0}}) std::cout << "padding " << padding << ": tick '10' at " << at(build(padding), "10") << "\\n";
auto fig = studio("tight_padding_out");
fig.tight_layout(true, 24.0);
fig.pane2("p").trend(std::vector<double>{{0, 1, 2}}, std::vector<double>{{0, 10, 0}});
fig.save_svg("figure.svg");"""),
        ex("no_args", "`tight_layout()` is `tight_layout(true, 6.0)`.",
           f"""auto build = [&](bool explicit_args) {{
  auto fig = studio("tight_no_args");
  if (explicit_args) fig.tight_layout(true, 6.0);
  else fig.tight_layout();
  fig.pane2("p").trend({LINE});
  return fig.to_svg();
}};
std::cout << "same figure: " << (build(true) == build(false)) << "\\n";
auto fig = studio("tight_no_args_out");
fig.tight_layout();
fig.pane2("p").trend({LINE});
fig.save_svg("figure.svg");"""),
        ex("off_again", "`tight_layout(false)` goes back to the relaxed layout: the figure is the one that never asked for the compact one.",
           f"""auto build = [&](bool toggled) {{
  auto fig = studio("tight_off_again");
  if (toggled) fig.tight_layout().tight_layout(false);
  fig.pane2("p").trend({LINE});
  return fig.to_svg();
}};
std::cout << "same as never set: " << (build(true) == build(false)) << "\\n";
auto fig = studio("tight_off_again_out");
fig.pane2("p").trend({LINE});
fig.save_svg("figure.svg");"""),
        ex("many_panes", "It matters most with many panes, where each cell has little room: a 2x3 grid of small plots in the relaxed and in the compact layout. The compact figure gives the plots more of the canvas.",
           f"""auto build = [&](bool tight) {{
  auto fig = studio("tight_many");
  fig.mosaic(2, 3);
  if (tight) fig.tight_layout(true, 0.0);
  for (int i = 0; i < 6; ++i) {{
    auto& pane = fig.pane2("pane " + std::to_string(i));
    pane.trend({LINE});
    pane.x_label("x").y_label("y");
  }}
  return fig;
}};
auto relaxed = build(false);
auto compact = build(true);
std::cout << "figures differ: " << (relaxed.to_svg() != compact.to_svg()) << "\\n";
compact.save_svg("figure.svg");"""),
        ex("errors", "The padding must be finite and not negative; otherwise the call throws, and the layout and padding stay as they were.",
           f"""auto fig = studio("tight_errors");
for (double bad : {{-1.0, std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity()}}) {{
  try {{
    fig.tight_layout(true, bad);
  }} catch (const pluss::Error& e) {{
    std::cout << e.message() << "\\n";
  }}
}}
std::cout << "enabled: " << fig.tight_layout_enabled() << ", padding " << fig.tight_layout_padding() << "\\n";
fig.pane2("p").trend({LINE});
fig.save_svg("figure.svg");"""),
    ]
    write("tight_layout", dict(
        slug="tight_layout", title="Studio::tight_layout",
        signature="Studio& Studio::tight_layout(bool enable = true, double padding = 6.0)",
        intro="""Switches the figure between its relaxed layout (the default) and a compact one that gives more of the canvas to the plots. The compact layout uses smaller margins at the figure's edges, smaller room around each plot for tick labels and axis labels, and smaller gutters between the cells of a [`mosaic`](mosaic.md); a `padding` is then added back around the edges.

Use it for figures with many small panes, or for a figure that is going to be placed in a page where the margins are already there. The layout is a set of proportions, not a fixed shrinkage: texts and labels are measured as before, so nothing is cut off in the compact layout that was not cut in the relaxed one.

`enable = false` returns to the relaxed layout. The padding is only used while the compact layout is on, but it is stored and checked in every call, so a bad value is always refused.""",
        parameters="""**enable** (`bool`, default `true`)
    `true` for the compact layout, `false` for the relaxed one.

**padding** (`double`, default 6.0)
    Room added back around the edges of the figure, in drawing units. Finite and not negative.""",
        returns="""`Studio&`
    The same figure, for chaining.""",
        examples=ex_,
        notes="- The padding is stored by every call, with the default when none is given, so `tight_layout()` after `tight_layout(true, 20)` resets it to 6.\n- Read the state with [`tight_layout_enabled`](tight_layout_enabled.md) and [`tight_layout_padding`](tight_layout_padding.md).",
        see=["[`Studio::tight_layout_enabled`](tight_layout_enabled.md), [`tight_layout_padding`](tight_layout_padding.md): read the state.", "[`Studio::mosaic`](mosaic.md): the gutters the compact layout reduces.", "[`Studio::size`](size.md): the canvas."]))


def context_scales():
    base = f"""auto fig = studio("{{n}}");"""
    ex_ = [
        ex("default", "A new figure's panes start at 1 for both, which is the default size of tick and axis label texts.",
           f"""auto fig = studio("scales_default");
auto& pane = fig.pane2("p");
pane.trend({LINE});
pane.x_label("time");
{SIZE}
const std::string svg = fig.to_svg();
std::cout << "figure: " << fig.default_tick_font_scale() << " / " << fig.default_axis_label_font_scale() << "; pane: " << pane.tick_font_scale() << " / " << pane.axis_label_font_scale() << "\\n";
std::cout << "tick size " << size_of(svg, "1.0") << ", axis label size " << size_of(svg, "time") << "\\n";
fig.save_svg("figure.svg");"""),
        ex("larger", "Setting the scales before the panes are made: every pane takes them. With 1.4 and 1.2 the tick labels and axis labels are larger in both panes of the figure.",
           f"""auto fig = studio("scales_larger");
fig.context_scales(1.4, 1.2);
fig.mosaic(1, 2);
for (int i = 0; i < 2; ++i) {{
  auto& pane = fig.pane2("pane " + std::to_string(i));
  pane.trend({LINE});
  pane.x_label("time");
}}
{SIZE}
const std::string svg = fig.to_svg();
std::cout << "tick size " << size_of(svg, "1.0") << ", axis label size " << size_of(svg, "time") << "\\n";
std::cout << "panes: " << fig.panes()[0].tick_font_scale() << ", " << fig.panes()[1].tick_font_scale() << "\\n";
fig.save_svg("figure.svg");"""),
        ex("after", "Only panes made afterwards are affected. The first pane was made before the call and keeps 1, the second takes 2.",
           f"""auto fig = studio("scales_after");
auto& before = fig.pane2("before");
before.trend({LINE});
fig.context_scales(2.0, 2.0);
auto& after = fig.pane2("after");
after.trend({LINE});
std::cout << "before: " << before.tick_font_scale() << ", after: " << after.tick_font_scale() << "\\n";
fig.save_svg("figure.svg");"""),
        ex("pane_wins", "A pane's own call is made on the pane and wins for it. The figure's scales set the starting point; `Pane::tick_font_scale` then changes one pane.",
           f"""auto fig = studio("scales_pane_wins");
fig.context_scales(1.5, 1.5);
fig.mosaic(1, 2);
auto& plain = fig.pane2("figure default");
plain.trend({LINE});
auto& own = fig.pane2("own scale");
own.trend({LINE});
own.tick_font_scale(0.8);
std::cout << "plain: " << plain.tick_font_scale() << ", own: " << own.tick_font_scale() << "\\n";
fig.save_svg("figure.svg");"""),
        ex("smaller", "Scales below 1 make text smaller, to fit a small canvas with many panes.",
           f"""auto fig = studio("scales_smaller");
fig.size(640.0, 400.0);
fig.context_scales(0.8, 0.8);
fig.mosaic(2, 2);
for (int i = 0; i < 4; ++i) {{
  auto& pane = fig.pane2("pane " + std::to_string(i));
  pane.trend({LINE});
  pane.x_label("x");
}}
fig.save_svg("figure.svg");"""),
        ex("errors", "Each scale must be finite and positive. A bad value is refused with a message that names it, and the figure's scales are not changed.",
           f"""auto fig = studio("scales_errors");
auto report = [&](const char* what, auto&& call) {{
  try {{
    call();
  }} catch (const pluss::Error& e) {{
    std::cout << what << ": " << e.message() << "\\n";
  }}
}};
report("tick zero", [&] {{ fig.context_scales(0.0, 1.0); }});
report("tick negative", [&] {{ fig.context_scales(-1.0, 1.0); }});
report("tick NaN", [&] {{ fig.context_scales(std::numeric_limits<double>::quiet_NaN(), 1.0); }});
report("label zero", [&] {{ fig.context_scales(1.0, 0.0); }});
report("label infinite", [&] {{ fig.context_scales(1.0, std::numeric_limits<double>::infinity()); }});
std::cout << "unchanged: " << fig.default_tick_font_scale() << " / " << fig.default_axis_label_font_scale() << "\\n";
fig.pane2("p").trend({LINE});
fig.save_svg("figure.svg");"""),
    ]
    write("context_scales", dict(
        slug="context_scales", title="Studio::context_scales",
        signature="Studio& Studio::context_scales(double tick_font_scale, double axis_label_font_scale)",
        intro="""Sets the scales of text that panes **made afterwards** start with: the factor on the tick label size and the factor on the axis label size, 1 being the default size of each. It is the one call that makes all the text of a figure larger or smaller, as for a poster, a slide or a small thumbnail, without setting [`Pane::tick_font_scale`](../pane/tick_font_scale.md) and [`axis_label_font_scale`](../pane/axis_label_font_scale.md) pane by pane.

Like [`theme`](theme.md), it is a default that [`pane2`](pane2.md) and [`pane3`](pane3.md) copy into the new pane when they make it. Panes that already exist keep what they have, so call `context_scales` before the first pane. A pane's own calls still change it afterwards. The layout follows the text: larger labels get the room they need, so they are not cut off.

Both scales must be finite and positive, or the call throws and nothing changes.""",
        parameters="""**tick_font_scale** (`double`)
    The factor on the tick label size for new panes. Finite and positive.

**axis_label_font_scale** (`double`)
    The factor on the axis label size for new panes. Finite and positive.""",
        returns="""`Studio&`
    The same figure, for chaining.""",
        examples=ex_,
        notes="- Read the defaults back with `default_tick_font_scale()` and `default_axis_label_font_scale()`.\n- The titles, the headline and the legends are not scaled by this call.",
        see=["[`Pane::tick_font_scale`](../pane/tick_font_scale.md), [`Pane::axis_label_font_scale`](../pane/axis_label_font_scale.md): the scales of one pane.", "[`Studio::theme`](theme.md): the other default panes copy when they are made.", "[`Studio::pane2`](pane2.md): makes a pane."]))


if __name__ == "__main__":
    mosaic(); tight_layout(); context_scales()
    print("wrote studio layout specs")
