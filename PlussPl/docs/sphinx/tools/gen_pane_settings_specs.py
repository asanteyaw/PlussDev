#!/usr/bin/env python3
"""Method specs for the display settings of a Pane (legend, grid, kind, visibility, labels' placement, colour scale,
font scales, title, caption) into tools/method_specs/pane/. Run, then `python3 tools/plotdocs.py --methods pane`."""
import sys, pathlib
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from gen_axis_specs import write, TEXTS

AT = r"""auto at = [](const std::string& svg, const std::string& text) {
  const auto hit = svg.find(">" + text + "<");
  if (hit == std::string::npos) return std::string("absent");
  const auto open = svg.rfind("<text", hit);
  auto number = [&](const char* key) {
    const auto k = svg.find(std::string(key) + "=\"", open) + std::string(key).size() + 2;
    return std::stod(svg.substr(k, svg.find('"', k) - k));
  };
  return "(" + std::to_string(static_cast<int>(number("x"))) + ", " + std::to_string(static_cast<int>(number("y"))) + ")";
};"""

LOOK = r"""auto look = [](const std::string& svg, const std::string& text) {
  const auto hit = svg.find(">" + text + "<");
  if (hit == std::string::npos) return std::string("absent");
  const auto open = svg.rfind("<text", hit);
  const std::string tag = svg.substr(open, hit - open);
  const auto anchor = tag.find("text-anchor=\"");
  std::string out = "anchor " + tag.substr(anchor + 13, tag.find('"', anchor + 13) - anchor - 13);
  const auto rot = tag.find("rotate(");
  out += rot == std::string::npos ? ", upright" : ", rotated " + tag.substr(rot + 7, tag.find(' ', rot) - rot - 7);
  return out;
};"""

SIZE = r"""auto size_of = [](const std::string& svg, const std::string& text) {
  const auto hit = svg.find(">" + text + "<");
  if (hit == std::string::npos) return std::string("absent");
  const auto open = svg.rfind("<text", hit);
  const auto k = svg.find("font-size=\"", open) + 11;
  return svg.substr(k, svg.find('"', k) - k);
};"""

TWO = """auto fig = studio("{name}");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{{1, 2, 3, 4, 5}}, std::vector<double>{{2, 4, 3, 5, 6}}).label("series a");
pane.trend(std::vector<double>{{1, 2, 3, 4, 5}}, std::vector<double>{{3, 5, 4, 6, 7}}).label("series b");"""

TRY = """try {{
  {call};
}} catch (const pluss::Error& e) {{
  std::cout << e.message() << "\\n";
}}"""


def two(name):
    return TWO.format(name=name)


# ------------------------------------------------------------------ legend
def legend_page():
    places = [("NorthEast", "the upper right corner of the plot area (the usual place)"),
              ("NorthWest", "the upper left corner"), ("SouthEast", "the lower right corner"),
              ("SouthWest", "the lower left corner"),
              ("OutsideEast", "outside the plot area, to its right: the plot is left as wide as it was and the legend takes room beside it")]
    ex = [dict(id="none", text="The default is no inset legend: the traces carry labels, but nothing is drawn for them until a place is chosen.",
               code=f"""{two('pane_legend_none')}
const std::string svg = fig.to_svg();
std::cout << "place is None: " << (pane.legend_place() == KeyPlace::None) << "\\n";
std::cout << "label drawn: " << (svg.find(">series a<") != std::string::npos) << "\\n";
fig.save_svg("figure.svg");""")]
    for name, desc in places:
        ex.append(dict(id=name.lower(), text=f"`KeyPlace::{name}`: {desc}. The position of the first entry's text, read from the figure, shows where the box went.",
                       code=f"""{two('pane_legend_' + name.lower())}
pane.legend(KeyPlace::{name});
const std::string svg = fig.to_svg();
{AT}
std::cout << "series a at " << at(svg, "series a") << ", series b at " << at(svg, "series b") << "\\n";
fig.save_svg("figure.svg");"""))
    ex += [
        dict(id="auto", text="`KeyPlace::Auto` tries the four corners and takes the one where the legend covers the least of what is already drawn. Where the data runs from the upper left down to the lower right the upper right is free and `Auto` takes it; where it rises, the upper right is covered and `Auto` takes the upper left. The line is sampled along its length, so a line crossing a corner counts even with no data point in it. When two corners cover equally little, the first of NorthEast, NorthWest, SouthEast, SouthWest is taken.",
             code="""auto build = [&](bool falling, KeyPlace place) {
  auto fig = studio("pane_legend_auto");
  auto& pane = fig.pane2("p");
  std::vector<double> x{1, 2, 3, 4, 5};
  std::vector<double> a = falling ? std::vector<double>{6, 5, 4, 3, 2} : std::vector<double>{2, 3, 4, 5, 6};
  std::vector<double> b = falling ? std::vector<double>{5, 4, 3, 2, 1} : std::vector<double>{1, 2, 3, 4, 5};
  pane.trend(x, a).label("series a");
  pane.trend(x, b).label("series b");
  pane.legend(place);
  return fig.to_svg();
};
const KeyPlace corners[] = {KeyPlace::NorthEast, KeyPlace::NorthWest, KeyPlace::SouthEast, KeyPlace::SouthWest};
const char* names[] = {"NorthEast", "NorthWest", "SouthEast", "SouthWest"};
for (bool falling : {true, false}) {
  for (int i = 0; i < 4; ++i) {
    if (build(falling, KeyPlace::Auto) == build(falling, corners[i])) {
      std::cout << (falling ? "falling" : "rising ") << " data: Auto = " << names[i] << "\\n";
      break;
    }
  }
}
auto fig = studio("pane_legend_auto_out");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{1, 2, 3, 4, 5}, std::vector<double>{2, 3, 4, 5, 6}).label("series a");
pane.trend(std::vector<double>{1, 2, 3, 4, 5}, std::vector<double>{1, 2, 3, 4, 5}).label("series b");
pane.legend(KeyPlace::Auto);
fig.save_svg("figure.svg");"""),
        dict(id="only_labelled", text="Only labelled traces are listed. The second trace has no label and is not in the legend; a pane none of whose traces has a label draws no legend, and no room is kept for one.",
             code="""auto fig = studio("pane_legend_labelled");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{1, 2, 3, 4}, std::vector<double>{2, 4, 3, 5}).label("measured");
pane.trend(std::vector<double>{1, 2, 3, 4}, std::vector<double>{1, 3, 2, 4});
pane.legend(KeyPlace::NorthWest);
const std::string svg = fig.to_svg();
std::cout << "measured listed: " << (svg.find(">measured<") != std::string::npos) << "\\n";
auto bare = studio("pane_legend_unlabelled");
auto& other = bare.pane2("p");
other.trend(std::vector<double>{1, 2, 3, 4}, std::vector<double>{2, 4, 3, 5});
auto bare_plain = bare.to_svg();
other.legend(KeyPlace::OutsideEast);
std::cout << "unlabelled pane unchanged by an outside legend: " << (bare.to_svg() == bare_plain) << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="off", text="Turning it off again. `legend(KeyPlace::None)` removes the inset legend; the figure is the figure that never had one.",
             code="""auto build = [&](bool toggled) {
  auto fig = studio("pane_legend_off");
  auto& pane = fig.pane2("p");
  pane.trend(std::vector<double>{1, 2, 3, 4, 5}, std::vector<double>{2, 4, 3, 5, 6}).label("series a");
  if (toggled) {
    pane.legend(KeyPlace::NorthEast);
    pane.legend(KeyPlace::None);
  }
  return fig.to_svg();
};
std::cout << "same as never set: " << (build(true) == build(false)) << "\\n";
auto fig = studio("pane_legend_off_out");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{1, 2, 3, 4, 5}, std::vector<double>{2, 4, 3, 5, 6}).label("series a");
fig.save_svg("figure.svg");"""),
        dict(id="own_and_figure", text="A pane's own legend and the figure's docked legend are separate. Here the pane lists its two series inside its plot area, and the figure docks a legend of all labelled traces to the east; each is drawn where it was asked.",
             code=f"""{two('pane_legend_both')}
pane.legend(KeyPlace::SouthEast);
fig.legend(KeyDock::East);
const std::string svg = fig.to_svg();
const auto first = svg.find(">series a<");
std::cout << "series a drawn twice: " << (first != std::string::npos && svg.find(">series a<", first + 1) != std::string::npos) << "\\n";
fig.save_svg("figure.svg");"""),
    ]
    page = dict(
        slug="legend",
        title="Pane::legend",
        signature="Pane& Pane::legend(KeyPlace place) noexcept",
        intro="""Chooses where the pane draws its own legend, inside or beside its plot area: a box that lists the labelled traces of this pane, one entry per trace, with a sample of the trace's colour and shape and its label. [`Studio::legend`](../studio/legend.md) docks one legend for the whole figure instead; the two are independent and either, both or neither can be on.

| `KeyPlace` | Where the legend goes |
|---|---|
| `None` | Nowhere: no legend. The default. |
| `Auto` | The corner among NorthEast, NorthWest, SouthEast, SouthWest where the legend covers the least of what is drawn; ties go to the first of them in that order. |
| `NorthEast`, `NorthWest`, `SouthEast`, `SouthWest` | That corner of the plot area, inset slightly from the axes. |
| `OutsideEast` | Outside the plot area, to its right. The pane keeps its room for the legend. |

Only traces that have a [`label`](../trace/label.md) are listed, in the order they were added. A pane with no labelled trace draws no legend and keeps no room for it. The frame is chosen with [`legend_frame`](legend_frame.md) and the proportions of the box with [`legend_shape`](legend_shape.md); a label too long for the box wraps, and a label longer than four lines ends in "...".""",
        parameters="""**place** (`KeyPlace`)
    Where to draw the legend; `KeyPlace::None` turns it off.""",
        returns="""`Pane&`
    The same pane, for chaining.""",
        examples=ex,
        notes="""- `noexcept`; no value is invalid.
- The request is stored as given: [`legend_place`](legend_place.md) returns `Auto` after `legend(KeyPlace::Auto)`, not the corner it resolved to; the corner is chosen each time the figure is drawn, from the data as it is then.
- A corner legend sits over the plot area. It is placed clear of the data only by `Auto`; the explicit corners go where they are told.""",
        see=["[`Pane::legend_place`](legend_place.md): read the request back.", "[`Pane::legend_frame`](legend_frame.md), [`legend_shape`](legend_shape.md): the frame and proportions.", "[`Studio::legend`](../studio/legend.md): the legend of the whole figure.", "[`Trace::label`](../trace/label.md): the text of an entry."],
    )
    write("legend", page)


def legend_place_page():
    page = dict(
        slug="legend_place",
        title="Pane::legend_place",
        signature="KeyPlace Pane::legend_place() const noexcept",
        intro="""Returns the placement most recently given to [`legend`](legend.md): `None` (the default), `Auto`, `NorthEast`, `NorthWest`, `SouthEast`, `SouthWest` or `OutsideEast`.

It returns the request, not the result. After `legend(KeyPlace::Auto)` it returns `Auto`, whatever corner the figure then draws the legend in; the corner is chosen when the figure is drawn. Use it to find out whether a pane has an inset legend at all (anything but `None`) before adding a figure-level legend that would repeat it.""",
        parameters="*(none)*",
        returns="""`KeyPlace`
    The placement last set, `None` for a pane that was never given one.""",
        examples=[
            dict(id="default", text="A new pane has no inset legend.",
                 code="""auto fig = studio("legend_place_default");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3}).label("a");
std::cout << "None: " << (pane.legend_place() == KeyPlace::None) << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="each", text="Every placement reads back as set.",
                 code="""auto fig = studio("legend_place_each");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3}).label("a");
const KeyPlace places[] = {KeyPlace::Auto, KeyPlace::NorthEast, KeyPlace::NorthWest, KeyPlace::SouthEast, KeyPlace::SouthWest, KeyPlace::OutsideEast, KeyPlace::None};
const char* names[] = {"Auto", "NorthEast", "NorthWest", "SouthEast", "SouthWest", "OutsideEast", "None"};
for (int i = 0; i < 7; ++i) {
  pane.legend(places[i]);
  std::cout << names[i] << ": " << (pane.legend_place() == places[i]) << "\\n";
}
pane.legend(KeyPlace::NorthWest);
fig.save_svg("figure.svg");"""),
            dict(id="auto_is_a_request", text="`Auto` is a request. The figure draws the legend in one of the four corners, but the accessor still returns `Auto`.",
                 code="""auto fig = studio("legend_place_auto");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{1, 2, 3}, std::vector<double>{3, 2, 1}).label("a");
pane.legend(KeyPlace::Auto);
fig.to_svg();
std::cout << "still Auto after drawing: " << (pane.legend_place() == KeyPlace::Auto) << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="decide", text="Deciding whether a pane already has a legend. A figure-level legend is added only for panes that have none of their own.",
                 code="""auto fig = studio("legend_place_decide");
fig.mosaic(1, 2);
auto& left = fig.pane2("with legend");
left.trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3}).label("a");
left.legend(KeyPlace::NorthEast);
auto& right = fig.pane2("without");
right.trend(std::vector<double>{1, 2, 3}, std::vector<double>{3, 1, 2}).label("b");
for (const auto& pane : fig.panes()) {
  std::cout << pane.title_text() << ": " << (pane.legend_place() != KeyPlace::None ? "has its own legend" : "needs one") << "\\n";
}
fig.save_svg("figure.svg");"""),
        ],
        notes="- `noexcept`; reads only.",
        see=["[`Pane::legend`](legend.md): set the placement.", "[`Pane::legend_frame`](legend_frame.md), [`legend_shape`](legend_shape.md): the other legend settings."],
    )
    write("legend_place", page)


def legend_frame_page():
    ex = [dict(id="default", text="The default is `Soft`: a quiet background behind the entries, with no hard border.",
               code=f"""{two('legend_frame_default')}
pane.legend(KeyPlace::NorthWest);
std::cout << "Soft: " << (pane.legend_frame() == KeyFrame::Soft) << "\\n";
fig.save_svg("figure.svg");""")]
    for name, desc in [("Framed", "a solid background with a border"), ("Soft", "a faint background, no border"), ("None", "no background and no border: the entries sit directly on the plot")]:
        ex.append(dict(id=name.lower(), text=f"`KeyFrame::{name}`: {desc}.",
                       code=f"""{two('legend_frame_' + name.lower())}
pane.legend(KeyPlace::NorthWest);
pane.legend_frame(KeyFrame::{name});
std::cout << "reads back {name}: " << (pane.legend_frame() == KeyFrame::{name}) << "\\n";
fig.save_svg("figure.svg");"""))
    ex.append(dict(id="differ", text="The three frames are three different drawings. Comparing the figures of the same pane with each frame shows that none of them equals another.",
                   code="""auto build = [&](KeyFrame frame) {
  auto fig = studio("legend_frame_compare");
  auto& pane = fig.pane2("p");
  pane.trend(std::vector<double>{1, 2, 3, 4, 5}, std::vector<double>{2, 4, 3, 5, 6}).label("series a");
  pane.legend(KeyPlace::NorthWest);
  pane.legend_frame(frame);
  return fig.to_svg();
};
const auto framed = build(KeyFrame::Framed), soft = build(KeyFrame::Soft), none = build(KeyFrame::None);
std::cout << "framed != soft: " << (framed != soft) << ", soft != none: " << (soft != none) << ", framed != none: " << (framed != none) << "\\n";
auto fig = studio("legend_frame_compare_out");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{1, 2, 3, 4, 5}, std::vector<double>{2, 4, 3, 5, 6}).label("series a");
pane.legend(KeyPlace::NorthWest).legend_frame(KeyFrame::Framed);
fig.save_svg("figure.svg");"""))
    ex.append(dict(id="no_legend", text="Without a legend the frame has nothing to apply to: it is stored, the figure is unchanged, and it takes effect when a legend is turned on.",
                   code="""auto build = [&](bool framed) {
  auto fig = studio("legend_frame_unused");
  auto& pane = fig.pane2("p");
  pane.trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3}).label("a");
  if (framed) pane.legend_frame(KeyFrame::Framed);
  return fig.to_svg();
};
std::cout << "unchanged without a legend: " << (build(true) == build(false)) << "\\n";
auto fig = studio("legend_frame_unused_out");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3}).label("a");
pane.legend_frame(KeyFrame::Framed).legend(KeyPlace::NorthEast);
fig.save_svg("figure.svg");"""))
    page = dict(
        slug="legend_frame",
        title="Pane::legend_frame",
        signature="Pane& Pane::legend_frame(KeyFrame frame) noexcept\nKeyFrame Pane::legend_frame() const noexcept",
        intro="""Chooses how the pane's own legend ([`legend`](legend.md)) is boxed, and reads the choice back. The same `KeyFrame` values style the figure's docked legend through [`Studio::legend_frame`](../studio/legend_frame.md); this one applies to this pane's legend only.

