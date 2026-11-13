#!/usr/bin/env python3
"""Writes tools/specs/StyleOptions.py (the type page, with the table of every value read from include/pluss/pl/style.h)
and the method specs of Studio::style and Studio::style_options. Run, then plotdocs.py StyleOptions and
plotdocs.py --methods studio."""
import pathlib, re, sys

HERE = pathlib.Path(__file__).resolve().parent
STYLE_H = HERE.parents[2] / "include" / "pluss" / "pl" / "style.h"


def parse():
    """[(section, [(name, kind, default_text, meaning)])] in file order."""
    text = STYLE_H.read_text()
    body = text[text.index("struct StyleOptions {") + len("struct StyleOptions {"):text.index("[[nodiscard]] inline const char* first_non_finite")]
    values = {}
    sections, current = [], None
    pending_comment = []
    for raw in body.split("\n"):
        line = raw.strip()
        m = re.match(r"//\s*-{3,}\s*(.*?)\s*-{3,}", line)
        if m:
            current = [m.group(1).rstrip(". ").strip(), []]
            sections.append(current)
            pending_comment = []
            continue
        if line.startswith("//"):
            pending_comment.append(line[2:].strip())
            continue
        m = re.match(r"(double|int|std::size_t|StyleSpacing|StyleMaterial|Rgba) (\w+)\s*(\{.*?\}|=\s*[^;]*);\s*(//.*)?$", line)
        if not m:
            continue
        typ, name, init, comment = m.groups()
        meaning = (comment[2:].strip() if comment else "")
        init_text = init.strip()
        if init_text.startswith("="):
            init_text = init_text[1:].strip()
        value = None
        if typ in ("double", "int", "std::size_t"):
            expr = init_text.strip("{}")
            try:
                value = eval(re.sub(r"\b[a-z_][a-z0-9_]*\b", lambda mm: repr(values[mm.group(0)]) if mm.group(0) in values and isinstance(values[mm.group(0)], (int, float)) else mm.group(0), expr), {"__builtins__": {}})
            except Exception:
                value = None
            if isinstance(value, (int, float)):
                values[name] = value
            shown = fmt(value) if value is not None else init_text.strip("{}")
        elif typ == "StyleSpacing":
            parts = [p.strip() for p in init_text.strip("{}").split(",")]
            vals = []
            for p in parts:
                try:
                    vals.append(eval(re.sub(r"\b[a-z_][a-z0-9_]*\b", lambda mm: repr(values[mm.group(0)]) if mm.group(0) in values and isinstance(values[mm.group(0)], (int, float)) else mm.group(0), p), {"__builtins__": {}}))
                except Exception:
                    vals.append(None)
            shown = "relaxed " + (fmt(vals[0]) if vals[0] is not None else parts[0]) + ", compact " + (fmt(vals[1]) if vals[1] is not None else parts[1])
        elif typ == "StyleMaterial":
            nums = [p.strip() for p in init_text.strip("{}").split(",")]
            shown = "ambient %s, diffuse %s, specular %s, exponent %s, highlight %s" % tuple(nums) if len(nums) == 5 else init_text
        else:  # Rgba
            mm = re.search(r"from_u8\(([^)]*)\)", init_text)
            shown = "rgba(" + mm.group(1).replace(" ", "") + ")" if mm else init_text
        if current is None:
            current = ["Values", []]
            sections.append(current)
        current[1].append((name, typ, shown, meaning))
        pending_comment = []
    return sections


def fmt(v):
    if isinstance(v, int):
        return str(v)
    if abs(v) >= 1e5 or (v != 0 and abs(v) < 1e-3):
        return f"{v:g}"
    return f"{v:.6g}"


def table(sections):
    out = []
    for title, rows in sections:
        if not rows:
            continue
        out += [f"### {title}", "", "| Name | Meaning | Default |", "|---|---|---|"]
        for name, typ, shown, meaning in rows:
            out.append(f"| `{name}` | {meaning or '-'} | {shown} |")
        out.append("")
    return "\n".join(out)


def write_type(sections):
    count = sum(len(r) for _, r in sections)
    tbl = table(sections)
    page = r'''import sys, pathlib
sys.path.insert(0, str(pathlib.Path(__file__).parent))
from _common import *
PAGE = dict(
    slug="StyleOptions",
    summary="The look of a figure as numbers: sizes, spacings, strokes and colours, each with its default.",
    title="StyleOptions",
    signature="",
    intro="""`StyleOptions` is the set of numbers that give a figure its look: the sizes of the fonts, the length of ticks, the room around a plot, the width of a legend box, the size of a marker, the shading of a surface, the colours of the grid. A figure draws with the defaults below unless it is given another set, so the look of every figure of a program can be changed by one call, and nothing needs to be edited in the library.

To restyle a figure, take a copy of its current values with [`Studio::style_options`](../studio/style_options.md), change the ones you want, and give the set back with [`Studio::style`](../studio/style.md). The set is copied into the figure: later changes to your copy do not reach it, and one set can be given to any number of figures. Values are plain numbers, in these units:

| Suffix | Unit |
|---|---|
| `_pt` | points, the unit of the drawing. |
| `_em` | a multiple of the font size the quantity sits beside, so it follows the font and its scale ([`tick_font_scale`](../pane/tick_font_scale.md), [`context_scales`](../studio/context_scales.md)). |
| `_share` | a fraction of the figure, of a cell or of a plot. |
| `_factor`, `_alpha`, `_shade` | dimensionless multipliers and opacities. |

A value is only a number: a quantity that depends on the content (the gap between the longest tick label and the axis label, the width a legend needs) is not here. It is worked out when the figure is drawn, from these values and from the content. Values marked guard keep the drawing sane (a marker never shrinks below a visible dot) and are not ways to make a distance look right. Every value must be a finite number; a set with another is refused by `Studio::style`, naming the value.

There are <<COUNT>> values, in the groups below. A pair such as `pane_left_inset` holds two numbers, one for the relaxed layout and one for the compact layout ([`tight_layout`](../studio/tight_layout.md)); a material holds the five coefficients of its shading; a colour holds red, green, blue and alpha between 0 and 1.""",
    table_title="## Values",
    table="""<<TBL>>""",
    examples=[
        dict(id="read", text="The defaults, read from a figure that has not been styled. A copy is taken, one value is changed in the copy, and the figure is unchanged until the copy is given back.",
             code="""auto fig = studio("style_read");
fig.pane2("p").trend(std::vector<double>{{0, 1, 2}}, std::vector<double>{{0, 10, 0}});
StyleOptions options = fig.style_options();
std::cout << "tick font " << options.tick_font_pt << " pt, axis label font " << options.axis_label_font_pt << " pt\\n";
options.tick_font_pt = 16.0;
std::cout << "figure still uses " << fig.style_options().tick_font_pt << "\\n";
fig.style(options);
std::cout << "figure now uses " << fig.style_options().tick_font_pt << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="restyle", text="Restyling a figure for a talk: larger fonts, longer ticks and larger markers, set in one place. The two figures hold the same data.",
             code="""auto build = [&](bool talk) {{
  auto fig = studio(talk ? "style_talk" : "style_default");
  auto& pane = fig.pane2("p");
  pane.cloud(std::vector<double>{{1, 2, 3, 4}}, std::vector<double>{{2, 4, 3, 5}}, std::vector<double>{{60, 60, 60, 60}});
  pane.x_label("time").y_label("value");
  if (talk) {{
    StyleOptions options = fig.style_options();
    options.tick_font_pt = 15.0;
    options.axis_label_font_pt = 17.0;
    options.tick_length_em = 0.8;
    options.marker_radius_per_sqrt_size = 0.6;
    fig.style(options);
  }}
  return fig;
}};
auto plain = build(false), talk = build(true);
std::cout << "figures differ: " << (plain.to_svg() != talk.to_svg()) << "\\n";
talk.save_svg("figure.svg");"""),
        dict(id="per_figure", text="A style belongs to the figure it is given to. Two figures built one after the other, the first styled and the second not: the second is drawn with the defaults.",
             code="""auto build = [&](bool styled) {{
  auto fig = studio("style_per_figure");
  fig.pane2("p").trend(std::vector<double>{{0, 1, 2}}, std::vector<double>{{0, 10, 0}});
  if (styled) {{
    StyleOptions options = fig.style_options();
    options.tick_font_pt = 18.0;
    fig.style(options);
  }}
  return fig.to_svg();
}};
const std::string styled_first = build(true);
const std::string plain_after = build(false);
const std::string plain_alone = build(false);
std::cout << "unstyled figure after a styled one is unchanged: " << (plain_after == plain_alone) << "\\n";
std::cout << "styled differs from unstyled: " << (styled_first != plain_after) << "\\n";
auto fig = studio("style_per_figure_out");
fig.pane2("p").trend(std::vector<double>{{0, 1, 2}}, std::vector<double>{{0, 10, 0}});
fig.save_svg("figure.svg");"""),
        dict(id="shared", text="One set for many figures. A program that makes several figures builds the set once and gives it to each, so they share a look.",
             code="""StyleOptions house;
house.tick_font_pt = 12.0;
house.axis_label_font_pt = 13.0;
house.headline_font_pt = 22.0;
auto fig_a = studio("style_shared_a");
auto fig_b = studio("style_shared_b");
for (auto* fig : {{&fig_a, &fig_b}}) {{
  fig->style(house);
  fig->headline("Same look");
  fig->pane2("p").trend(std::vector<double>{{0, 1, 2}}, std::vector<double>{{1, 3, 2}});
}}
std::cout << "same tick font: " << (fig_a.style_options().tick_font_pt == fig_b.style_options().tick_font_pt) << "\\n";
fig_a.save_svg("figure.svg");"""),
        dict(id="spacing", text="A pair of values for the two layouts. The room left of the plot in the relaxed layout is changed, and the figure moves; the compact layout keeps its own value.",
             code="""auto build = [&](double left) {{
  auto fig = studio("style_spacing");
  fig.pane2("p").trend(std::vector<double>{{0, 1, 2}}, std::vector<double>{{0, 10, 0}});
  StyleOptions options = fig.style_options();
  options.pane_left_inset.relaxed = left;
  fig.style(options);
  return fig.to_svg();
}};
const double defaults = StyleOptions{{}}.pane_left_inset.relaxed;
std::cout << "default relaxed inset: " << defaults << " pt, compact: " << StyleOptions{{}}.pane_left_inset.compact << " pt\\n";
std::cout << "a larger inset changes the figure: " << (build(defaults + 40.0) != build(defaults)) << "\\n";
auto fig = studio("style_spacing_out");
fig.pane2("p").trend(std::vector<double>{{0, 1, 2}}, std::vector<double>{{0, 10, 0}});
StyleOptions options = fig.style_options();
options.pane_left_inset.relaxed = defaults + 40.0;
fig.style(options);
fig.save_svg("figure.svg");"""),
        dict(id="errors", text="A value that is not a finite number is refused, naming it, and the figure keeps the style it had.",
             code="""auto fig = studio("style_errors");
fig.pane2("p").trend(std::vector<double>{{0, 1, 2}}, std::vector<double>{{0, 10, 0}});
StyleOptions options = fig.style_options();
options.tick_font_pt = std::numeric_limits<double>::quiet_NaN();
try {{
  fig.style(options);
}} catch (const pluss::Error& e) {{
  std::cout << e.message() << "\\n";
}}
options.tick_font_pt = std::numeric_limits<double>::infinity();
try {{
  fig.style(options);
}} catch (const pluss::Error& e) {{
  std::cout << e.message() << "\\n";
}}
std::cout << "figure keeps " << fig.style_options().tick_font_pt << "\\n";
fig.save_svg("figure.svg");"""),
    ],
    notes="""- A set is copied into the figure when it is given, and a figure that was never given one uses the defaults listed above.
- Fonts that a figure scales ([`context_scales`](../studio/context_scales.md), [`Pane::tick_font_scale`](../pane/tick_font_scale.md)) multiply the point sizes here.
- The values here are the whole of the figure's taste. Changing them does not change what is drawn, only how it looks.""",
    see=["[`Studio::style`](../studio/style.md): give a figure a set of values.", "[`Studio::style_options`](../studio/style_options.md): read the values a figure draws with.", "[`Studio::tight_layout`](../studio/tight_layout.md): the compact layout, which the pairs of values serve.", "[`Studio::context_scales`](../studio/context_scales.md): scale the text of a whole figure."],
)
'''
    page = page.replace("{{", "{").replace("}}", "}").replace("<<COUNT>>", str(count)).replace("<<TBL>>", tbl)
    (HERE / "specs" / "StyleOptions.py").write_text(page)