| `KeyFrame` | Drawing |
|---|---|
| `Framed` | A solid background and a border. The legend stands apart from the data. |
| `Soft` | A faint background, no border. The default. |
| `None` | Neither: the entries are drawn directly on the plot. |

The setter and the getter share a name; the call with an argument sets, the call without one reads.""",
        parameters="""**frame** (`KeyFrame`, setter only)
    `Framed`, `Soft` or `None`.""",
        returns="""**Setter:** `Pane&`, the same pane.
**Getter:** `KeyFrame`, the frame in force.""",
        examples=ex,
        notes="- Both forms are `noexcept`.\n- The frame is stored whether or not the pane has a legend; it is drawn when there is one.",
        see=["[`Pane::legend`](legend.md): turn the legend on and place it.", "[`Pane::legend_shape`](legend_shape.md): the proportions of the box.", "[`Studio::legend_frame`](../studio/legend_frame.md): the same for the docked legend.", "[`KeyFrame`](../types/KeyFrame.md): the values it takes."],
    )
    write("legend_frame", page)


def legend_shape_page():
    six = """const char* names[] = {"alpha", "beta", "gamma", "delta", "epsilon", "zeta"};
for (int i = 0; i < 6; ++i) {
  pane.trend(std::vector<double>{1, 2, 3}, std::vector<double>{static_cast<double>(i), i + 1.0, static_cast<double>(i)}).label(names[i]);
}"""
    positions = r"""auto where = [](const std::string& svg, const std::string& text) {
  const auto hit = svg.find(">" + text + "<");
  const auto open = svg.rfind("<text", hit);
  auto number = [&](const char* key) {
    const auto k = svg.find(std::string(key) + "=\"", open) + std::string(key).size() + 2;
    return static_cast<int>(std::stod(svg.substr(k, svg.find('"', k) - k)));
  };
  return "(" + std::to_string(number("x")) + ", " + std::to_string(number("y")) + ")";
};"""
    ex = []
    for name, text in [("Auto", "`KeyShape::Auto`, the default: for a pane's legend, one column, entries listed down the box."),
                       ("Horizontal", "`KeyShape::Horizontal` lays the entries out in a row, and in as many columns as the room allows. In a corner of this figure all six fit on one line."),
                       ("Vertical", "`KeyShape::Vertical` always uses one column, one entry per row.")]:
        ex.append(dict(id=name.lower(), text=text + " The positions of the entries' texts, read from the figure, show the arrangement.",
                       code=f"""auto fig = studio("legend_shape_{name.lower()}");
auto& pane = fig.pane2("p");
{six}
pane.legend(KeyPlace::NorthWest).legend_shape(KeyShape::{name});
const std::string svg = fig.to_svg();
{positions}
std::cout << "reads back {name}: " << (pane.legend_shape() == KeyShape::{name}) << "\\n";
for (const char* name : names) std::cout << name << " " << where(svg, name) << "  ";
std::cout << "\\n";
fig.save_svg("figure.svg");"""))
    ex.append(dict(id="outside", text="Beside the plot the box is narrower, so `Horizontal` wraps its row into columns: here two columns of three. `Auto` and `Vertical` still give one column.",
                   code=f"""auto build = [&](KeyShape shape) {{
  auto fig = studio("legend_shape_outside");
  auto& pane = fig.pane2("p");
{six}
  pane.legend(KeyPlace::OutsideEast).legend_shape(shape);
  return fig.to_svg();
}};
{positions}
const std::string wide = build(KeyShape::Horizontal), tall = build(KeyShape::Vertical);
std::cout << "Horizontal: alpha " << where(wide, "alpha") << " beta " << where(wide, "beta") << " gamma " << where(wide, "gamma") << "\\n";
std::cout << "Vertical:   alpha " << where(tall, "alpha") << " beta " << where(tall, "beta") << " gamma " << where(tall, "gamma") << "\\n";
auto fig = studio("legend_shape_outside_out");
auto& pane = fig.pane2("p");
{six}
pane.legend(KeyPlace::OutsideEast).legend_shape(KeyShape::Horizontal);
fig.save_svg("figure.svg");"""))
    ex.append(dict(id="long", text="Long labels wrap inside whatever box there is. In a column the box is as wide as the longest label needs, up to a share of the plot, and a label wider than that wraps onto more lines; the shape does not make a box wider than its room, so with labels this long every shape gives the same legend.",
                   code="""auto build = [&](KeyShape shape) {
  auto fig = studio("legend_shape_long");
  auto& pane = fig.pane2("p");
  pane.trend(std::vector<double>{1, 2, 3, 4}, std::vector<double>{2, 4, 3, 5}).label("a long descriptive label for the first series");
  pane.trend(std::vector<double>{1, 2, 3, 4}, std::vector<double>{1, 3, 2, 4}).label("another long descriptive label for the second");
  pane.trend(std::vector<double>{1, 2, 3, 4}, std::vector<double>{0, 2, 1, 3}).label("and a third label that is just as long as the rest");
  pane.legend(KeyPlace::NorthWest).legend_shape(shape);
  return fig.to_svg();
};
std::cout << "Auto == Horizontal: " << (build(KeyShape::Auto) == build(KeyShape::Horizontal)) << ", Auto == Vertical: " << (build(KeyShape::Auto) == build(KeyShape::Vertical)) << "\\n";
auto fig = studio("legend_shape_long_out");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{1, 2, 3, 4}, std::vector<double>{2, 4, 3, 5}).label("a long descriptive label for the first series");
pane.trend(std::vector<double>{1, 2, 3, 4}, std::vector<double>{1, 3, 2, 4}).label("another long descriptive label for the second");
pane.legend(KeyPlace::NorthWest);
fig.save_svg("figure.svg");"""))
    ex.append(dict(id="one", text="With one entry there is nothing to arrange: every shape draws the same legend.",
                   code="""auto build = [&](KeyShape shape) {
  auto fig = studio("legend_shape_one");
  auto& pane = fig.pane2("p");
  pane.trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3}).label("only");
  pane.legend(KeyPlace::NorthWest).legend_shape(shape);
  return fig.to_svg();
};
std::cout << "all equal: " << (build(KeyShape::Auto) == build(KeyShape::Horizontal) && build(KeyShape::Auto) == build(KeyShape::Vertical)) << "\\n";
auto fig = studio("legend_shape_one_out");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3}).label("only");
pane.legend(KeyPlace::NorthWest);
fig.save_svg("figure.svg");"""))
    page = dict(
        slug="legend_shape",
        title="Pane::legend_shape",
        signature="Pane& Pane::legend_shape(KeyShape shape) noexcept\nKeyShape Pane::legend_shape() const noexcept",
        intro="""Chooses how the entries of the pane's own legend ([`legend`](legend.md)) are arranged, and reads the choice back.

| `KeyShape` | Arrangement |
|---|---|
| `Auto` | The default. For a pane's legend: one column, entries down the box. |
| `Horizontal` | A wide box: the entries run along rows, in as many columns as the room allows (one row when they all fit, several columns of fewer rows when not). |
| `Vertical` | A narrow box: one column, one entry per row. |

The room is the plot area for a corner legend and a fixed width for `OutsideEast`; the box is never wider than its room. A label too long for the box wraps onto further lines, and one longer than four lines ends in "...". With one entry, or with labels so long that only one fits per row, the shapes draw the same legend. See [`KeyShape`](../types/KeyShape.md) for its values, and [`Studio::legend_shape`](../studio/legend_shape.md) for the legend of the whole figure, where `Auto` depends on the edge it is docked to.""",
        parameters="""**shape** (`KeyShape`, setter only)
    `Auto`, `Horizontal` or `Vertical`.""",
        returns="""**Setter:** `Pane&`, the same pane.
**Getter:** `KeyShape`, the arrangement in force.""",
        examples=ex,
        notes="- Both forms are `noexcept`.\n- The shape is stored whether or not the pane has a legend.",
        see=["[`Pane::legend`](legend.md): turn the legend on.", "[`Pane::legend_frame`](legend_frame.md): the box's frame.", "[`Studio::legend_shape`](../studio/legend_shape.md): the same for the docked legend.", "[`KeyShape`](../types/KeyShape.md): the values it takes."],
    )
    write("legend_shape", page)


# ------------------------------------------------------------------ grid
def grid_page():
    base = """auto fig = studio("{n}");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{{0, 1, 2, 3, 4}}, std::vector<double>{{0, 2, 1, 3, 2}});"""
    ex = [
        dict(id="default", text="The default is `None`: no grid lines on any axis.",
             code=base.format(n="grid_default") + """
std::cout << "None: " << (pane.grid_style() == GridStyle::None) << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="major", text="`GridStyle::Major` draws a line at every major tick of every axis.",
             code=base.format(n="grid_major") + """
pane.grid(GridStyle::Major);
std::cout << "x major: " << pane.x_axis().major_grid << ", y major: " << pane.y_axis().major_grid << ", x minor: " << pane.x_axis().minor_grid << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="majorminor", text="`GridStyle::MajorMinor` adds a finer line between the major ones.",
             code=base.format(n="grid_majorminor") + """
pane.grid(GridStyle::MajorMinor);
std::cout << "x major: " << pane.x_axis().major_grid << ", x minor: " << pane.x_axis().minor_grid << ", y minor: " << pane.y_axis().minor_grid << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="three_styles", text="Three different figures. The grid is the only difference between them.",
             code="""auto build = [&](GridStyle style) {
  auto fig = studio("grid_compare");
  auto& pane = fig.pane2("p");
  pane.trend(std::vector<double>{0, 1, 2, 3, 4}, std::vector<double>{0, 2, 1, 3, 2});
  pane.grid(style);
  return fig.to_svg();
};
const auto none = build(GridStyle::None), major = build(GridStyle::Major), minor = build(GridStyle::MajorMinor);
std::cout << "none != major: " << (none != major) << ", major != major+minor: " << (major != minor) << "\\n";
auto fig = studio("grid_compare_out");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{0, 1, 2, 3, 4}, std::vector<double>{0, 2, 1, 3, 2});
pane.grid(GridStyle::MajorMinor);
fig.save_svg("figure.svg");"""),
        dict(id="overwrites", text="`grid` sets every axis the same way, so it overwrites what the per-axis calls set before it. The y axis had a major grid of its own; after `grid(GridStyle::None)` it has none.",
             code=base.format(n="grid_overwrite") + """
pane.y_major_grid(true);
std::cout << "before: y major " << pane.y_axis().major_grid << "\\n";
pane.grid(GridStyle::None);
std::cout << "after:  y major " << pane.y_axis().major_grid << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="summary", text="`grid_style()` summarises the per-axis grids: `MajorMinor` if any axis has a minor grid, else `Major` if any axis has a major grid, else `None`. One axis turned off does not lower it while another still has the line.",
             code=base.format(n="grid_summary") + """
pane.x_major_grid(true).y_major_grid(true);
std::cout << "two majors: " << static_cast<int>(pane.grid_style()) << " (Major = 1)\\n";
pane.x_major_grid(false);
std::cout << "x off, y still on: " << static_cast<int>(pane.grid_style()) << "\\n";
pane.y_minor_grid(true);
std::cout << "y minor added: " << static_cast<int>(pane.grid_style()) << " (MajorMinor = 2)\\n";
fig.save_svg("figure.svg");"""),
        dict(id="three_d", text="On a 3D pane the style covers all three axes, and the grid is drawn on the faces behind the data.",
             code="""auto fig = studio("grid_3d");
auto& pane = fig.pane3("p");
pane.path3(std::vector<double>{0, 1, 2, 1}, std::vector<double>{0, 1, 0, 2}, std::vector<double>{0, 1, 2, 3});
pane.grid(GridStyle::Major);
std::cout << "z major: " << pane.z_axis().major_grid << "\\n";
fig.save_svg("figure.svg");"""),
    ]
    page = dict(
        slug="grid",
        title="Pane::grid",
        signature="Pane& Pane::grid(GridStyle style) noexcept\nGridStyle Pane::grid_style() const noexcept",
        intro="""Sets the grid lines of every axis of the pane at once, and reads the overall style back.

| `GridStyle` | Drawing |
|---|---|
| `None` | No grid lines. The default. |
| `Major` | A line at every major tick, on every axis. |
| `MajorMinor` | Major lines, and a finer line between them. |

The style is a shorthand for the six per-axis switches ([`x_major_grid`](x_major_grid.md), [`x_minor_grid`](x_minor_grid.md) and the y and z ones): `grid` sets all six to match the style and so overwrites whatever they held. The reverse holds too: `grid_style()` is worked out from the switches, so it always describes what is drawn. It is `MajorMinor` if any axis has its minor grid on, otherwise `Major` if any axis has its major grid on, otherwise `None`.

Use `grid` for the same grid everywhere, and the per-axis switches for a grid on one axis only (horizontal lines on a bar chart, for instance). Call `grid` first and the switches afterwards; the other order loses the switches.""",
        parameters="""**style** (`GridStyle`, setter only)
    `None`, `Major` or `MajorMinor`, applied to all three axes.""",
        returns="""**Setter:** `Pane&`, the same pane.
**Getter:** `GridStyle`, the summary of the six switches.""",
        examples=ex,
        notes="- Both forms are `noexcept`.\n- The z switches are set on a planar pane too; a planar pane has no z axis, so they draw nothing.",
        see=["[`Pane::x_major_grid`](x_major_grid.md), [`x_minor_grid`](x_minor_grid.md): one axis at a time.", "[`GridStyle`](../types/GridStyle.md): the values it takes.", "[`Pane::grid_style` summary]: see Example 6."],
    )
    page["see"] = page["see"][:-1]
    write("grid", page)



# ------------------------------------------------------------------ kind, visibility
def kind_page():
    page = dict(
        slug="kind",
        title="Pane::kind",
        signature="PaneKind Pane::kind() const noexcept",
        intro="""Returns which kind of surface the pane is: `PaneKind::Planar`, made by [`Studio::pane2`](../studio/pane2.md), or `PaneKind::Volumetric`, made by [`Studio::pane3`](../studio/pane3.md). The kind is fixed when the pane is made and never changes.

The kind decides what can be added to the pane. The planar trace methods ([`trend`](trend.md), [`cloud`](cloud.md), [`columns`](columns.md), [`violin`](violin.md), [`contour`](contour.md), and the rest of the planar kinds) need a planar pane, and the volumetric ones ([`path3`](path3.md), [`cloud3`](cloud3.md), [`spire3`](spire3.md), [`sheet`](sheet.md), [`isoshell`](isoshell.md)...) need a volumetric one; calling the wrong family throws. The kind also decides the starting [`lens`](lens.md): a planar pane begins with the flat, straight-on view of `Lens::planar()`, a volumetric pane with the angled view of `Lens::spatial()`. Both can be changed afterwards.

Call `kind()` in code that receives panes it did not make (a helper that decorates every pane of a figure, say) to choose between the planar and volumetric calls.""",
        parameters="*(none)*",
        returns="""`PaneKind`
    `Planar` or `Volumetric`.""",
        examples=[
            dict(id="both", text="The two factories make the two kinds.",
                 code="""auto fig = studio("kind_both");
fig.mosaic(1, 2);
auto& flat = fig.pane2("flat");
flat.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
auto& deep = fig.pane3("deep");
deep.path3(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0}, std::vector<double>{0, 1, 2});
std::cout << "flat is Planar: " << (flat.kind() == PaneKind::Planar) << ", deep is Volumetric: " << (deep.kind() == PaneKind::Volumetric) << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="planar_refuses", text="A planar pane refuses the volumetric trace methods, naming the kind it needs.",
                 code="""auto fig = studio("kind_planar_refuses");
auto& pane = fig.pane2("p");
try {
  pane.path3(std::vector<double>{0, 1}, std::vector<double>{0, 1}, std::vector<double>{0, 1});
} catch (const pluss::Error& e) {
  std::cout << e.message() << "\\n";
}
pane.trend(std::vector<double>{0, 1}, std::vector<double>{0, 1});
fig.save_svg("figure.svg");"""),
            dict(id="volumetric_refuses", text="And a volumetric pane refuses the planar ones.",
                 code="""auto fig = studio("kind_volumetric_refuses");
auto& pane = fig.pane3("p");
try {
  pane.trend(std::vector<double>{0, 1}, std::vector<double>{0, 1});
} catch (const pluss::Error& e) {
  std::cout << e.message() << "\\n";
}
pane.path3(std::vector<double>{0, 1}, std::vector<double>{0, 1}, std::vector<double>{0, 1});
fig.save_svg("figure.svg");"""),
            dict(id="lens", text="The kind sets the starting view. A planar pane looks straight at the plane; a volumetric pane starts at an angle, so that depth shows.",
                 code="""auto fig = studio("kind_lens");
fig.mosaic(1, 2);
auto& flat = fig.pane2("flat");
flat.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
auto& deep = fig.pane3("deep");
deep.path3(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0}, std::vector<double>{0, 1, 2});
std::cout << "flat: azimuth " << flat.lens().azimuth_deg() << ", elevation " << flat.lens().elevation_deg() << "\\n";
std::cout << "deep: azimuth " << deep.lens().azimuth_deg() << ", elevation " << deep.lens().elevation_deg() << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="dispatch", text="Decorating panes of either kind. The helper labels every axis and then adds the z label only where there is a z axis.",
                 code="""auto fig = studio("kind_dispatch");
fig.mosaic(1, 2);
auto& flat = fig.pane2("flat");
flat.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
auto& deep = fig.pane3("deep");
deep.path3(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0}, std::vector<double>{0, 1, 2});
for (auto* pane : {&flat, &deep}) {
  pane->x_label("x").y_label("y");
  if (pane->kind() == PaneKind::Volumetric) pane->z_label("z");
}
std::cout << "flat z label: '" << flat.z_axis().label << "', deep z label: '" << deep.z_axis().label << "'\\n";
fig.save_svg("figure.svg");"""),
        ],
        notes="- `noexcept`, and has no setter.\n- The planar and volumetric trace methods are listed on their own pages; each names the kind it needs in the error it throws.",
        see=["[`Studio::pane2`](../studio/pane2.md), [`Studio::pane3`](../studio/pane3.md): make a pane.", "[`Pane::lens`](lens.md): the view, whose starting value depends on the kind."],
    )
    write("kind", page)


def visible_page():
    page = dict(
        slug="visible",
        title="Pane::visible",
        signature="Pane& Pane::visible(bool enable) noexcept\nbool Pane::visible() const noexcept\nPane& Pane::hide() noexcept\nPane& Pane::show() noexcept",
        intro="""Controls whether the pane is drawn at all. A hidden pane draws nothing: no title, no axes, no traces, no legend entries. It keeps its cell in a [`mosaic`](../studio/mosaic.md), so the other panes stay where they are and the cell is left empty. Its data and settings are kept, and showing it again restores it exactly.

`hide()` is `visible(false)` and `show()` is `visible(true)`. The getter takes no argument. The setter requires one, so `pane.visible()` always reads and never changes anything.

Use it to compare figures with and without a pane, or to switch panes by a condition without rebuilding the figure. To keep a pane but remove only its axes, use [`axes_visible`](axes_visible.md); to remove one trace, use [`Trace::visible`](../trace/visible.md).""",
        parameters="""**enable** (`bool`, setter only)
    `true` to draw the pane, `false` to hide it.""",
        returns="""**Setters:** `Pane&`, the same pane.
**Getter:** `bool`, whether the pane is drawn. A new pane is visible.""",
        examples=[
            dict(id="default", text="A new pane is visible, and reading the state with no argument changes nothing.",
                 code="""auto fig = studio("visible_default");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
pane.hide();
std::cout << "after hide: " << pane.visible() << "\\n";
std::cout << "reading again: " << pane.visible() << "\\n";
pane.show();
std::cout << "after show: " << pane.visible() << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="spellings", text="`hide()` and `visible(false)` are the same, and so are `show()` and `visible(true)`.",
                 code="""auto build = [&](int how) {
  auto fig = studio("visible_spellings");
  auto& pane = fig.pane2("p");
  pane.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
  if (how == 0) pane.hide();
  if (how == 1) pane.visible(false);
  return fig.to_svg();
};
std::cout << "hide() == visible(false): " << (build(0) == build(1)) << "\\n";
auto fig = studio("visible_spellings_out");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
pane.visible(false).visible(true);
fig.save_svg("figure.svg");"""),
            dict(id="draws_nothing", text="A hidden pane draws nothing. Its title is in the figure while it is shown and gone while it is hidden, and showing it again gives back the original figure.",
                 code="""auto fig = studio("visible_draws");
auto& pane = fig.pane2("alpha");
pane.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
const std::string shown = fig.to_svg();
pane.hide();
const std::string hidden = fig.to_svg();
pane.show();
std::cout << "title while shown: " << (shown.find(">alpha<") != std::string::npos) << ", while hidden: " << (hidden.find(">alpha<") != std::string::npos) << "\\n";
std::cout << "shown again == shown before: " << (fig.to_svg() == shown) << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="mosaic", text="The cell stays. Of two panes side by side, the left is hidden: the right one is still in the right half of the figure and the left half is empty.",
                 code="""auto fig = studio("visible_mosaic");
fig.mosaic(1, 2);
auto& left = fig.pane2("left");
left.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
auto& right = fig.pane2("right");
right.trend(std::vector<double>{0, 1, 2}, std::vector<double>{1, 0, 1});
%s
const std::string before = fig.to_svg();
left.hide();
const std::string after = fig.to_svg();
std::cout << "right title before " << at(before, "right") << ", after " << at(after, "right") << "\\n";
fig.save_svg("figure.svg");""" % AT),
            dict(id="condition", text="Switching by a condition. The same figure is built for two modes; the second pane is shown only in the detailed one.",
                 code="""auto build = [&](bool detailed) {
  auto fig = studio("visible_condition");
  fig.mosaic(1, 2);
  auto& overview = fig.pane2("overview");
  overview.trend(std::vector<double>{0, 1, 2, 3}, std::vector<double>{0, 2, 1, 3});
  auto& detail = fig.pane2("detail");
  detail.trend(std::vector<double>{0, 1, 2, 3}, std::vector<double>{1, 3, 2, 4});
  detail.visible(detailed);
  return fig.to_svg();
};
std::cout << "detail drawn: " << (build(true).find(">detail<") != std::string::npos) << ", " << (build(false).find(">detail<") != std::string::npos) << "\\n";
auto fig = studio("visible_condition_out");
fig.mosaic(1, 2);
auto& overview = fig.pane2("overview");
overview.trend(std::vector<double>{0, 1, 2, 3}, std::vector<double>{0, 2, 1, 3});
auto& detail = fig.pane2("detail");
detail.trend(std::vector<double>{0, 1, 2, 3}, std::vector<double>{1, 3, 2, 4});
detail.hide();
fig.save_svg("figure.svg");"""),
        ],
        notes="- All four are `noexcept`.\n- Hiding a pane does not remove its data or settings, and does not remove it from [`Studio::panes`](../studio/panes.md).\n- A hidden pane gets no share of a group caption (see [`align_x`](align_x.md)).",
        see=["[`Pane::axes_visible`](axes_visible.md): hide only the axes.", "[`Trace::visible`](../trace/visible.md): hide one trace.", "[`Studio::mosaic`](../studio/mosaic.md): the cells a hidden pane keeps."],
    )
    write("visible", page)


def axes_visible_page():
    base = """auto fig = studio("{n}");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{{0, 1, 2}}, std::vector<double>{{0, 1, 0}});"""
    page = dict(
        slug="axes_visible",
        title="Pane::axes_visible",
        signature="Pane& Pane::axes_visible(bool enable) noexcept\nbool Pane::axes_visible() const noexcept",
        intro="""Controls whether the pane draws its axis lines and tick marks. With `false` the data, the grid, the tick labels and the axis labels are still drawn; only the lines of the axes and the marks on them go. The pane keeps its place, and the plot area becomes the room for the data. A new pane has its axes visible.

For a figure with no axis furniture at all (a sparkline, an inset, a picture-like plot) turn off the axes and the tick labels together with [`x_tick_labels`](x_tick_labels.md) and [`y_tick_labels`](y_tick_labels.md); a pane left with nothing around the plot uses the narrow margins of a bare plot, and fits more of the figure to the data. To remove the whole pane use [`visible`](visible.md).

The setter requires its argument, so `pane.axes_visible()` always reads.""",
        parameters="""**enable** (`bool`, setter only)
    `true` to draw axis lines and ticks, `false` to leave them out.""",
        returns="""**Setter:** `Pane&`, the same pane.
**Getter:** `bool`, whether the axes are drawn.""",
        examples=[
            dict(id="default", text="The default is visible; the setter changes the state and the getter reads it.",
                 code=base.format(n="axes_default") + """
std::cout << "default: " << pane.axes_visible() << "\\n";
pane.axes_visible(false);
std::cout << "after false: " << pane.axes_visible() << "\\n";
pane.axes_visible(true);
fig.save_svg("figure.svg");"""),
            dict(id="hidden", text="Without the axes. The line of the axes and the tick marks are gone and the tick labels stay, so the figure is different but the same numbers are written on it.",
                 code=base.format(n="axes_hidden") + """
const std::string with_axes = fig.to_svg();
pane.axes_visible(false);
const std::string without = fig.to_svg();
std::cout << "figures differ: " << (with_axes != without) << "\\n";
std::cout << "tick label 1.0 still drawn: " << (without.find(">1.0<") != std::string::npos) << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="bare", text="A bare plot. With the axes and the tick labels all turned off and no axis label, nothing is left around the plot, and the plot takes narrow margins: the data fills more of the figure.",
                 code="""auto build = [&](bool bare) {
  auto fig = studio("axes_bare");
  auto& pane = fig.pane2("p");
  pane.trend(std::vector<double>{0, 1, 2, 3}, std::vector<double>{0, 2, 1, 3});
  if (bare) pane.axes_visible(false).x_tick_labels(false).y_tick_labels(false);
  return fig.to_svg();
};
%s
const auto framed = build(false), bare = build(true);
std::cout << "bare figure has no tick text: " << (texts(bare).find("0.5") == std::string::npos) << "\\n";
auto fig = studio("axes_bare_out");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{0, 1, 2, 3}, std::vector<double>{0, 2, 1, 3});
pane.axes_visible(false).x_tick_labels(false).y_tick_labels(false);
fig.save_svg("figure.svg");""" % TEXTS),
            dict(id="three_d", text="On a volumetric pane the same switch removes the box of axes around the data.",
                 code="""auto build = [&](bool visible) {
  auto fig = studio("axes_3d");
  auto& pane = fig.pane3("p");
  pane.path3(std::vector<double>{0, 1, 2, 1}, std::vector<double>{0, 1, 0, 2}, std::vector<double>{0, 1, 2, 3});
  pane.axes_visible(visible);
  return fig.to_svg();
};
std::cout << "differ: " << (build(true) != build(false)) << "\\n";
auto fig = studio("axes_3d_out");
auto& pane = fig.pane3("p");
pane.path3(std::vector<double>{0, 1, 2, 1}, std::vector<double>{0, 1, 0, 2}, std::vector<double>{0, 1, 2, 3});
pane.axes_visible(false);
fig.save_svg("figure.svg");"""),
            dict(id="not_visible", text="Not the same as hiding the pane. With the axes off the traces are drawn; with the pane hidden nothing is.",
                 code="""auto fig = studio("axes_vs_visible");
fig.mosaic(1, 2);
auto& axes_off = fig.pane2("axes off");
axes_off.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
axes_off.axes_visible(false);
auto& hidden = fig.pane2("hidden");
hidden.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
hidden.hide();
const std::string svg = fig.to_svg();
std::cout << "axes-off title drawn: " << (svg.find(">axes off<") != std::string::npos) << ", hidden title drawn: " << (svg.find(">hidden<") != std::string::npos) << "\\n";
fig.save_svg("figure.svg");"""),
        ],
        notes="- Both forms are `noexcept`.\n- The axes' labels ([`x_label`](x_label.md)) and the tick labels are separate settings and stay as they are.",
        see=["[`Pane::visible`](visible.md): hide the whole pane.", "[`Pane::x_tick_labels`](x_tick_labels.md), [`y_tick_labels`](y_tick_labels.md): the tick labels.", "[`Pane::grid`](grid.md): the grid lines."],
    )
    write("axes_visible", page)



# ------------------------------------------------------------------ axis label placement
P3 = """auto fig = studio("{n}");
auto& pane = fig.pane3("p");
pane.path3(std::vector<double>{{0, 1, 2, 1}}, std::vector<double>{{0, 1, 0, 2}}, std::vector<double>{{0, 1, 2, 3}});
pane.x_label("xx").y_label("yy");"""


def parallel_page():
    page = dict(
        slug="parallel_xy_labels",
        title="Pane::parallel_xy_labels",
        signature="Pane& Pane::parallel_xy_labels(bool enable) noexcept\nbool Pane::parallel_xy_labels() const noexcept",
        intro="""On a volumetric pane, chooses how the x and y axis labels sit against their axes. With `true`, the default, each label is turned to run along its axis edge as the view shows it, and set off from the edge; with `false` the labels stay upright, below and to the left of their edges. Planar panes have flat axes, so the setting changes nothing there.

An explicit rotation ([`x_label_rotation`](x_label_rotation.md), [`y_label_rotation`](y_label_rotation.md)) always wins: a label with a rotation of its own is drawn at that angle whatever this switch says. The z label is not affected; it always follows its own axis.

The setter requires its argument, so `pane.parallel_xy_labels()` always reads.""",
        parameters="""**enable** (`bool`, setter only)
    `true` for labels that run along their axes, `false` for upright labels.""",
        returns="""**Setter:** `Pane&`, the same pane.
**Getter:** `bool`, the setting. `true` for a new pane.""",
        examples=[
            dict(id="default", text="The default is on. The labels of a volumetric pane run along their axes: each is rotated by the angle of its edge in the view, so neither is upright.",
                 code=P3.format(n="parallel_default") + "\n" + LOOK + """
const std::string svg = fig.to_svg();
std::cout << "default: " << pane.parallel_xy_labels() << "\\n";
std::cout << "x label: " << look(svg, "xx") << "\\n";
std::cout << "y label: " << look(svg, "yy") << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="off", text="Turned off, the labels are upright.",
                 code=P3.format(n="parallel_off") + "\n" + LOOK + """
pane.parallel_xy_labels(false);
const std::string svg = fig.to_svg();
std::cout << "now: " << pane.parallel_xy_labels() << "\\n";
std::cout << "x label: " << look(svg, "xx") << "\\n";
std::cout << "y label: " << look(svg, "yy") << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="view", text="The angle follows the view. Turning the pane by 60 degrees changes the angle each label is drawn at, because the edges point elsewhere in the picture.",
                 code=P3.format(n="parallel_view") + "\n" + LOOK + """
const std::string before = fig.to_svg();
pane.lens().look(60.0, 25.0);
const std::string after = fig.to_svg();
std::cout << "x label at the default view: " << look(before, "xx") << "\\n";
std::cout << "x label after look(60, 25):  " << look(after, "xx") << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="explicit", text="An explicit rotation wins. With `x_label_rotation(0)` the x label is upright although the switch is on; the y label, which has none, still runs along its edge.",
                 code=P3.format(n="parallel_explicit") + "\n" + LOOK + """
pane.x_label_rotation(0.0);
const std::string svg = fig.to_svg();
std::cout << "x label: " << look(svg, "xx") << "\\n";
std::cout << "y label: " << look(svg, "yy") << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="no_labels", text="Without labels there is nothing to place, and the switch changes nothing; the same holds for a planar pane.",
                 code="""auto build3 = [&](bool on) {
  auto fig = studio("parallel_nolabels");
  auto& pane = fig.pane3("p");
  pane.path3(std::vector<double>{0, 1, 2, 1}, std::vector<double>{0, 1, 0, 2}, std::vector<double>{0, 1, 2, 3});
  pane.parallel_xy_labels(on);
  return fig.to_svg();
};
auto build2 = [&](bool on) {
  auto fig = studio("parallel_planar");
  auto& pane = fig.pane2("p");
  pane.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
  pane.x_label("x").y_label("y");
  pane.parallel_xy_labels(on);
  return fig.to_svg();
};
std::cout << "3D without labels unchanged: " << (build3(true) == build3(false)) << ", planar unchanged: " << (build2(true) == build2(false)) << "\\n";
auto fig = studio("parallel_nolabels_out");
auto& pane = fig.pane3("p");
pane.path3(std::vector<double>{0, 1, 2, 1}, std::vector<double>{0, 1, 0, 2}, std::vector<double>{0, 1, 2, 3});
fig.save_svg("figure.svg");"""),
        ],
        notes="- Both forms are `noexcept`.\n- The ticks' own rotation is separate: see [`x_tick_rotation`](x_tick_rotation.md).",
        see=["[`Pane::x_label_rotation`](x_label_rotation.md), [`y_label_rotation`](y_label_rotation.md): rotate a label by hand, which overrides this switch.", "[`Pane::lens`](lens.md): the view that decides the angles."],
    )
    write("parallel_xy_labels", page)