def write_methods():
    import gen_axis_specs as g
    out = HERE / "method_specs" / "studio"

    def w(slug, page):
        out.joinpath(f"{slug}.py").write_text("PAGE = " + g.repr_page(page) + "\n")
    w("style", dict(
        slug="style", title="Studio::style",
        signature="Studio& Studio::style(const StyleOptions& options)",
        intro="""Gives the figure a set of [`StyleOptions`](../types/StyleOptions.md): the numbers that decide its look, such as the sizes of fonts, the length of ticks, the room around a plot and the size of markers. The set is copied into the figure and used every time the figure is drawn, by [`to_svg`](to_svg.md), [`show`](show.md) and the save calls. A figure that was never given a set is drawn with the defaults.

The usual way is to take a copy of the current values with [`style_options`](style_options.md), change some, and give the copy back. A set can be given to any number of figures, and a later call replaces the earlier one entirely, so a call after a change to a copy is the way to apply it.

Every value must be a finite number. A set with a value that is not (a `NaN` or an infinity) is refused at once, with a message that names the value, and the figure keeps the style it had.""",
        parameters="""**options** (`StyleOptions`)
    The values to draw with.""",
        returns="""`Studio&`
    The same figure, for chaining.""",
        examples=[
            dict(id="basic", text="Giving a figure a larger tick font. The figure drawn with the set differs from the one drawn without it.",
                 code="""auto build = [&](bool styled) {
  auto fig = studio("style_basic");
  fig.pane2("p").trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 10, 0});
  if (styled) {
    StyleOptions options = fig.style_options();
    options.tick_font_pt = 16.0;
    fig.style(options);
  }
  return fig.to_svg();
};
std::cout << "styled differs: " << (build(true) != build(false)) << "\\n";
auto fig = studio("style_basic_out");
fig.pane2("p").trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 10, 0});
StyleOptions options = fig.style_options();
options.tick_font_pt = 16.0;
fig.style(options);
fig.save_svg("figure.svg");"""),
            dict(id="replace", text="A later call replaces the earlier set as a whole: values changed by the first call and not by the second are back at their defaults, because the second set was made from the defaults.",
                 code="""auto fig = studio("style_replace");
fig.pane2("p").trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 10, 0});
StyleOptions first;
first.tick_font_pt = 16.0;
fig.style(first);
StyleOptions second;
second.axis_label_font_pt = 15.0;
fig.style(second);
std::cout << "tick font " << fig.style_options().tick_font_pt << ", axis label font " << fig.style_options().axis_label_font_pt << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="copy", text="The set is copied when it is given. Changing the variable afterwards does not change the figure.",
                 code="""auto fig = studio("style_copy");
fig.pane2("p").trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 10, 0});
StyleOptions options;
options.tick_font_pt = 14.0;
fig.style(options);
options.tick_font_pt = 30.0;
std::cout << "figure uses " << fig.style_options().tick_font_pt << ", the variable holds " << options.tick_font_pt << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="chain", text="The call returns the figure, so it chains with the other figure settings.",
                 code="""StyleOptions options;
options.headline_font_pt = 24.0;
auto fig = studio("style_chain");
fig.style(options).headline("A larger headline").size(900.0, 500.0);
fig.pane2("p").trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 10, 0});
fig.save_svg("figure.svg");"""),
            dict(id="errors", text="A value that is not finite is refused, and the figure keeps its style.",
                 code="""auto fig = studio("style_errors_method");
fig.pane2("p").trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 10, 0});
StyleOptions options;
options.axis_label_font_pt = std::numeric_limits<double>::infinity();
try {
  fig.style(options);
} catch (const pluss::Error& e) {
  std::cout << e.message() << "\\n";
}
std::cout << "axis label font still " << fig.style_options().axis_label_font_pt << "\\n";
fig.save_svg("figure.svg");"""),
        ],
        notes="- Not `noexcept`: the set is copied, and a non-finite value throws `pluss::Error`.\n- Only the values listed on the [`StyleOptions`](../types/StyleOptions.md) page can be changed; the layout that depends on content is still worked out from the content.",
        see=["[`StyleOptions`](../types/StyleOptions.md): every value, with its default.", "[`Studio::style_options`](style_options.md): read the values a figure draws with.", "[`Studio::context_scales`](context_scales.md): scale the text of the whole figure."]))
    w("style_options", dict(
        slug="style_options", title="Studio::style_options",
        signature="const StyleOptions& Studio::style_options() const",
        intro="""Returns the [`StyleOptions`](../types/StyleOptions.md) the figure draws with: the set it was given by [`style`](style.md), or the defaults if it was never given one. It is the way to start a restyling, by taking a copy and changing the values that should differ, and to check what a figure will use.

The reference is to the figure's own set and follows later calls to `style`; copy it to keep the values.""",
        parameters="*(none)*",
        returns="""`const StyleOptions&`
    The values the figure draws with.""",
        examples=[
            dict(id="defaults", text="A figure that was never styled reports the defaults.",
                 code="""auto fig = studio("style_options_defaults");
fig.pane2("p").trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 10, 0});
const StyleOptions& options = fig.style_options();
const StyleOptions defaults;
std::cout << "tick font " << options.tick_font_pt << " pt; equal to the defaults: " << (options.tick_font_pt == defaults.tick_font_pt && options.pane_left_inset.relaxed == defaults.pane_left_inset.relaxed) << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="after_style", text="After `style` it reports the set that was given.",
                 code="""auto fig = studio("style_options_after");
fig.pane2("p").trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 10, 0});
StyleOptions given;
given.tick_font_pt = 13.0;
fig.style(given);
std::cout << "tick font " << fig.style_options().tick_font_pt << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="start_from", text="Starting from a figure's own values. A copy of the current set is changed and given back, which changes only the value touched.",
                 code="""auto fig = studio("style_options_start");
fig.pane2("p").trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 10, 0});
StyleOptions options = fig.style_options();
const double before_label = options.axis_label_font_pt;
options.tick_font_pt = 15.0;
fig.style(options);
std::cout << "tick font " << fig.style_options().tick_font_pt << ", axis label font unchanged: " << (fig.style_options().axis_label_font_pt == before_label) << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="per_figure", text="Each figure has its own set: one figure styled, another not.",
                 code="""auto styled = studio("style_options_styled");
auto plain = studio("style_options_plain");
StyleOptions options;
options.tick_font_pt = 20.0;
styled.style(options);
plain.pane2("p").trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 10, 0});
styled.pane2("p").trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 10, 0});
std::cout << "styled " << styled.style_options().tick_font_pt << ", plain " << plain.style_options().tick_font_pt << "\\n";
styled.save_svg("figure.svg");"""),
        ],
        notes="- Reads only; a reference into the figure, valid until the figure is destroyed.",
        see=["[`Studio::style`](style.md): give the figure a set.", "[`StyleOptions`](../types/StyleOptions.md): every value, with its default."]))


if __name__ == "__main__":
    sys.path.insert(0, str(HERE))
    secs = parse()
    write_type(secs)
    write_methods()
    print(sum(len(r) for _, r in secs), "values in", len(secs), "groups")