def y_centered_page():
    base = """auto fig = studio("{n}");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{{0, 1, 2}}, std::vector<double>{{0, 1, 0}});
pane.y_label("measured value");"""
    page = dict(
        slug="y_label_centered",
        title="Pane::y_label_centered",
        signature="Pane& Pane::y_label_centered(bool enable = true) noexcept",
        intro="""Chooses where the y label stands. By default (`false`) it is written upright, at the top of the axis, ending at the axis: it reads like a heading for the axis and takes no sideways room beyond its own width. With `true` it is turned to run vertically and is centred on the axis, along its length, outside the tick labels.

The two suit different figures. The upright label reads easily and keeps a long label legible; the centred one takes less width and is the usual convention for a tall, narrow plot. Either way the pane keeps room for the label, so it is never cut by the figure's edge. Only the y axis has the choice; the x label is always centred below its axis.

An explicit [`y_label_rotation`](y_label_rotation.md) is applied on top of either placement.""",
        parameters="""**enable** (`bool`, default `true`)
    `true` to turn the label vertical and centre it on the axis; `false` for the upright label at the top.""",
        returns="""`Pane&`
    The same pane, for chaining.""",
        examples=[
            dict(id="default", text="By default the label is upright, with its end at the axis.",
                 code=base.format(n="ycentered_default") + "\n" + LOOK + """
std::cout << look(fig.to_svg(), "measured value") << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="centered", text="Centred, it is turned to run up the axis, middle-anchored on the axis.",
                 code=base.format(n="ycentered_on") + "\n" + LOOK + """
pane.y_label_centered();
std::cout << look(fig.to_svg(), "measured value") << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="off", text="`y_label_centered(false)` is the default again: the figure is the one that never made the call.",
                 code="""auto build = [&](int mode) {
  auto fig = studio("ycentered_off");
  auto& pane = fig.pane2("p");
  pane.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
  pane.y_label("measured value");
  if (mode == 1) pane.y_label_centered(true);
  if (mode == 2) pane.y_label_centered(true).y_label_centered(false);
  return fig.to_svg();
};
std::cout << "off again == never set: " << (build(2) == build(0)) << ", on != off: " << (build(1) != build(0)) << "\\n";
auto fig = studio("ycentered_off_out");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
pane.y_label("measured value");
fig.save_svg("figure.svg");"""),
            dict(id="long", text="A long label stays inside the figure in both placements: the pane makes room for the upright label at its left, and the centred label sits beside the axis.",
                 code="""auto fig = studio("ycentered_long");
fig.mosaic(1, 2);
auto& upright = fig.pane2("upright");
upright.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
upright.y_label("a rather long measured value in arbitrary units");
auto& centered = fig.pane2("centered");
centered.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
centered.y_label("a rather long measured value in arbitrary units").y_label_centered();
fig.save_svg("figure.svg");"""),
            dict(id="rotation", text="With an explicit rotation the label is turned by that angle, and the placement decides where it is anchored.",
                 code=base.format(n="ycentered_rotation") + "\n" + LOOK + """
pane.y_label_centered().y_label_rotation(-45.0);
std::cout << look(fig.to_svg(), "measured value") << "\\n";
fig.save_svg("figure.svg");"""),
        ],
        notes="- `noexcept`. Only the y axis has this switch.\n- The call has no effect on a pane without a y label.",
        see=["[`Pane::y_label`](y_label.md): the label.", "[`Pane::y_label_rotation`](y_label_rotation.md): rotate it by hand.", "[`Pane::y_label_gap`](y_label_gap.md): the distance from the tick labels."],
    )
    write("y_label_centered", page)


# ------------------------------------------------------------------ colour scale
class Tpl(str):
    def format(self, **kw):
        out = str(self)
        for k, v in kw.items():
            out = out.replace('{' + k + '}', v)
        return out


CLOUD = """pane.cloud(std::vector<double>{1, 2, 3, 4}, std::vector<double>{1, 2, 3, 4}, std::vector<double>{5, 5, 5, 5}, std::vector<double>{0.13, 0.4, 0.62, 0.97});"""
FIELD = """pane.fieldmap(std::vector<double>{1, 2, 3}, std::vector<double>{1, 2, 3}, pluss::series::linspace(0.0, 1.0, 9, pluss::ScalarType::Float64).reshape({3, 3}));"""


def guide_ticks(call):
    return TEXTS


def scale_guide_page():
    base = Tpl("""auto fig = studio("{n}");
auto& pane = fig.pane2("p");
""" + CLOUD)
    page = dict(
        slug="scale_guide",
        title="Pane::scale_guide",
        signature="Pane& Pane::scale_guide(bool enable = true) noexcept\nbool Pane::scale_guide_enabled() const noexcept",
        intro="""Turns the pane's **scale guide** on or off: the colour bar drawn beside the plot that shows which values the colours stand for, with the palette as a gradient and ticks along it. It is the key to a colour scale, as [`legend`](legend.md) is the key to labelled traces.

The guide describes the colours of the traces whose colour comes from data: a [`cloud`](cloud.md) given a colour series, a [`fieldmap`](fieldmap.md), a [`contour`](contour.md) and the volumetric surfaces. Its range is the range the colours are mapped over: the data's own, or the one set by [`color_limits`](color_limits.md) or [`color_limit_method`](color_limit_method.md), or by a trace's own limits ([`Trace::color_limits`](../trace/color_limits.md)). When no visible trace has a colour scale there is nothing to describe: no guide is drawn and no room is kept for one, whatever the switch says.

The guide is off by default. Its setter and its getter have different names.""",
        parameters="""**enable** (`bool`, default `true`, setter only)
    `true` to draw the guide, `false` to leave it out.""",
        returns="""**Setter (`scale_guide`):** `Pane&`, the same pane.
**Getter (`scale_guide_enabled`):** `bool`, the switch.""",
        examples=[
            dict(id="default", text="Off by default. The colours of the cloud are drawn, with no bar to say what they mean.",
                 code=base.format(n="guide_default") + """
std::cout << "enabled: " << pane.scale_guide_enabled() << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="on", text="With the guide on, the bar is drawn beside the plot, and its ticks are written with the other texts of the figure.",
                 code=base.format(n="guide_on") + "\n" + TEXTS + """
const std::string off = texts(fig.to_svg());
pane.scale_guide();
std::cout << "enabled: " << pane.scale_guide_enabled() << "\\n";
std::cout << "off: " << off << "\\n";
std::cout << "on:  " << texts(fig.to_svg()) << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="fieldmap", text="A field map is coloured by its values, so it has a guide too: here the values run from 0 to 1.",
                 code="""auto fig = studio("guide_fieldmap");
auto& pane = fig.pane2("p");
""" + FIELD + """
pane.scale_guide(true);
fig.save_svg("figure.svg");"""),
            dict(id="follows_limits", text="The guide shows the range the colours are mapped over. With the limits set to 0..10, the same data takes the low end of the palette and the bar's ticks run to 10.",
                 code=base.format(n="guide_limits") + "\n" + TEXTS + """
pane.scale_guide(true);
std::cout << "data range:   " << texts(fig.to_svg()) << "\\n";
pane.color_limits(Range1d{0.0, 10.0});
std::cout << "limits 0..10: " << texts(fig.to_svg()) << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="nothing_to_show", text="A trace with no colour scale has no guide. The switch is stored, the figure is unchanged, and no room is kept beside the plot.",
                 code="""auto build = [&](bool on) {
  auto fig = studio("guide_trend");
  auto& pane = fig.pane2("p");
  pane.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
  pane.scale_guide(on);
  return fig.to_svg();
};
std::cout << "same figure with the guide on: " << (build(true) == build(false)) << "\\n";
auto fig = studio("guide_trend_out");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
pane.scale_guide(true);
std::cout << "enabled: " << pane.scale_guide_enabled() << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="three_d", text="On a volumetric pane, a coloured surface has its guide beside the picture.",
                 code="""auto fig = studio("guide_3d");
auto& pane = fig.pane3("p");
pane.spire3(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 2}, pluss::series::linspace(0.0, 4.0, 9, pluss::ScalarType::Float64).reshape({3, 3}));
pane.scale_guide(true);
fig.save_svg("figure.svg");"""),
        ],
        notes="- Both forms are `noexcept`.\n- The room for the guide is kept only when it is drawn.",
        see=["[`Pane::color_limits`](color_limits.md), [`color_limit_method`](color_limit_method.md): the range the guide shows.", "[`Trace::palette`](../trace/palette.md): the colours.", "[`Pane::legend`](legend.md): the key to labelled traces."],
    )
    write("scale_guide", page)


def color_limits_page():
    base = Tpl("""auto fig = studio("{n}");
auto& pane = fig.pane2("p");
""" + CLOUD + """
pane.scale_guide(true);""")
    page = dict(
        slug="color_limits",
        title="Pane::color_limits",
        signature="Pane& Pane::color_limits(Range1d limits)\nPane& Pane::auto_color() noexcept",
        intro="""Sets by hand the range of values the pane's colours are mapped over, so that the lowest colour of the palette stands for `limits.low` and the highest for `limits.high`; values outside the range take the end colours. `auto_color()` goes back to deriving the range from the data (by [`color_limit_method`](color_limit_method.md)).

Without limits each trace spreads its palette over its own data, so two panes of different data use their colours differently. Set the same limits on both (or on one pane's traces, see below) when colours must mean the same value across figures, or when the range should be a round number instead of the data's extremes.

The range applies to every trace of the pane that is coloured by data: clouds, field maps, contours and surfaces, planar and volumetric. A trace can carry limits of its own ([`Trace::color_limits`](../trace/color_limits.md)), and those take precedence over the pane's for that trace. The range must be finite with `low` below `high`; otherwise the call throws and the pane is unchanged.""",
        parameters="""**limits** (`Range1d`)
    The values of the lowest and the highest colour. `low` must be strictly below `high`, both finite.""",
        returns="""`Pane&`
    The same pane, for chaining. `auto_color()` is `noexcept`.""",
        examples=[
            dict(id="manual", text="Setting the range. The data runs from 0.13 to 0.97; with limits 0..10 the guide's ticks run to 10, and the points, all in the lower tenth of the range, are drawn in the low colours of the palette.",
                 code=base.format(n="color_limits_manual") + "\n" + TEXTS + """
std::cout << "from the data: " << texts(fig.to_svg()) << "\\n";
pane.color_limits(Range1d{0.0, 10.0});
std::cout << "limits 0..10:  " << texts(fig.to_svg()) << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="auto", text="`auto_color()` returns to the data's range; the figure is the one that never set limits.",
                 code="""auto build = [&](bool toggled) {
  auto fig = studio("color_limits_auto");
  auto& pane = fig.pane2("p");
  """ + CLOUD + """
  pane.scale_guide(true);
  if (toggled) pane.color_limits(Range1d{0.0, 10.0}).auto_color();
  return fig.to_svg();
};
std::cout << "after auto_color == never set: " << (build(true) == build(false)) << "\\n";
auto fig = studio("color_limits_auto_out");
auto& pane = fig.pane2("p");
""" + CLOUD + """
pane.scale_guide(true);
fig.save_svg("figure.svg");"""),
            dict(id="two_panes", text="The same limits on two panes make their colours comparable. Both panes use 0..1, so the colours of the second pane, whose data is only in the upper half, are the upper colours of the palette.",
                 code="""auto fig = studio("color_limits_two");
fig.mosaic(1, 2);
for (int i = 0; i < 2; ++i) {
  auto& pane = fig.pane2(i == 0 ? "full range" : "upper half");
  const double lo = i == 0 ? 0.0 : 0.5;
  pane.cloud(std::vector<double>{1, 2, 3, 4}, std::vector<double>{1, 2, 3, 4}, std::vector<double>{5, 5, 5, 5}, std::vector<double>{lo, lo + 0.1, lo + 0.3, lo + 0.5});
  pane.color_limits(Range1d{0.0, 1.0}).scale_guide(true);
}
fig.save_svg("figure.svg");"""),
            dict(id="surfaces", text="Surfaces and field maps follow the same limits: this field map's values run from 0 to 1, and limits 0..4 make the whole map use the lower quarter of the palette.",
                 code="""auto build = [&](bool limited) {
  auto fig = studio("color_limits_field");
  auto& pane = fig.pane2("p");
  """ + FIELD + """
  pane.scale_guide(true);
  if (limited) pane.color_limits(Range1d{0.0, 4.0});
  return fig.to_svg();
};
std::cout << "limits change the figure: " << (build(true) != build(false)) << "\\n";
auto fig = studio("color_limits_field_out");
auto& pane = fig.pane2("p");
""" + FIELD + """
pane.scale_guide(true).color_limits(Range1d{0.0, 4.0});
fig.save_svg("figure.svg");"""),
            dict(id="trace_wins", text="A trace's own limits win over the pane's. The pane sets 0..10, the trace 0..1: the figure is the one with the trace's limits alone.",
                 code="""auto build = [&](bool pane_limits) {
  auto fig = studio("color_limits_trace");
  auto& pane = fig.pane2("p");
  pane.cloud(std::vector<double>{1, 2, 3, 4}, std::vector<double>{1, 2, 3, 4}, std::vector<double>{5, 5, 5, 5}, std::vector<double>{0.13, 0.4, 0.62, 0.97}).color_limits(Range1d{0.0, 1.0});
  if (pane_limits) pane.color_limits(Range1d{0.0, 10.0});
  return fig.to_svg();
};
std::cout << "pane limits ignored for that trace: " << (build(true) == build(false)) << "\\n";
auto fig = studio("color_limits_trace_out");
auto& pane = fig.pane2("p");
pane.cloud(std::vector<double>{1, 2, 3, 4}, std::vector<double>{1, 2, 3, 4}, std::vector<double>{5, 5, 5, 5}, std::vector<double>{0.13, 0.4, 0.62, 0.97}).color_limits(Range1d{0.0, 1.0});
pane.color_limits(Range1d{0.0, 10.0});
fig.save_svg("figure.svg");"""),
            dict(id="errors", text="Invalid ranges throw and leave the limits as they were: `low` must be strictly below `high`, and both must be finite.",
                 code="""auto fig = studio("color_limits_errors");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{0, 1}, std::vector<double>{0, 1});
try { pane.color_limits(Range1d{2.0, 1.0}); } catch (const pluss::Error& e) { std::cout << e.message() << "\\n"; }
try { pane.color_limits(Range1d{1.0, 1.0}); } catch (const pluss::Error& e) { std::cout << e.message() << "\\n"; }
try { pane.color_limits(Range1d{0.0, std::numeric_limits<double>::infinity()}); } catch (const pluss::Error& e) { std::cout << e.message() << "\\n"; }
std::cout << "limits still unset: " << !pane.color_axis().limits.has_value() << "\\n";
fig.save_svg("figure.svg");"""),
        ],
        notes="- `color_limits` throws a `pluss::Error` for an invalid range; `auto_color()` is `noexcept`.\n- The limits do not change which values a trace holds, only how they are coloured. Values outside the range are drawn in the end colours.",
        see=["[`Pane::color_limit_method`](color_limit_method.md): how the range is derived when it is not set by hand.", "[`Pane::scale_guide`](scale_guide.md): the bar that shows the range.", "[`Trace::color_limits`](../trace/color_limits.md): limits of one trace.", "[`Trace::palette`](../trace/palette.md): the colours."],
    )
    write("color_limits", page)


def color_limit_method_page():
    base = """auto build = [&](LimitMethod method) {
  auto fig = studio("color_method");
  auto& pane = fig.pane2("p");
  """ + CLOUD + """
  pane.scale_guide(true);
  pane.color_limit_method(method);
  return fig.to_svg();
};"""
    page = dict(
        slug="color_limit_method",
        title="Pane::color_limit_method",
        signature="Pane& Pane::color_limit_method(LimitMethod method) noexcept",
        intro="""Chooses how the range of the colour scale is derived from the data when it is not set by hand ([`color_limits`](color_limits.md) overrides the method). It is the colour-scale counterpart of [`x_limit_method`](x_limit_method.md).

| `LimitMethod` | Range |
|---|---|
| `Tight` | Exactly the extremes of the data: the lowest value gets the lowest colour and the highest value the highest colour. The default. |
| `TickAligned` | The extremes widened outwards to round numbers, the ones a bar's ticks would fall on. |
| `Padded` | The extremes widened by a small share of the range on each side, so no value sits at the very end of the palette. |

When all values are equal the range is widened by 0.5 each way whatever the method, so that the palette has some width. A trace's own limits ([`Trace::color_limits`](../trace/color_limits.md)) take precedence over everything here.""",
        parameters="""**method** (`LimitMethod`)
    `Tight`, `TickAligned` or `Padded`.""",
        returns="""`Pane&`
    The same pane, for chaining.""",
        examples=[
            dict(id="default", text="The default is `Tight`: the data runs from 0.13 to 0.97, and the guide's ticks fall inside that range.",
                 code=base + "\n" + TEXTS + """
std::cout << "tight: " << texts(build(LimitMethod::Tight)) << "\\n";
auto fig = studio("color_method_default");
auto& pane = fig.pane2("p");
""" + CLOUD + """
pane.scale_guide(true);
std::cout << "default is Tight: " << (pane.color_axis().limit_method == LimitMethod::Tight) << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="tick_aligned", text="`TickAligned` widens the range to round numbers: the same data now spans 0 to 1, and the bar's ticks begin at 0.",
                 code=base + "\n" + TEXTS + """
std::cout << "tick-aligned: " << texts(build(LimitMethod::TickAligned)) << "\\n";
auto fig = studio("color_method_tick");
auto& pane = fig.pane2("p");
""" + CLOUD + """
pane.scale_guide(true).color_limit_method(LimitMethod::TickAligned);
fig.save_svg("figure.svg");"""),
            dict(id="padded", text="`Padded` leaves a margin at both ends of the palette, so the extreme values are not drawn in the extreme colours.",
                 code=base + "\n" + TEXTS + """
std::cout << "padded: " << texts(build(LimitMethod::Padded)) << "\\n";
auto fig = studio("color_method_padded");
auto& pane = fig.pane2("p");
""" + CLOUD + """
pane.scale_guide(true).color_limit_method(LimitMethod::Padded);
fig.save_svg("figure.svg");"""),
            dict(id="three_differ", text="The three methods give three different figures.",
                 code=base + """
const auto tight = build(LimitMethod::Tight), aligned = build(LimitMethod::TickAligned), padded = build(LimitMethod::Padded);
std::cout << "tight != aligned: " << (tight != aligned) << ", tight != padded: " << (tight != padded) << ", aligned != padded: " << (aligned != padded) << "\\n";
auto fig = studio("color_method_three");
auto& pane = fig.pane2("p");
""" + CLOUD + """
fig.save_svg("figure.svg");"""),
            dict(id="manual_wins", text="Limits set by hand take over. With `color_limits(0, 10)` set, the method makes no difference.",
                 code="""auto build = [&](LimitMethod method) {
  auto fig = studio("color_method_manual");
  auto& pane = fig.pane2("p");
  """ + CLOUD + """
  pane.scale_guide(true).color_limits(Range1d{0.0, 10.0});
  pane.color_limit_method(method);
  return fig.to_svg();
};
std::cout << "Tight == Padded with limits set: " << (build(LimitMethod::Tight) == build(LimitMethod::Padded)) << "\\n";
auto fig = studio("color_method_manual_out");
auto& pane = fig.pane2("p");
""" + CLOUD + """
pane.color_limits(Range1d{0.0, 10.0});
fig.save_svg("figure.svg");"""),
            dict(id="surface", text="Surfaces use it too: the method decides the range of a field map's colours in the same way.",
                 code="""auto build = [&](LimitMethod method) {
  auto fig = studio("color_method_field");
  auto& pane = fig.pane2("p");
  pane.fieldmap(std::vector<double>{1, 2, 3}, std::vector<double>{1, 2, 3}, pluss::series::linspace(0.13, 0.97, 9, pluss::ScalarType::Float64).reshape({3, 3}));
  pane.scale_guide(true).color_limit_method(method);
  return fig.to_svg();
};
std::cout << "Tight != TickAligned: " << (build(LimitMethod::Tight) != build(LimitMethod::TickAligned)) << "\\n";
auto fig = studio("color_method_field_out");
auto& pane = fig.pane2("p");
pane.fieldmap(std::vector<double>{1, 2, 3}, std::vector<double>{1, 2, 3}, pluss::series::linspace(0.13, 0.97, 9, pluss::ScalarType::Float64).reshape({3, 3}));
pane.scale_guide(true).color_limit_method(LimitMethod::TickAligned);
fig.save_svg("figure.svg");"""),
        ],
        notes="- `noexcept`; every value is valid.\n- The default for the colour scale is `Tight`, unlike the position axes, which default to `TickAligned`: colours are expected to span the data.",
        see=["[`Pane::color_limits`](color_limits.md): set the range by hand, overriding the method.", "[`Pane::x_limit_method`](x_limit_method.md): the same choice for a position axis.", "[`Pane::scale_guide`](scale_guide.md): the bar that shows the range."],
    )
    write("color_limit_method", page)


# ------------------------------------------------------------------ fonts, title, caption
def font_scale_page(slug, what, field, default_pt, default_text, sample, examples_extra=""):
    pass


def tick_font_page():
    base = """auto fig = studio("{n}");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{{0, 1, 2}}, std::vector<double>{{0, 1, 0}});
pane.x_label("time").title("scaled");"""
    page = dict(
        slug="tick_font_scale",
        title="Pane::tick_font_scale",
        signature="Pane& Pane::tick_font_scale(double scale)\ndouble Pane::tick_font_scale() const noexcept",
        intro="""Scales the font of the tick labels of the pane, on all its axes at once. The scale is a factor on the default tick size: 1 is the default, 1.5 makes the labels half as large again, 0.75 three quarters. The layout follows: the margins grow with the labels, so larger labels are not cut off and the plot area gives way to them.

The scale must be finite and positive; anything else throws. It applies to the tick labels only: the axis labels have their own scale ([`axis_label_font_scale`](axis_label_font_scale.md)), the title and the legend are unaffected. The getter takes no argument.""",
        parameters="""**scale** (`double`, setter only)
    The factor on the default tick label size. Finite and strictly positive.""",
        returns="""**Setter:** `Pane&`, the same pane.
**Getter:** `double`, the scale in force; `1.0` for a new pane.""",
        examples=[
            dict(id="default", text="The default scale is 1. The tick labels are drawn at the default size.",
                 code=base.format(n="tickfont_default") + "\n" + SIZE + """
std::cout << "scale: " << pane.tick_font_scale() << ", tick label size: " << size_of(fig.to_svg(), "1.0") << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="larger", text="A scale of 1.5 makes the tick labels half as large again, and leaves the axis label and the title as they were.",
                 code=base.format(n="tickfont_larger") + "\n" + SIZE + """
pane.tick_font_scale(1.5);
const std::string svg = fig.to_svg();
std::cout << "tick: " << size_of(svg, "1.0") << ", axis label: " << size_of(svg, "time") << ", title: " << size_of(svg, "scaled") << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="smaller", text="A scale below 1 shrinks them, which leaves more of the figure to the data.",
                 code=base.format(n="tickfont_smaller") + "\n" + SIZE + """
pane.tick_font_scale(0.75);
std::cout << "tick: " << size_of(fig.to_svg(), "1.0") << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="all_axes", text="Every axis takes the scale. On a volumetric pane the labels of all three axes grow together.",
                 code="""auto fig = studio("tickfont_3d");
auto& pane = fig.pane3("p");
pane.path3(std::vector<double>{0, 1, 2, 1}, std::vector<double>{0, 1, 0, 2}, std::vector<double>{0, 1, 2, 3});
pane.tick_font_scale(1.4);
fig.save_svg("figure.svg");"""),
            dict(id="layout", text="The room follows the text. With larger tick labels the plot area starts further from the left edge: the y tick labels, right-aligned against the axis, end further to the right.",
                 code="""auto build = [&](double scale) {
  auto fig = studio("tickfont_layout");
  auto& pane = fig.pane2("p");
  pane.trend(std::vector<double>{0, 100, 200}, std::vector<double>{0, 1000, 0});
  pane.tick_font_scale(scale);
  return fig.to_svg();
};
%s
std::cout << "tick '1000' ends at " << at(build(1.0), "1000") << " at scale 1, " << at(build(2.0), "1000") << " at scale 2\\n";
auto fig = studio("tickfont_layout_out");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{0, 100, 200}, std::vector<double>{0, 1000, 0});
pane.tick_font_scale(2.0);
fig.save_svg("figure.svg");""" % AT),
            dict(id="errors", text="Zero, negative and non-finite scales throw, and the scale is left as it was.",
                 code="""auto fig = studio("tickfont_errors");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{0, 1}, std::vector<double>{0, 1});
for (double bad : {0.0, -1.0, std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity()}) {
  try {
    pane.tick_font_scale(bad);
  } catch (const pluss::Error& e) {
    std::cout << e.message() << "\\n";
  }
}
std::cout << "still " << pane.tick_font_scale() << "\\n";
fig.save_svg("figure.svg");"""),
        ],
        notes="- The setter throws `pluss::Error` for a scale that is not finite and positive; the getter is `noexcept`.\n- The scale multiplies the base size, so it is a ratio and not a size in points.",
        see=["[`Pane::axis_label_font_scale`](axis_label_font_scale.md): the same for the axis labels.", "[`Pane::x_tick_rotation`](x_tick_rotation.md): turn the labels.", "[`Pane::x_tick_labels`](x_tick_labels.md): switch the labels off."],
    )
    write("tick_font_scale", page)


def axis_label_font_page():
    base = """auto fig = studio("{n}");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{{0, 1, 2}}, std::vector<double>{{0, 1, 0}});
pane.x_label("time").y_label("level").title("scaled");"""
    page = dict(
        slug="axis_label_font_scale",
        title="Pane::axis_label_font_scale",
        signature="Pane& Pane::axis_label_font_scale(double scale)\ndouble Pane::axis_label_font_scale() const noexcept",
        intro="""Scales the font of the axis labels ([`x_label`](x_label.md), [`y_label`](y_label.md), [`z_label`](z_label.md)) of the pane, all axes at once. The scale is a factor on the default label size: 1 is the default, 2 doubles it. The room for the labels follows the size.

The scale must be finite and positive; anything else throws. It applies to the axis labels only; the tick labels have their own scale ([`tick_font_scale`](tick_font_scale.md)), and the title and legend are unaffected. The getter takes no argument.""",
        parameters="""**scale** (`double`, setter only)
    The factor on the default axis label size. Finite and strictly positive.""",
        returns="""**Setter:** `Pane&`, the same pane.
**Getter:** `double`, the scale in force; `1.0` for a new pane.""",
        examples=[
            dict(id="default", text="The default is 1, and the axis labels are a little larger than the tick labels.",
                 code=base.format(n="labelfont_default") + "\n" + SIZE + """
const std::string svg = fig.to_svg();
std::cout << "scale: " << pane.axis_label_font_scale() << ", axis label size: " << size_of(svg, "time") << ", tick size: " << size_of(svg, "1.0") << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="larger", text="A scale of 2 doubles both axis labels and leaves the ticks and the title alone.",
                 code=base.format(n="labelfont_larger") + "\n" + SIZE + """
pane.axis_label_font_scale(2.0);
const std::string svg = fig.to_svg();
std::cout << "x label: " << size_of(svg, "time") << ", y label: " << size_of(svg, "level") << ", tick: " << size_of(svg, "1.0") << ", title: " << size_of(svg, "scaled") << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="smaller", text="A scale below 1 gives quieter labels.",
                 code=base.format(n="labelfont_smaller") + "\n" + SIZE + """
pane.axis_label_font_scale(0.7);
std::cout << "x label: " << size_of(fig.to_svg(), "time") << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="independent", text="The two scales are independent. Here the tick labels are enlarged and the axis labels kept: only the tick size changes.",
                 code=base.format(n="labelfont_independent") + "\n" + SIZE + """
pane.tick_font_scale(1.6);
const std::string svg = fig.to_svg();
std::cout << "axis scale " << pane.axis_label_font_scale() << ": x label " << size_of(svg, "time") << ", tick " << size_of(svg, "1.0") << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="three_d", text="On a volumetric pane the labels of all three axes take the scale.",
                 code="""auto fig = studio("labelfont_3d");
auto& pane = fig.pane3("p");
pane.path3(std::vector<double>{0, 1, 2, 1}, std::vector<double>{0, 1, 0, 2}, std::vector<double>{0, 1, 2, 3});
pane.x_label("x").y_label("y").z_label("z");
pane.axis_label_font_scale(1.6);
fig.save_svg("figure.svg");"""),
            dict(id="errors", text="Zero, negative and non-finite scales throw, and the scale is left as it was.",
                 code="""auto fig = studio("labelfont_errors");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{0, 1}, std::vector<double>{0, 1});
for (double bad : {0.0, -2.0, std::numeric_limits<double>::quiet_NaN()}) {
  try {
    pane.axis_label_font_scale(bad);
  } catch (const pluss::Error& e) {
    std::cout << e.message() << "\\n";
  }
}
std::cout << "still " << pane.axis_label_font_scale() << "\\n";
fig.save_svg("figure.svg");"""),
        ],
        notes="- The setter throws `pluss::Error` for a scale that is not finite and positive; the getter is `noexcept`.",
        see=["[`Pane::tick_font_scale`](tick_font_scale.md): the same for the tick labels.", "[`Pane::x_label`](x_label.md): the labels this scales.", "[`Pane::x_label_gap`](x_label_gap.md): the distance of a label from the ticks."],
    )
    write("axis_label_font_scale", page)


def title_pages():
    base = """auto fig = studio("{n}");
auto& pane = fig.pane2("{t}");
pane.trend(std::vector<double>{{0, 1, 2}}, std::vector<double>{{0, 1, 0}});"""
    t = dict(
        slug="title",
        title="Pane::title",
        signature="Pane& Pane::title(std::string title)",
        intro="""Sets the title of the pane, drawn above its plot area, centred over it. It replaces the title the pane was made with ([`Studio::pane2`](../studio/pane2.md)'s argument) and may be called at any time. An empty string removes the title, and a pane without one takes back the room the title would have used.

The title belongs to this pane. The figure's own heading is [`Studio::headline`](../studio/headline.md); a pane's [`caption`](caption.md) is a note below the plot; and an axis has its label. Text between dollar signs is typeset as mathematics.""",
        parameters="""**title** (`std::string`)
    The title. Empty for none.""",
        returns="""`Pane&`
    The same pane, for chaining.""",
        examples=[
            dict(id="set", text="Setting a title after the pane is made. The text is written above the plot and read back with `title_text()`.",
                 code=base.format(n="title_set", t="") + """
pane.title("Daily totals");
std::cout << "title_text: " << pane.title_text() << "\\n";
std::cout << "drawn: " << (fig.to_svg().find(">Daily totals<") != std::string::npos) << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="replace", text="A second call replaces the first, and the title the pane was made with is replaced the same way.",
                 code=base.format(n="title_replace", t="first") + """
std::cout << "made with: " << pane.title_text() << "\\n";
pane.title("second");
std::cout << "after title(): " << pane.title_text() << "\\n";
const std::string svg = fig.to_svg();
std::cout << "first drawn: " << (svg.find(">first<") != std::string::npos) << ", second drawn: " << (svg.find(">second<") != std::string::npos) << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="clear", text="An empty string removes the title. The figure is then exactly the figure of a pane that never had one, the room included.",
                 code="""auto build = [&](bool titled_then_cleared) {
  auto fig = studio("title_clear");
  auto& pane = fig.pane2();
  pane.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
  if (titled_then_cleared) pane.title("temporary").title("");
  return fig.to_svg();
};
std::cout << "cleared == never titled: " << (build(true) == build(false)) << "\\n";
auto fig = studio("title_clear_out");
auto& pane = fig.pane2();
pane.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
fig.save_svg("figure.svg");"""),
            dict(id="mosaic", text="Each pane has its own. In a mosaic every pane is titled for what it shows.",
                 code="""auto fig = studio("title_mosaic");
fig.mosaic(1, 2);
auto& a = fig.pane2();
a.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
a.title("rising then falling");
auto& b = fig.pane2();
b.trend(std::vector<double>{0, 1, 2}, std::vector<double>{1, 0, 1});
b.title("falling then rising");
fig.save_svg("figure.svg");"""),
            dict(id="math", text="Mathematics between dollar signs is typeset.",
                 code=base.format(n="title_math", t="") + """
pane.title("Decay $e^{-t}$");
fig.save_svg("figure.svg");"""),
            dict(id="distinct", text="A title is not the headline and not a caption: the three are set by three calls and drawn in three places.",
                 code=base.format(n="title_distinct", t="") + """
pane.title("pane title").caption("pane caption");
fig.headline("figure headline");
const std::string svg = fig.to_svg();
std::cout << "all three drawn: " << (svg.find(">pane title<") != std::string::npos && svg.find(">pane caption<") != std::string::npos && svg.find(">figure headline<") != std::string::npos) << "\\n";
fig.save_svg("figure.svg");"""),
        ],
        notes="- Any string is accepted; the empty string clears the title.\n- Read the title back with [`title_text`](title_text.md).",
        see=["[`Pane::title_text`](title_text.md): read the title.", "[`Pane::caption`](caption.md): a note below the plot.", "[`Studio::headline`](../studio/headline.md): the heading of the figure."],
    )
    write("title", t)
    page = dict(
        slug="title_text",
        title="Pane::title_text",
        signature="const std::string& Pane::title_text() const noexcept",
        intro="""Returns the pane's title: the text it was made with ([`Studio::pane2`](../studio/pane2.md), [`pane3`](../studio/pane3.md)) or the last one given to [`title`](title.md). An empty string means no title.

Use it to find a pane among those of a figure ([`Studio::panes`](../studio/panes.md) lists them), to build a legend or a report from the titles, or to restore a title after changing it. The reference is to the pane's own string and follows later calls; copy it to keep the value.""",
        parameters="*(none)*",
        returns="""`const std::string&`
    The title, empty for none.""",
        examples=[
            dict(id="made_with", text="The title given when the pane is made.",
                 code="""auto fig = studio("title_text_made");
auto& pane = fig.pane2("Revenue");
pane.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
std::cout << "'" << pane.title_text() << "'\\n";
fig.save_svg("figure.svg");"""),
            dict(id="untitled", text="A pane made without a title has an empty one.",
                 code="""auto fig = studio("title_text_untitled");
auto& pane = fig.pane2();
pane.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
std::cout << "empty: " << pane.title_text().empty() << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="after_title", text="After `title()` it returns the new text.",
                 code="""auto fig = studio("title_text_after");
auto& pane = fig.pane2("old");
pane.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
pane.title("new");
std::cout << pane.title_text() << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="listing", text="Listing the panes of a figure by title.",
                 code="""auto fig = studio("title_text_listing");
fig.mosaic(1, 3);
for (const char* name : {"north", "south", "east"}) {
  fig.pane2(name).trend(std::vector<double>{0, 1}, std::vector<double>{0, 1});
}
for (const auto& pane : fig.panes()) std::cout << pane.title_text() << " ";
std::cout << "\\n";
fig.save_svg("figure.svg");"""),
        ],
        notes="- `noexcept`; reads only.",
        see=["[`Pane::title`](title.md): set the title.", "[`Studio::panes`](../studio/panes.md): the panes of a figure."],
    )
    write("title_text", page)


def caption_pages():
    base = """auto fig = studio("{n}");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{{0, 1, 2}}, std::vector<double>{{0, 1, 0}});"""
    page = dict(
        slug="caption",
        title="Pane::caption",
        signature="Pane& Pane::caption(std::string text)",
        intro="""Sets a caption for this pane: a line of text written below its plot, for a source, a unit, a note on how the data was made. An empty string removes it.

A caption belongs to one pane and needs nothing else. The figure-level captions written once for a group of panes are [`Studio::x_caption`](../studio/x_caption.md) and its y and z forms, which panes join by [alignment group](align_x.md); a pane's [`title`](title.md) is above the plot and an axis label is beside an axis. The pane keeps room for its caption only while it has one.""",
        parameters="""**text** (`std::string`)
    The caption. Empty for none.""",
        returns="""`Pane&`
    The same pane, for chaining.""",
        examples=[
            dict(id="set", text="A caption written below the plot, and read back with `caption_text()`.",
                 code=base.format(n="caption_set") + """
pane.caption("Source: field survey, 2025");
std::cout << pane.caption_text() << "\\n";
std::cout << "drawn: " << (fig.to_svg().find(">Source: field survey, 2025<") != std::string::npos) << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="replace_clear", text="A second call replaces the caption and an empty string removes it; with it removed the figure is the figure of a pane that never had one.",
                 code="""auto build = [&](int mode) {
  auto fig = studio("caption_clear");
  auto& pane = fig.pane2("p");
  pane.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
  if (mode >= 1) pane.caption("first");
  if (mode == 2) pane.caption("second").caption("");
  return fig.to_svg();
};
std::cout << "first drawn: " << (build(1).find(">first<") != std::string::npos) << "\\n";
std::cout << "cleared == never set: " << (build(2) == build(0)) << "\\n";
auto fig = studio("caption_clear_out");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
pane.caption("second");
fig.save_svg("figure.svg");"""),
            dict(id="each_pane", text="Each pane has its own caption, written under its own plot.",
                 code="""auto fig = studio("caption_each");
fig.mosaic(1, 2);
auto& a = fig.pane2("a");
a.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
a.caption("measured in June");
auto& b = fig.pane2("b");
b.trend(std::vector<double>{0, 1, 2}, std::vector<double>{1, 0, 1});
b.caption("measured in July");
fig.save_svg("figure.svg");"""),
            dict(id="distinct", text="A caption is not a title and not an axis label; all three are drawn, each where it belongs.",
                 code=base.format(n="caption_distinct") + """
pane.title("title").caption("caption").x_label("axis label");
const std::string svg = fig.to_svg();
std::cout << "title, caption and label all drawn: " << (svg.find(">title<") != std::string::npos && svg.find(">caption<") != std::string::npos && svg.find(">axis label<") != std::string::npos) << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="math", text="Mathematics between dollar signs is typeset.",
                 code=base.format(n="caption_math") + """
pane.caption("Fitted with $y = a e^{-bt}$");
fig.save_svg("figure.svg");"""),
        ],
        notes="- Any string is accepted; the empty string clears the caption.",
        see=["[`Pane::caption_text`](caption_text.md): read it back.", "[`Pane::title`](title.md): the text above the plot.", "[`Studio::x_caption`](../studio/x_caption.md): a caption shared by a group of panes."],
    )
    write("caption", page)
    page = dict(
        slug="caption_text",
        title="Pane::caption_text",
        signature="const std::string& Pane::caption_text() const noexcept",
        intro="""Returns the pane's caption, as set by [`caption`](caption.md); an empty string means none. A new pane has none.

Use it to check what a pane carries before changing it, or to collect the captions of a figure's panes into a note. The reference is to the pane's own string and follows later calls; copy it to keep the value.""",
        parameters="*(none)*",
        returns="""`const std::string&`
    The caption, empty for none.""",
        examples=[
            dict(id="default", text="A new pane has no caption.",
                 code=base.format(n="caption_text_default") + """
std::cout << "empty: " << pane.caption_text().empty() << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="after", text="After `caption()` it returns the text; after clearing it, the empty string.",
                 code=base.format(n="caption_text_after") + """
pane.caption("a note");
std::cout << "'" << pane.caption_text() << "'\\n";
pane.caption("");
std::cout << "'" << pane.caption_text() << "'\\n";
fig.save_svg("figure.svg");"""),
            dict(id="collect", text="Collecting the captions of a figure.",
                 code="""auto fig = studio("caption_text_collect");
fig.mosaic(1, 2);
auto& a = fig.pane2("a");
a.trend(std::vector<double>{0, 1}, std::vector<double>{0, 1});
a.caption("first note");
auto& b = fig.pane2("b");
b.trend(std::vector<double>{0, 1}, std::vector<double>{0, 1});
for (const auto& pane : fig.panes()) {
  std::cout << pane.title_text() << ": " << (pane.caption_text().empty() ? "(none)" : pane.caption_text()) << "\\n";
}
fig.save_svg("figure.svg");"""),
        ],
        notes="- `noexcept`; reads only.",
        see=["[`Pane::caption`](caption.md): set the caption.", "[`Pane::title_text`](title_text.md): the title."],
    )
    write("caption_text", page)



# ------------------------------------------------------------------ lens, lights, theme, accessors, place
SURF = """auto z = pluss::series::linspace(0.0, 4.0, 9, pluss::ScalarType::Float64).reshape({3, 3});"""
PATH3 = """pane.path3(std::vector<double>{0, 1, 2, 1}, std::vector<double>{0, 1, 0, 2}, std::vector<double>{0, 1, 2, 3});"""


def lens_page():
    page = dict(
        slug="lens",
        title="Pane::lens",
        signature="const Lens& Pane::lens() const noexcept\nLens& Pane::lens() noexcept",
        intro="""Returns the pane's camera, a [`Lens`](../types/Lens.md): the viewing angles, the point looked at, the direction that is up, and the projection. There is no `Pane::lens(Lens)` setter: the non-const call returns a reference, and the view is changed through the chainable calls of the lens, `pane.lens().look(60, 25).project(ProjectionKind::Perspective)`, or by assigning a whole `Lens` to it.

| Call on the lens | Effect |
|---|---|
| `look(azimuth, elevation)` | Sets the viewing angles in degrees, replacing the current ones. |
| `spin(d_azimuth, d_elevation)` | Turns the view by the given angles from where it is. |
| `project(kind)` | `Orthographic` (the default of a pane: parallel lines stay parallel) or `Perspective` (distant parts look smaller). |
| `view_angle(degrees)` | The field of view of the perspective projection, strictly between 0 and 180; anything else throws. Orthographic views ignore it. |
| `target_at(point)` | The point the picture is centred on, in box units (0 is the centre of the data box, 0.5 its faces). |
| `up(vector)` | The direction drawn upward on the page. |

A volumetric pane starts with `Lens::spatial()` (azimuth -37.5, elevation 30, up = z) and a planar pane with `Lens::planar()` (straight down the z axis, up = y); see [`kind`](kind.md). Only volumetric panes are drawn through the lens: a planar pane looks straight down whatever it holds.""",
        parameters="*(none)*",
        returns="""`const Lens&` / `Lens&`
    The pane's own lens. The reference follows later calls; copy the lens to keep a view.""",
        examples=[
            dict(id="defaults", text="The lens each kind of pane starts with.",
                 code="""auto fig = studio("lens_defaults");
fig.mosaic(1, 2);
auto& flat = fig.pane2("flat");
flat.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
auto& deep = fig.pane3("deep");
auto& pane = deep;
%s
auto show = [](const char* name, const Lens& lens) {
  std::cout << name << ": azimuth " << lens.azimuth_deg() << ", elevation " << lens.elevation_deg() << ", view angle " << lens.view_angle_deg()
            << ", orthographic " << (lens.projection() == ProjectionKind::Orthographic) << ", target (" << lens.target().x << ", " << lens.target().y << ", " << lens.target().z
            << "), up (" << lens.up_vector().x << ", " << lens.up_vector().y << ", " << lens.up_vector().z << ")\\n";
};
show("planar    ", flat.lens());
show("volumetric", deep.lens());
fig.save_svg("figure.svg");""" % PATH3),
            dict(id="look", text="`look` sets the angles outright; a second call replaces the first. Four views of the same data by azimuth and elevation.",
                 code="""const double views[4][2] = {{-37.5, 30.0}, {-90.0, 10.0}, {0.0, 80.0}, {45.0, 30.0}};
auto fig = studio("lens_look");
fig.mosaic(1, 4);
for (int i = 0; i < 4; ++i) {
  auto& pane = fig.pane3("az " + std::to_string(static_cast<int>(views[i][0])) + ", el " + std::to_string(static_cast<int>(views[i][1])));
  %s
  pane.lens().look(0.0, 0.0).look(views[i][0], views[i][1]);
}
std::cout << "last pane: azimuth " << fig.panes().back().lens().azimuth_deg() << ", elevation " << fig.panes().back().lens().elevation_deg() << "\\n";
fig.save_svg("figure.svg");""" % PATH3),
            dict(id="spin", text="`spin` adds to the angles. After `look(10, 20)` a spin of (5, -5) leaves 15 and 15, and a second spin goes on from there; `look` would have replaced them.",
                 code="""auto fig = studio("lens_spin");
auto& pane = fig.pane3("p");
%s
pane.lens().look(10.0, 20.0).spin(5.0, -5.0);
std::cout << "after look(10, 20).spin(5, -5): " << pane.lens().azimuth_deg() << ", " << pane.lens().elevation_deg() << "\\n";
pane.lens().spin(5.0, -5.0);
std::cout << "after a second spin: " << pane.lens().azimuth_deg() << ", " << pane.lens().elevation_deg() << "\\n";
fig.save_svg("figure.svg");""" % PATH3),
            dict(id="project", text="`project`. A pane starts orthographic, where parallel lines stay parallel; in the perspective view the near part of the scene is drawn larger and parallel edges converge.",
                 code="""auto fig = studio("lens_project");
fig.mosaic(1, 2);
for (int i = 0; i < 2; ++i) {
  auto& pane = fig.pane3(i == 0 ? "Orthographic" : "Perspective");
  %s
  pane.lens().project(i == 0 ? ProjectionKind::Orthographic : ProjectionKind::Perspective);
}
std::cout << "default is Orthographic: " << (studio("d").pane3().lens().projection() == ProjectionKind::Orthographic) << "\\n";
fig.save_svg("figure.svg");""" % PATH3),
            dict(id="view_angle", text="`view_angle` is the field of view of the perspective projection. A wide angle puts the eye close and exaggerates the perspective, a narrow one moves it away and flattens the view; the orthographic view has no field of view and ignores the angle.",
                 code="""auto build = [&](ProjectionKind kind, double angle) {
  auto fig = studio("lens_view_angle");
  auto& pane = fig.pane3("p");
  %s
  pane.lens().project(kind).view_angle(angle);
  return fig.to_svg();
};
std::cout << "perspective, 20 vs 70 degrees differ: " << (build(ProjectionKind::Perspective, 20.0) != build(ProjectionKind::Perspective, 70.0)) << "\\n";
std::cout << "orthographic, 20 vs 70 degrees differ: " << (build(ProjectionKind::Orthographic, 20.0) != build(ProjectionKind::Orthographic, 70.0)) << "\\n";
auto fig = studio("lens_view_angle_out");
fig.mosaic(1, 2);
for (int i = 0; i < 2; ++i) {
  auto& pane = fig.pane3(i == 0 ? "20 degrees" : "70 degrees");
  %s
  pane.lens().project(ProjectionKind::Perspective).view_angle(i == 0 ? 20.0 : 70.0);
}
fig.save_svg("figure.svg");""" % (PATH3, PATH3)),
            dict(id="view_angle_errors", text="An angle outside (0, 180) has no field of view, so `view_angle` throws and leaves the lens as it was.",
                 code="""auto fig = studio("lens_view_angle_errors");
auto& pane = fig.pane3("p");
%s
for (double bad : {0.0, 180.0, -5.0, std::numeric_limits<double>::quiet_NaN()}) {
  try {
    pane.lens().view_angle(bad);
  } catch (const pluss::Error& e) {
    std::cout << e.message() << "\\n";
  }
}
std::cout << "still " << pane.lens().view_angle_deg() << "\\n";
fig.save_svg("figure.svg");""" % PATH3),
            dict(id="target", text="`target_at` centres the picture on a point, given in box units: (0, 0, 0) is the centre of the data box, which is where every lens starts. A target 0.4 box widths along x puts that point in the middle, and the whole box stays in view, so the data sits off the centre of the picture.",
                 code="""auto fig = studio("lens_target");
fig.mosaic(1, 2);
for (int i = 0; i < 2; ++i) {
  auto& pane = fig.pane3(i == 0 ? "target at the centre" : "target at (0.4, 0, 0)");
  %s
  if (i == 1) pane.lens().target_at(Vec3d{0.4, 0.0, 0.0});
}
const Lens& lens = fig.panes().back().lens();
std::cout << "target (" << lens.target().x << ", " << lens.target().y << ", " << lens.target().z << ")\\n";
fig.save_svg("figure.svg");""" % PATH3),
            dict(id="up", text="`up` chooses which direction is drawn upward. By default z is up; with y up, the same data is drawn turned.",
                 code="""auto fig = studio("lens_up");
fig.mosaic(1, 2);
for (int i = 0; i < 2; ++i) {
  auto& pane = fig.pane3(i == 0 ? "z up" : "y up");
  %s
  if (i == 1) pane.lens().up(Vec3d{0.0, 1.0, 0.0});
}
fig.save_svg("figure.svg");""" % PATH3),
            dict(id="assign", text="A view can be kept and reused as a value. The lens of the first pane is copied into the second, which is then drawn from the same view; assigning `Lens::planar()` goes straight down.",
                 code="""auto fig = studio("lens_assign");
fig.mosaic(1, 3);
auto& first = fig.pane3("tuned");
%s
first.lens().look(20.0, 45.0).project(ProjectionKind::Perspective);
const Lens kept = first.lens();
auto& second = fig.pane3("copied");
%s
second.lens() = kept;
auto& third = fig.pane3("from above");
%s
third.lens() = Lens::planar();
std::cout << "copied: azimuth " << second.lens().azimuth_deg() << ", elevation " << second.lens().elevation_deg() << "\\n";
std::cout << "from above: elevation " << third.lens().elevation_deg() << "\\n";
fig.save_svg("figure.svg");""" % (PATH3.replace("pane.", "first."), PATH3.replace("pane.", "second."), PATH3.replace("pane.", "third."))),
            dict(id="planar", text="A planar pane is drawn straight down whatever its lens says: the figure is the same with any view.",
                 code="""auto build = [&](bool turned) {
  auto fig = studio("lens_planar");
  auto& pane = fig.pane2("p");
  pane.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
  if (turned) pane.lens().look(40.0, 40.0);
  return fig.to_svg();
};
std::cout << "planar pane unchanged by look(): " << (build(true) == build(false)) << "\\n";
auto fig = studio("lens_planar_out");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
fig.save_svg("figure.svg");"""),
        ],
        notes="- `look`, `spin`, `target_at`, `up` and `project` are `noexcept`; `view_angle` throws `pluss::Error` for an angle outside (0, 180).\n- The pane is fitted to its plot area after the view is applied, so the view changes the shape of the picture and not its size.",
        see=["[`Lens`](../types/Lens.md): the type, with its full member list.", "[`Pane::kind`](kind.md): which lens a pane starts with.", "[`Pane::lights`](lights.md): the light, which is placed relative to the view.", "[`Pane::parallel_xy_labels`](parallel_xy_labels.md): axis labels that follow the view."],
    )
    write("lens", page)


def lights_page():
    page = dict(
        slug="lights",
        title="Pane::lights",
        signature="const LightRig& Pane::lights() const noexcept\nLightRig& Pane::lights() noexcept",
        intro="""Returns the pane's [`LightRig`](../types/LightRig.md): the lamps that shade its surfaces. As with [`lens`](lens.md) there is no setter; the non-const call returns a reference and lights are added through the rig's chainable calls.

A new pane has **no lights**, and surfaces are shaded only when both are true: the pane has at least one light, and the trace asks for shading with [`Trace::lighting`](../trace/lighting.md) (`Flat` or `Gouraud`; the default `None` draws the colours as they are). A lit surface is drawn with the brightness the lights give each face and a highlight set by its [`material`](../trace/material.md).

| Call on the rig | Adds |
|---|---|
| `headlight()` | One lamp at the camera, pointing the way the camera does: flat, shadowless light. |
| `lamp_right()` | One lamp above and to the right of the camera (30 degrees each). |
| `lamp_left()` | One lamp above and to the left (-30 degrees azimuth). |
| `sun_key()` | Two far-away (parallel) lights, a key and a softer fill, fixed relative to the camera. |
| `add(LightSpec)` | One lamp as specified: azimuth and elevation in degrees, `LightSpace::Camera` (follows the view) or `Data` (stays fixed to the data as the view turns), and `LightStyle::Local` (a lamp at a finite distance) or `Infinite` (parallel rays). |
| `clear()` | Removes every light. |

The brightness is the average over the lights, so adding a second lamp gives a different light and not a brighter one. `empty()` and `lights()` on the rig read the lights back; reading the list from a pane is `pane.lights().lights()`.""",
        parameters="*(none)*",
        returns="""`const LightRig&` / `LightRig&`
    The pane's own rig. The reference follows later calls.""",
        examples=[
            dict(id="default", text="A new pane has no lights.",
                 code="""auto fig = studio("lights_default");
auto& pane = fig.pane3("p");
%s
std::cout << "empty: " << pane.lights().empty() << ", count: " << pane.lights().lights().size() << "\\n";
fig.save_svg("figure.svg");""" % PATH3),
            dict(id="presets", text="The presets. Each adds the lights shown: `headlight` and the two lamps one local light each, `sun_key` two parallel ones.",
                 code="""auto fig = studio("lights_presets");
auto& pane = fig.pane3("p");
%s
const char* styles[] = {"Local", "Infinite"};
auto list = [&](const char* name) {
  std::cout << name << ":";
  for (const auto& light : pane.lights().lights()) std::cout << " (az " << light.azimuth_deg << ", el " << light.elevation_deg << ", " << styles[static_cast<int>(light.style)] << ")";
  std::cout << "\\n";
  pane.lights().clear();
};
pane.lights().headlight();  list("headlight ");
pane.lights().lamp_right(); list("lamp_right");
pane.lights().lamp_left();  list("lamp_left ");
pane.lights().sun_key();    list("sun_key   ");
fig.save_svg("figure.svg");""" % PATH3),
            dict(id="custom", text="`add` takes a light of your own. This one is parallel light from the far side, fixed to the data rather than the camera.",
                 code="""auto fig = studio("lights_custom");
auto& pane = fig.pane3("p");
%s
pane.lights().add(LightSpec{90.0, 10.0, LightSpace::Data, LightStyle::Infinite});
const auto& light = pane.lights().lights().front();
std::cout << "lights: " << pane.lights().lights().size() << ", azimuth " << light.azimuth_deg << ", elevation " << light.elevation_deg
          << ", fixed to data " << (light.space == LightSpace::Data) << ", infinite " << (light.style == LightStyle::Infinite) << "\\n";
fig.save_svg("figure.svg");""" % PATH3),
            dict(id="needs_lighting", text="Lights act on traces that ask for shading. The same surface and the same lamp: with `lighting(LightingMode::None)`, the default, the lamp changes nothing; with `Flat` the faces are shaded.",
                 code=SURF + """
auto build = [&](LightingMode mode, bool lamp) {
  auto fig = studio("lights_needs");
  auto& pane = fig.pane3("p");
  pane.spire3(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 2}, z).lighting(mode);
  if (lamp) pane.lights().lamp_right();
  return fig.to_svg();
};
std::cout << "lighting None, lamp == no lamp: " << (build(LightingMode::None, true) == build(LightingMode::None, false)) << "\\n";
std::cout << "lighting Flat, lamp != no lamp: " << (build(LightingMode::Flat, true) != build(LightingMode::Flat, false)) << "\\n";
auto fig = studio("lights_needs_out");
auto& pane = fig.pane3("p");
pane.spire3(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 2}, z).lighting(LightingMode::Flat);
pane.lights().lamp_right();
fig.save_svg("figure.svg");"""),
            dict(id="rigs", text="Different rigs, different pictures. The lamp on the right and the lamp on the left shade the same surface differently, and so do the headlight and the sun.",
                 code=SURF + """
auto build = [&](int rig) {
  auto fig = studio("lights_rigs");
  auto& pane = fig.pane3("p");
  pane.spire3(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 2}, z).lighting(LightingMode::Flat);
  if (rig == 0) pane.lights().headlight();
  if (rig == 1) pane.lights().lamp_right();
  if (rig == 2) pane.lights().lamp_left();
  if (rig == 3) pane.lights().sun_key();
  return fig.to_svg();
};
std::cout << "right != left: " << (build(1) != build(2)) << ", headlight != right: " << (build(0) != build(1)) << ", sun != right: " << (build(3) != build(1)) << "\\n";
auto fig = studio("lights_rigs_out");
fig.mosaic(1, 4);
const char* names[] = {"headlight", "lamp_right", "lamp_left", "sun_key"};
for (int i = 0; i < 4; ++i) {
  auto& pane = fig.pane3(names[i]);
  pane.spire3(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 2}, z).lighting(LightingMode::Flat);
  if (i == 0) pane.lights().headlight();
  if (i == 1) pane.lights().lamp_right();
  if (i == 2) pane.lights().lamp_left();
  if (i == 3) pane.lights().sun_key();
}
fig.save_svg("figure.svg");"""),
            dict(id="clear", text="`clear` removes every light: the pane is unlit again and the figure is the one that never had a light.",
                 code=SURF + """
auto build = [&](bool lit_then_cleared) {
  auto fig = studio("lights_clear");
  auto& pane = fig.pane3("p");
  pane.spire3(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 2}, z).lighting(LightingMode::Flat);
  if (lit_then_cleared) pane.lights().sun_key().clear();
  return fig.to_svg();
};
std::cout << "cleared == never lit: " << (build(true) == build(false)) << "\\n";
auto fig = studio("lights_clear_out");
auto& pane = fig.pane3("p");
pane.spire3(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 2}, z).lighting(LightingMode::Flat);
pane.lights().sun_key().clear();
std::cout << "empty: " << pane.lights().empty() << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="space", text="Camera or data. A light fixed to the camera is placed relative to the viewer and keeps its direction on the screen as the view turns; a light fixed to the data is placed relative to the scene and turns with it. The same lamp in the two spaces is two different lights, whatever the view.",
                 code=SURF + """
auto build = [&](LightSpace space, double azimuth) {
  auto fig = studio("lights_space");
  auto& pane = fig.pane3("p");
  pane.spire3(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 2}, z).lighting(LightingMode::Flat);
  pane.lights().add(LightSpec{30.0, 30.0, space, LightStyle::Infinite});
  pane.lens().look(azimuth, 30.0);
  return fig.to_svg();
};
std::cout << "default view: Camera != Data: " << (build(LightSpace::Camera, -37.5) != build(LightSpace::Data, -37.5)) << "\\n";
std::cout << "turned view:  Camera != Data: " << (build(LightSpace::Camera, 60.0) != build(LightSpace::Data, 60.0)) << "\\n";
auto fig = studio("lights_space_out");
fig.mosaic(1, 2);
for (int i = 0; i < 2; ++i) {
  auto& pane = fig.pane3(i == 0 ? "fixed to the camera" : "fixed to the data");
  pane.spire3(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 2}, z).lighting(LightingMode::Flat);
  pane.lights().add(LightSpec{30.0, 30.0, i == 0 ? LightSpace::Camera : LightSpace::Data, LightStyle::Infinite});
  pane.lens().look(60.0, 30.0);
}
fig.save_svg("figure.svg");"""),
        ],
        notes="- The rig's `clear`, `headlight`, `lamp_right`, `lamp_left` and `sun_key` are `noexcept`; `add` can throw only on allocation failure.\n- Lights have no effect on planar panes, on traces with `LightingMode::None`, or on traces that are not surfaces.",
        see=["[`LightRig`](../types/LightRig.md): the type.", "[`Trace::lighting`](../trace/lighting.md), [`Trace::material`](../trace/material.md), [`Trace::shade`](../trace/shade.md): how a surface answers the light.", "[`Pane::lens`](lens.md): the view that camera-fixed lights follow."],
    )
    write("lights", page)


def theme_tokens_page():
    page = dict(
        slug="theme_tokens",
        title="Pane::theme_tokens",
        signature="const Theme& Pane::theme_tokens() const noexcept\nTheme& Pane::theme_tokens() noexcept",
        intro="""Returns the pane's own [`Theme`](../types/Theme.md): the accent colours its traces cycle through, the background and foreground colours, and the palette of its surfaces. The non-const call returns a reference, and the theme is changed by assigning a whole `Theme` to it; a `Theme` has no setters for single fields.

A pane takes a **copy** of the figure's theme at the moment it is made ([`Studio::theme`](../studio/theme.md) before [`pane2`](../studio/pane2.md)). After that the two are independent: changing the pane's theme does not touch the figure or the other panes, and changing the figure's theme later does not reach panes that already exist. To restyle a whole figure, set the figure's theme first and then make the panes; to restyle one pane, assign to its `theme_tokens()`.""",
        parameters="*(none)*",
        returns="""`const Theme&` / `Theme&`
    The pane's own theme. The reference follows later calls; copy it to keep a theme.""",
        examples=[
            dict(id="default", text="A pane starts with the theme of its figure.",
                 code="""auto fig = studio("theme_default");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
std::cout << "figure: " << fig.theme_tokens().name() << ", pane: " << pane.theme_tokens().name() << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="assign", text="Assigning a theme restyles the pane: the same data in the two built-in themes, `Theme::paper()` and `Theme::ocean()`.",
                 code="""auto fig = studio("theme_assign");
fig.mosaic(1, 2);
auto& a = fig.pane2("paper");
a.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
a.theme_tokens() = Theme::paper();
auto& b = fig.pane2("ocean");
b.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
b.theme_tokens() = Theme::ocean();
std::cout << a.theme_tokens().name() << " and " << b.theme_tokens().name() << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="snapshot", text="The copy is taken when the pane is made. The first pane is made under the ocean theme; the figure's theme is then changed to paper, and the second pane is made. Each keeps the theme of its moment.",
                 code="""auto fig = studio("theme_snapshot");
fig.mosaic(1, 2);
fig.theme(Theme::ocean());
auto& first = fig.pane2("made under ocean");
first.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
fig.theme(Theme::paper());
auto& second = fig.pane2("made under paper");
second.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
std::cout << "first: " << first.theme_tokens().name() << ", second: " << second.theme_tokens().name() << ", figure: " << fig.theme_tokens().name() << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="independent", text="Independent afterwards. Assigning a theme to one pane leaves the figure's theme and the other pane as they were.",
                 code="""auto fig = studio("theme_independent");
fig.mosaic(1, 2);
auto& changed = fig.pane2("changed");
changed.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
auto& untouched = fig.pane2("untouched");
untouched.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
const std::string before = fig.theme_tokens().name();
changed.theme_tokens() = before == "paper" ? Theme::ocean() : Theme::paper();
std::cout << "changed: " << changed.theme_tokens().name() << ", untouched: " << untouched.theme_tokens().name() << ", figure still " << fig.theme_tokens().name() << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="read", text="Reading the tokens. The accents are the colours the traces take in turn, and the background and foreground are the colours of the page and of text and axes.",
                 code="""auto fig = studio("theme_read");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
const Theme& theme = pane.theme_tokens();
std::cout << theme.name() << ": " << theme.accents().size() << " accents; accent(0) red " << theme.accent(0).red << "\\n";
std::cout << "background alpha " << theme.background().alpha << ", foreground alpha " << theme.foreground().alpha << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="custom", text="A theme of your own. The constructor takes a name, the accents, the background and foreground, and the palette of surfaces; this one is a dark page with two accents.",
                 code="""auto fig = studio("theme_custom");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
pane.trend(std::vector<double>{0, 1, 2}, std::vector<double>{1, 0, 1});
pane.theme_tokens() = Theme("night", {Rgba::from_u8(255, 170, 60), Rgba::from_u8(90, 200, 255)}, Rgba::from_u8(18, 20, 28), Rgba::from_u8(230, 230, 235), PaletteRamp::thermal());
std::cout << pane.theme_tokens().name() << " with " << pane.theme_tokens().accents().size() << " accents\\n";
fig.save_svg("figure.svg");"""),
        ],
        notes="- Both forms are `noexcept`.\n- The figure's page background is the figure's theme; a pane's own background colour is drawn inside its area.",
        see=["[`Theme`](../types/Theme.md): the type and its built-in themes.", "[`Studio::theme`](../studio/theme.md): the figure's theme, which new panes copy.", "[`Studio::theme_tokens`](../studio/theme_tokens.md): the figure's theme, read back."],
    )
    write("theme_tokens", page)


def axis_accessors_page():
    page = dict(
        slug="axis_accessors",
        title="Pane::x_axis, y_axis, z_axis, color_axis",
        signature="const AxisState& Pane::x_axis() const noexcept\nconst AxisState& Pane::y_axis() const noexcept\nconst AxisState& Pane::z_axis() const noexcept\nconst AxisState& Pane::color_axis() const noexcept",
        intro="""Read the whole state of one axis of the pane: the three position axes and the colour axis. Every axis setting ([`x_limits`](x_limits.md), [`x_label`](x_label.md), [`x_ticks`](x_ticks.md), [`scale_x`](scale_x.md), ...) is written into one of these four `AxisState` values, and these accessors return it as a read-only value. They are what to use to inspect a pane that was built elsewhere, or to check what a setting did.

| Field | Type | Holds | Set by |
|---|---|---|---|
| `scale` | `ScalePolicy` | Linear, log or symmetric-log scale. | [`scale_x`](scale_x.md) |
| `limit_mode` | `LimitMode` | `AutoFit` (from the data) or `Manual`. | [`x_limits`](x_limits.md), [`auto_x`](auto_x.md) |
| `limit_method` | `LimitMethod` | How an automatic range is widened. | [`x_limit_method`](x_limit_method.md) |
| `limits` | `std::optional<Range1d>` | The range set by hand, if any. | [`x_limits`](x_limits.md) |
| `tick_format` | `TickFormat` | How tick numbers are written. | [`x_tick_format`](x_tick_format.md) |
| `auto_tick_decimals` | `std::optional<int>` | A fixed number of decimals for automatic ticks. | [`x_auto_tick_decimals`](x_auto_tick_decimals.md) |
| `label` | `std::string` | The axis label. | [`x_label`](x_label.md) |
| `ticks` | `std::optional<AxisTicks>` | Ticks given by hand: positions and labels. | [`x_ticks`](x_ticks.md), [`x_categories`](x_categories.md) |
| `tick_rotation_deg` | `std::optional<double>` | Rotation of the tick labels. | [`x_tick_rotation`](x_tick_rotation.md) |
| `label_rotation_deg` | `std::optional<double>` | Rotation of the axis label. | [`x_label_rotation`](x_label_rotation.md) |
| `tick_label_gap` | `std::optional<double>` | Distance of tick labels from the axis, in points. | [`x_tick_label_gap`](x_tick_label_gap.md) |
| `label_gap` | `std::optional<double>` | Distance of the axis label from the tick labels. | [`x_label_gap`](x_label_gap.md) |
| `label_centered_on_axis` | `bool` | The y label stands vertical, centred on the axis. | [`y_label_centered`](y_label_centered.md) |
| `tick_labels_visible` | `bool` | Whether tick labels are drawn. | [`x_tick_labels`](x_tick_labels.md) |
| `major_grid`, `minor_grid` | `bool` | The grid lines of the axis. | [`x_major_grid`](x_major_grid.md), [`x_minor_grid`](x_minor_grid.md) |

An optional field is empty until its setter is called, which means "use the default for the figure"; the clear calls (`clear_x_tick_rotation`, ...) empty it again. The colour axis uses the limit fields only: [`color_limits`](color_limits.md), [`color_limit_method`](color_limit_method.md).""",
        parameters="*(none)*",
        returns="""`const AxisState&`
    The state of the axis. The reference is to the pane's own state and follows later calls; copy the value to keep a snapshot.""",
        examples=[
            dict(id="defaults", text="A new axis. Nothing is set by hand: the limits come from the data, the label is empty and every optional field is empty.",
                 code="""auto fig = studio("axis_defaults");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
const AxisState& axis = pane.x_axis();
std::cout << "AutoFit: " << (axis.limit_mode == LimitMode::AutoFit) << ", limits set: " << axis.limits.has_value() << ", label '" << axis.label << "'\\n";
std::cout << "limit method TickAligned: " << (axis.limit_method == LimitMethod::TickAligned) << ", tick labels visible: " << axis.tick_labels_visible << ", grid: " << axis.major_grid << axis.minor_grid << "\\n";
std::cout << "rotation set: " << axis.tick_rotation_deg.has_value() << ", gaps set: " << axis.tick_label_gap.has_value() << axis.label_gap.has_value() << ", ticks by hand: " << axis.ticks.has_value() << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="after_setters", text="After the setters. The state reads back what was set: limits by hand, a label, a log scale on the y axis, and a rotation.",
                 code="""auto fig = studio("axis_after");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{1, 10, 100}, std::vector<double>{1, 10, 100});
pane.x_limits(Range1d{0.0, 120.0}).x_label("size").x_tick_rotation(30.0).scale_y(ScalePolicy{ScaleKind::Log10});
const AxisState& x = pane.x_axis();
std::cout << "x: Manual " << (x.limit_mode == LimitMode::Manual) << ", limits " << x.limits->low << " .. " << x.limits->high << ", label '" << x.label << "', rotation " << *x.tick_rotation_deg << "\\n";
std::cout << "y: Log10 " << (pane.y_axis().scale.kind == ScaleKind::Log10) << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="clear", text="Clearing a setting empties its field again.",
                 code="""auto fig = studio("axis_clear");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
pane.x_tick_rotation(45.0).x_label_gap(12.0);
std::cout << "set: " << pane.x_axis().tick_rotation_deg.has_value() << pane.x_axis().label_gap.has_value() << "\\n";
pane.clear_x_tick_rotation().clear_x_label_gap();
std::cout << "cleared: " << pane.x_axis().tick_rotation_deg.has_value() << pane.x_axis().label_gap.has_value() << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="ticks", text="Ticks given by hand are in the state with their labels.",
                 code="""auto fig = studio("axis_ticks");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
pane.x_ticks(std::vector<double>{0, 1, 2}, {"low", "mid", "high"});
const AxisState& axis = pane.x_axis();
std::cout << "by hand: " << axis.ticks.has_value() << ", labels:";
for (const auto& label : axis.ticks->labels) std::cout << " " << label;
std::cout << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="color", text="The colour axis keeps the range of the colour scale. Its method starts as `Tight`; `color_limits` makes it manual.",
                 code="""auto fig = studio("axis_color");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
std::cout << "method Tight: " << (pane.color_axis().limit_method == LimitMethod::Tight) << ", manual: " << (pane.color_axis().limit_mode == LimitMode::Manual) << "\\n";
pane.color_limits(Range1d{0.0, 5.0});
std::cout << "after color_limits: manual " << (pane.color_axis().limit_mode == LimitMode::Manual) << ", " << pane.color_axis().limits->low << " .. " << pane.color_axis().limits->high << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="z", text="`z_axis` exists on a planar pane too; it simply has no axis to draw, so its settings have no visible effect there.",
                 code="""auto fig = studio("axis_z");
auto& planar = fig.pane2("planar");
planar.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
planar.z_label("never drawn");
std::cout << "z label stored on a planar pane: '" << planar.z_axis().label << "', drawn: " << (fig.to_svg().find(">never drawn<") != std::string::npos) << "\\n";
fig.save_svg("figure.svg");"""),
        ],
        notes="- All four are `noexcept` and read only; change an axis through its setters.\n- The state is returned as it is stored; the range actually drawn (widened by the limit method, snapped to ticks) is worked out when the figure is drawn and is not in it.",
        see=["[`Pane::x_limits`](x_limits.md), [`x_label`](x_label.md), [`x_ticks`](x_ticks.md), [`scale_x`](scale_x.md): the setters, one page each.", "[`Pane::color_limits`](color_limits.md): the colour axis."],
    )
    write("axis_accessors", page)


def place_page():
    base = """auto fig = studio("{n}");
fig.mosaic(2, 2);
auto make = [&](const char* name) -> Pane& {{
  auto& pane = fig.pane2(name);
  pane.trend(std::vector<double>{{0, 1, 2}}, std::vector<double>{{0, 1, 0}});
  return pane;
}};"""
    page = dict(
        slug="place",
        title="Pane::place",
        signature="Pane& Pane::place(std::int64_t row, std::int64_t col)\nPane& Pane::place(std::int64_t row, std::int64_t col, std::int64_t row_span, std::int64_t col_span)\nconst PanePlacement& Pane::placement() const noexcept",
        intro="""Gives the pane a fixed cell, or a block of cells, in the figure's [`mosaic`](../studio/mosaic.md), instead of the next free cell in the order the panes were added. `place(row, col)` takes one cell; `place(row, col, row_span, col_span)` takes `row_span` rows and `col_span` columns whose top-left cell is `(row, col)`, counted from zero. It is the top-level counterpart of [`Studio::subpane2`](../studio/subpane2.md), which places a pane in a grid nested inside another pane.

Panes that are placed claim their cells first. The panes that are not placed then fill the remaining cells in the order they were added, row by row, skipping the claimed ones. So a figure can have a wide pane across the top and ordinary panes below, without numbering every cell.

`placement()` reads the result back: `row`, `col`, `row_span`, `col_span`, and `explicit_slot`, which is `true` once `place` has been called. The other fields (`parent`, `local_rows`, ...) belong to sub-panes.

`place` checks its own arguments: `row` and `col` must not be negative, and the spans must be positive, or it throws. Whether the block fits the mosaic, and whether it overlaps another pane, depends on the mosaic, which may not be declared yet: those are found when the figure is drawn, and the error says which.""",
        parameters="""**row**, **col** (`std::int64_t`)
    The top-left cell, counted from zero. Not negative.

**row_span**, **col_span** (`std::int64_t`, default 1 and 1)
    How many rows and columns the pane covers. Positive.""",
        returns="""**`place`:** `Pane&`, the same pane.
**`placement()`:** `const PanePlacement&`, the slot.""",
        examples=[
            dict(id="one_cell", text="One cell. The pane is put in the lower left of a 2x2 mosaic, where nothing else would put it first.",
                 code=base.format(n="place_one") + """
auto& pane = make("lower left");
pane.place(1, 0);
const auto& slot = pane.placement();
std::cout << "row " << slot.row << ", col " << slot.col << ", spans " << slot.row_span << "x" << slot.col_span << ", explicit " << slot.explicit_slot << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="default", text="Without `place` the pane has no slot of its own: `explicit_slot` is false, and the cell is given when the figure is drawn.",
                 code=base.format(n="place_default") + """
auto& pane = make("auto");
std::cout << "explicit: " << pane.placement().explicit_slot << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="span", text="A span. A pane covering the whole bottom row of a 2x2 mosaic, with the first two panes added above it.",
                 code=base.format(n="place_span") + """
make("top left");
make("top right");
auto& wide = make("bottom, both columns");
wide.place(1, 0, 1, 2);
std::cout << "spans " << wide.placement().row_span << " row(s) and " << wide.placement().col_span << " column(s)\\n";
fig.save_svg("figure.svg");"""),
            dict(id="skip", text="The other panes flow around it. The tall pane is placed on the right column first; the three panes without a slot take the cells left, in the order they were added.",
                 code=base.format(n="place_skip") + """
make("a");
make("b");
make("tall right").place(0, 1, 2, 1);
std::cout << "panes: " << fig.pane_count() << "\\n";
fig.save_svg("figure.svg");""".replace('make("a");\nmake("b");\nmake("tall right").place(0, 1, 2, 1);', 'make("tall right").place(0, 1, 2, 1);\nmake("a");\nmake("b");')),
            dict(id="argument_errors", text="Bad arguments throw at once: a negative row or column, or a span that is zero or negative.",
                 code=base.format(n="place_errors") + """
auto& pane = make("p");
try { pane.place(-1, 0); } catch (const pluss::Error& e) { std::cout << e.message() << "\\n"; }
try { pane.place(0, -2); } catch (const pluss::Error& e) { std::cout << e.message() << "\\n"; }
try { pane.place(0, 0, 0, 1); } catch (const pluss::Error& e) { std::cout << e.message() << "\\n"; }
try { pane.place(0, 0, 1, -1); } catch (const pluss::Error& e) { std::cout << e.message() << "\\n"; }
std::cout << "still explicit: " << pane.placement().explicit_slot << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="outside", text="A slot outside the mosaic is found when the figure is drawn. Here the pane is placed at row 5 of a mosaic with two rows.",
                 code=base.format(n="place_outside") + """
make("p").place(5, 5);
try { fig.to_svg(); } catch (const pluss::Error& e) { std::cout << e.message() << "\\n"; }
auto ok = studio("place_outside_out");
ok.mosaic(2, 2);
ok.pane2("fits").trend(std::vector<double>{0, 1}, std::vector<double>{0, 1});
ok.save_svg("figure.svg");"""),
            dict(id="overlap", text="Two panes claiming the same cell are found when the figure is drawn too: the wide pane covers both cells of the top row, and the second pane is placed in one of them.",
                 code=base.format(n="place_overlap") + """
make("wide").place(0, 0, 1, 2);
make("clash").place(0, 1);
try { fig.to_svg(); } catch (const pluss::Error& e) { std::cout << e.message() << "\\n"; }
auto ok = studio("place_overlap_out");
ok.mosaic(2, 2);
auto& wide = ok.pane2("wide");
wide.trend(std::vector<double>{0, 1}, std::vector<double>{0, 1});
wide.place(0, 0, 1, 2);
auto& below = ok.pane2("below");
below.trend(std::vector<double>{0, 1}, std::vector<double>{1, 0});
below.place(1, 0);
ok.save_svg("figure.svg");"""),
        ],
        notes="- `place` throws `pluss::Error` for a negative cell or a non-positive span; `placement()` is `noexcept`.\n- The checks against the mosaic (fit and overlap) are made when the figure is drawn, so they follow the final size of the mosaic.",
        see=["[`Studio::mosaic`](../studio/mosaic.md): the grid that is filled.", "[`Studio::subpane2`](../studio/subpane2.md), [`subpane3`](../studio/subpane3.md): a nested grid inside a pane.", "[`Studio::pane2`](../studio/pane2.md): makes the pane."],
    )
    write("place", page)


def main():
    legend_page()
    legend_place_page()
    legend_frame_page()
    legend_shape_page()
    grid_page()
    kind_page()
    visible_page()
    axes_visible_page()
    parallel_page()
    y_centered_page()
    scale_guide_page()
    color_limits_page()
    color_limit_method_page()
    tick_font_page()
    axis_label_font_page()
    title_pages()
    caption_pages()
    lens_page()
    lights_page()
    theme_tokens_page()
    axis_accessors_page()
    place_page()
    print("wrote settings specs")


if __name__ == "__main__":
    main()
