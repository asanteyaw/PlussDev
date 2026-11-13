#!/usr/bin/env python3
"""Writes the method specs of the per-axis Pane settings (x_*, y_*, z_*) into tools/method_specs/pane/.

The three axes share one contract, so the prose and the examples come from one template per family; the z axis
is demonstrated on a 3D pane. Run, then `python3 tools/plotdocs.py --methods pane`."""
import pathlib, textwrap

OUT = pathlib.Path(__file__).resolve().parent / "method_specs" / "pane"
NAMES = {"x": "x", "y": "y", "z": "z"}


def _series(kind, n):
    """A deterministic series of length n for the axis that is not under test."""
    if kind == "x":
        return "{" + ", ".join(str(i + 1) for i in range(n)) + "}"
    if kind == "y":
        return "{" + ", ".join(str(2 + (i * 7) % 4) for i in range(n)) + "}"
    return "{" + ", ".join(str(i) for i in range(n)) + "}"


def plot(a, name, vals=None, label=None, indent=""):
    """C++ lines that make `fig` and `pane` with one trace; `vals` replaces the data of axis `a`, and the other
    axes are given series of the same length."""
    n = vals.count(",") + 1 if vals else 4
    base = {"x": _series("x", n), "y": _series("y", n), "z": _series("z", n)}
    if vals:
        base[a] = vals
    if a == "z":
        zeros = "{" + ", ".join(str(i % 2) for i in range(n)) + "}"
        lines = [f'auto fig = studio("{name}");', 'auto& pane = fig.pane3("p");',
                 f'pane.path3(std::vector<double>{{{base["x"][1:-1]}}}, std::vector<double>{zeros}, std::vector<double>{{{base["z"][1:-1]}}});']
    else:
        xs, ys = base["x"], base["y"]
        lines = [f'auto fig = studio("{name}");', 'auto& pane = fig.pane2("p");',
                 f'pane.trend(std::vector<double>{{{xs[1:-1]}}}, std::vector<double>{{{ys[1:-1]}}});']
    return "\n".join(indent + l for l in lines)


def narrow_plot(a, name):
    """Seven points whose values on axis `a` run 0..6, so a narrow range cuts the data."""
    seq, other = "{0, 1, 2, 3, 4, 5, 6}", "{0, 2, 1, 3, 2, 4, 3}"
    if a == "z":
        return f'auto fig = studio("{name}");\nauto& pane = fig.pane3("p");\npane.path3(std::vector<double>{{0, 1, 0, 1, 0, 1, 0}}, std::vector<double>{{0, 1, 0, 1, 0, 1, 0}}, std::vector<double>{seq});'
    xs, ys = (seq, other) if a == "x" else (other, seq)
    return f'auto fig = studio("{name}");\nauto& pane = fig.pane2("p");\npane.trend(std::vector<double>{xs}, std::vector<double>{ys});'


def builder(a, name, body, vals=None, args="bool flag", ret="fig.to_svg()"):
    """A C++ lambda that builds the figure for a comparison and returns the SVG (or a figure)."""
    p = plot(a, name, vals, indent="  ")
    return f"auto build = [&]({args}) {{\n{p}\n  {body}\n  return {ret};\n}};"


def write(slug, page):
    OUT.joinpath(f"{slug}.py").write_text("PAGE = " + repr_page(page) + "\n")


def repr_page(page):
    # readable triple-quoted output, one key per line
    parts = ["dict("]
    for key, val in page.items():
        if key == "examples":
            parts.append("    examples=[")
            for ex in val:
                parts.append(f"        dict(id={ex['id']!r}, text={ex['text']!r},\n             code={ex['code']!r}),")
            parts.append("    ],")
        elif key in ("see",):
            parts.append(f"    see={val!r},")
        else:
            parts.append(f"    {key}={val!r},")
    parts.append(")")
    return "\n".join(parts)


SEE_AXES = lambda fam, a: [f"[`Pane::{b}_{fam}`]({b}_{fam}.md): the same setting for the {b} axis." for b in "xyz" if b != a]

# Texts of an SVG as a list (shared helper pasted into examples that read tick labels).
TEXTS = """auto texts = [](const std::string& svg) {
  std::string out;
  for (std::size_t at = svg.find("<text"); at != std::string::npos; at = svg.find("<text", at + 1)) {
    const auto gt = svg.find('>', at);
    out += svg.substr(gt + 1, svg.find("</text>", gt) - gt - 1) + " ";
  }
  return out;
};"""


def family_label(a):
    A = a
    pane_kind = "3D pane" if a == "z" else "pane"
    ex = [
        dict(id="basic", text=f"Setting the {A} label. The label is stored in `{A}_axis().label` and drawn beside the {A} axis.",
             code=plot(a, f"{A}_label_basic") + f"\nstd::cout << \"before: '\" << pane.{A}_axis().label << \"'\\n\";\npane.{A}_label(\"Elapsed time (s)\");\nstd::cout << \"after: '\" << pane.{A}_axis().label << \"'\\n\";\nstd::cout << \"drawn: \" << (fig.to_svg().find(\">Elapsed time (s)<\") != std::string::npos) << \"\\n\";\nfig.save_svg(\"figure.svg\");"),
        dict(id="replace_clear", text="Replacing and clearing. A second call replaces the text, and an empty string clears it: the figure is then exactly the figure that never had a label.",
             code=f"""auto build = [&](const char* first, const char* second) {{
{plot(a, f"{A}_label_clear", indent="  ")}
  if (first) pane.{A}_label(first);
  if (second) pane.{A}_label(second);
  return fig.to_svg();
}};
std::cout << "replaced: " << (build("old", "new").find(">new<") != std::string::npos) << ", old still drawn: " << (build("old", "new").find(">old<") != std::string::npos) << "\\n";
std::cout << "cleared == never set: " << (build("temporary", "") == build(nullptr, nullptr)) << "\\n";
{plot(a, f"{A}_label_clear_out")}
pane.{A}_label("Final label");
fig.save_svg("figure.svg");"""),
        dict(id="distinct", text=f"Not the title and not a shared caption. The {pane_kind}'s title, its {A} label and a figure-level caption are three separate texts; each is set by its own call and each is drawn in its own place.",
             code=f"""{plot(a, f"{A}_label_distinct")}
pane.title("Pane title");
pane.{A}_label("Axis label");
pane.align_{A}("group");
fig.{A}_caption("group", "Shared caption");
const std::string svg = fig.to_svg();
std::cout << "title: " << (svg.find(">Pane title<") != std::string::npos) << ", label: " << (svg.find(">Axis label<") != std::string::npos) << ", caption: " << (svg.find(">Shared caption<") != std::string::npos) << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="independent", text=f"Per axis. Setting the {A} label leaves the labels of the other axes empty, and each axis keeps its own text.",
             code=f"""{plot(a, f"{A}_label_independent")}
pane.{A}_label("this axis only");
std::cout << "x: '" << pane.x_axis().label << "', y: '" << pane.y_axis().label << "', z: '" << pane.z_axis().label << "'\\n";
pane.x_label("x text").y_label("y text").z_label("z text");
std::cout << "all set: '" << pane.x_axis().label << "', '" << pane.y_axis().label << "', '" << pane.z_axis().label << "'\\n";
fig.save_svg("figure.svg");"""),
        dict(id="math", text="Math text. A label with `$...$` is typeset, so Greek letters and subscripts appear as symbols instead of the raw markup.",
             code=f"""{plot(a, f"{A}_label_math")}
pane.{A}_label("$\\\\sigma_t$ (volatility)");
const std::string svg = fig.to_svg();
std::cout << "raw markup drawn: " << (svg.find("$\\\\sigma_t$") != std::string::npos) << "\\n";
fig.save_svg("figure.svg");"""),
    ]
    return dict(
        slug=f"{A}_label", title=f"Pane::{A}_label", signature=f"Pane& Pane::{A}_label(std::string label)",
        intro=f"""Sets the label of the {A} axis: the text drawn beside the axis that says what it measures. It is separate from the pane's own title ([`Pane::title`](title.md), drawn above the whole plot) and from the shared caption of an alignment group ([`Studio::{A}_caption`](../studio/{A}_caption.md), drawn once for several panes); the three can be used together. The text is stored in `{A}_axis().label`, where it can be read back; there is no separate getter.

The setter takes any text. An empty string clears the label and gives exactly the figure that never had one, a second call replaces the first, and text between dollar signs is typeset as math. The label is read when the figure is rendered, so it can be set before or after the traces. Its distance from the axis is [`{A}_label_gap`]({A}_label_gap.md) and its angle is [`{A}_label_rotation`]({A}_label_rotation.md); the axis tick labels are separate ([`{A}_tick_labels`]({A}_tick_labels.md)).""",
        parameters="""**label** (`std::string`)
    The label text. Empty clears the label.""",
        returns="""`Pane&`
    The same pane, for chaining.""",
        examples=ex,
        notes=f"""- Not `noexcept`: it moves the text into the pane.
- The label of an axis does not change the axis range or its ticks.""",
        see=[f"[`Pane::{A}_label_gap`]({A}_label_gap.md), [`{A}_label_rotation`]({A}_label_rotation.md): where and at what angle it is drawn.", f"[`Studio::{A}_caption`](../studio/{A}_caption.md): one caption shared by aligned panes.", "[`Pane::title`](title.md): the title of the pane.", *SEE_AXES("label", A)],
    )




def only_axis(a):
    """Lines that hide the tick labels of the other axes so only axis `a` prints tick text."""
    others = [b for b in "xyz" if b != a]
    return "\n".join(f"pane.{b}_tick_labels(false);" for b in others)


def gap_like(a, fam, what, field, where):
    A = a
    name = f"{A}_{fam}"
    clear = f"clear_{A}_{fam}"
    ex = [
        dict(id="readback", text=f"Default, set, clear. A new pane has no override (`{field}` is empty); the call stores the value and `clear_{A}_{fam}()` removes it again.",
             code=plot(a, f"{name}_readback") + f"""
auto show = [&] {{ const auto& v = pane.{A}_axis().{field}; std::cout << (v ? std::to_string(*v) : std::string("none")) << "\\n"; }};
std::cout << "new pane: "; show();
pane.{name}(24.0);
std::cout << "after {name}(24): "; show();
pane.{clear}();
std::cout << "after {clear}(): "; show();
fig.save_svg("figure.svg");"""),
        dict(id="effect", text=f"The effect. With a label set, a gap of 0, 14 and 60 gives three different figures; the saved figure uses 60, which pushes {where} well away from the axis.",
             code=f"""auto build = [&](double gap) {{
{plot(a, f"{name}_effect", indent="  ")}
  pane.{A}_label("Axis label");
  pane.{name}(gap);
  return fig;
}};
std::cout << "0 vs 14: " << (build(0.0).to_svg() != build(14.0).to_svg()) << ", 14 vs 60: " << (build(14.0).to_svg() != build(60.0).to_svg()) << "\\n";
build(60.0).save_svg("figure.svg");"""),
        dict(id="clear_restores", text=f"Clearing restores the default exactly. After `{clear}()` the figure equals the one that never set a gap.",
             code=f"""auto build = [&](bool set_then_clear) {{
{plot(a, f"{name}_clear", indent="  ")}
  pane.{A}_label("Axis label");
  if (set_then_clear) pane.{name}(40.0).{clear}();
  return fig.to_svg();
}};
std::cout << "equal: " << (build(true) == build(false)) << "\\n";
{plot(a, f"{name}_clear_out")}
pane.{A}_label("Axis label");
fig.save_svg("figure.svg");"""),
        dict(id="invalid", text=f"Invalid gaps are refused and the stored value is kept. A negative number, infinity and NaN throw `pluss::Error`; zero is a valid gap.",
             code=f"""{plot(a, f"{name}_invalid")}
pane.{A}_label("Axis label");
pane.{name}(10.0);
for (double bad : {{-1.0, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()}}) {{
  try {{
    pane.{name}(bad);
  }} catch (const pluss::Error& e) {{
    std::cout << bad << ": " << e.message() << "\\n";
  }}
}}
pane.{name}(0.0);
std::cout << "zero accepted: " << *pane.{A}_axis().{field} << "\\n";
pane.{name}(10.0);
std::cout << "kept after errors: " << *pane.{A}_axis().{field} << "\\n";
fig.save_svg("figure.svg");"""),
    ]
    return dict(
        slug=name, title=f"Pane::{name}", signature=f"Pane& Pane::{name}(double points)\nPane& Pane::{clear}() noexcept",
        intro=f"""Sets the gap, in drawing units, between {what}. The setting is stored in `{A}_axis().{field}` as an optional value: empty until a gap is set, which means the renderer chooses a gap from the font size. `{name}` stores an explicit gap and `{clear}()` removes it, restoring the automatic choice, so a figure that sets and then clears a gap is exactly the figure that never set one.

The gap must be finite and not negative. Anything else (a negative number, infinity or NaN) is refused with `pluss::Error`, the stored value is left as it was, and zero is valid and puts the two elements as close as the renderer allows. The gap is read when the figure is rendered. It is per axis: the other axes keep their own.""",
        parameters="""**points** (`double`)
    The gap in drawing units. Finite and not negative.""",
        returns="""`Pane&`
    The same pane, for chaining.""",
        examples=ex,
        notes=f"""- `{name}` throws, so it is not `noexcept`; `{clear}` never throws.
- {where.capitalize() if False else ''}The related gap for the other element is [`{A}_{'label_gap' if fam=='tick_label_gap' else 'tick_label_gap'}`]({A}_{'label_gap' if fam=='tick_label_gap' else 'tick_label_gap'}.md).""",
        see=[f"[`Pane::{A}_label`]({A}_label.md): the label the gap is measured to.", f"[`Pane::{A}_{'label_gap' if fam=='tick_label_gap' else 'tick_label_gap'}`]({A}_{'label_gap' if fam=='tick_label_gap' else 'tick_label_gap'}.md): the other gap.", *SEE_AXES(fam, A)],
    )


def family_label_gap(a):
    return gap_like(a, "label_gap", f"the {a} axis (and its tick labels) and the axis label", "label_gap", f"the {a} label")


def family_tick_label_gap(a):
    return gap_like(a, "tick_label_gap", f"the {a} axis line and its tick labels", "tick_label_gap", f"the {a} tick labels")


def rotation_like(a, fam, what, field):
    A = a
    name = f"{A}_{fam}"
    clear = f"clear_{A}_{fam}"
    label_line = f"pane.{A}_label(\"Axis label\");" if fam == "label_rotation" else ""
    ex = [
        dict(id="readback", text=f"Default, set, clear. A new pane has no override (`{field}` is empty); the call stores the angle exactly as given, and `{clear}()` removes it.",
             code=plot(a, f"{name}_readback") + f"""
auto show = [&] {{ const auto& v = pane.{A}_axis().{field}; std::cout << (v ? std::to_string(*v) : std::string("none")) << "\\n"; }};
std::cout << "new pane: "; show();
pane.{name}(45.0);
std::cout << "after {name}(45): "; show();
pane.{clear}();
std::cout << "after {clear}(): "; show();
fig.save_svg("figure.svg");"""),
        dict(id="angles", text="Any finite angle. The values 0, 45, 90, -90 and 720 are all accepted and stored as given, with no normalisation; the saved figure is rotated 45 degrees.",
             code=f"""{plot(a, f"{name}_angles")}
{label_line}
for (double degrees : {{0.0, 45.0, 90.0, -90.0, 720.0}}) {{
  pane.{name}(degrees);
  std::cout << degrees << " stored as " << *pane.{A}_axis().{field} << "\\n";
}}
pane.{name}(45.0);
fig.save_svg("figure.svg");"""),
        dict(id="effect", text=f"The effect. Rotating changes the figure, and the rotated text carries a rotation in the SVG; the angle 0 equals the figure with no override.",
             code=f"""auto build = [&](const char* mode) {{
{plot(a, f"{name}_effect", indent="  ")}
  {label_line}
  if (std::string(mode) == "0") pane.{name}(0.0);
  if (std::string(mode) == "60") pane.{name}(60.0);
  return fig.to_svg();
}};
std::cout << "60 differs from none: " << (build("60") != build("none")) << ", rotate() present: " << (build("60").find("rotate(60") != std::string::npos) << "\\n";
std::cout << "explicit 0 equals none: " << (build("0") == build("none")) << "\\n";
{plot(a, f"{name}_effect_out")}
{label_line}
pane.{name}(60.0);
fig.save_svg("figure.svg");"""),
        dict(id="invalid", text="Only finite angles. Infinity and NaN are refused with `pluss::Error` and the stored angle is kept; every finite number is accepted.",
             code=f"""{plot(a, f"{name}_invalid")}
{label_line}
pane.{name}(30.0);
for (double bad : {{std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()}}) {{
  try {{
    pane.{name}(bad);
  }} catch (const pluss::Error& e) {{
    std::cout << bad << ": " << e.message() << "\\n";
  }}
}}
std::cout << "kept: " << *pane.{A}_axis().{field} << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="clear_restores", text=f"Clearing restores the default. After `{clear}()` the figure equals the one that never set an angle.",
             code=f"""auto build = [&](bool set_then_clear) {{
{plot(a, f"{name}_clear", indent="  ")}
  {label_line}
  if (set_then_clear) pane.{name}(75.0).{clear}();
  return fig.to_svg();
}};
std::cout << "equal: " << (build(true) == build(false)) << "\\n";
{plot(a, f"{name}_clear_out")}
{label_line}
fig.save_svg("figure.svg");"""),
    ]
    return dict(
        slug=name, title=f"Pane::{name}", signature=f"Pane& Pane::{name}(double degrees)\nPane& Pane::{clear}() noexcept",
        intro=f"""Rotates {what} by an explicit angle in degrees. The angle is stored in `{A}_axis().{field}` as an optional value: empty until set, which means the renderer chooses the angle for the axis. `{name}` stores the angle and `{clear}()` removes it, restoring the automatic choice, so a figure that sets and then clears an angle is exactly the figure that never set one.

The angle must be finite. Infinity and NaN are refused with `pluss::Error` and the stored value is left as it was; every finite number is stored as given, so 90, -90 and 720 are all accepted and are not reduced to a range. It is per axis, and it is read when the figure is rendered.""",
        parameters="""**degrees** (`double`)
    The angle in degrees. Finite.""",
        returns="""`Pane&`
    The same pane, for chaining.""",
        examples=ex,
        notes=f"""- `{name}` throws, so it is not `noexcept`; `{clear}` never throws.
- Rotated text needs room: the gap between the axis and its labels is set with [`{A}_{'label_gap' if fam=='label_rotation' else 'tick_label_gap'}`]({A}_{'label_gap' if fam=='label_rotation' else 'tick_label_gap'}.md).""",
        see=[f"[`Pane::{A}_{'tick_rotation' if fam=='label_rotation' else 'label_rotation'}`]({A}_{'tick_rotation' if fam=='label_rotation' else 'label_rotation'}.md): the other rotation.", *SEE_AXES(fam, A)],
    )


def family_label_rotation(a):
    return rotation_like(a, "label_rotation", f"the {a} axis label", "label_rotation_deg")


def family_tick_rotation(a):
    return rotation_like(a, "tick_rotation", f"the tick labels of the {a} axis", "tick_rotation_deg")


FAMILIES_BASE = {"label": family_label, "label_gap": family_label_gap, "tick_label_gap": family_tick_label_gap,
            "label_rotation": family_label_rotation, "tick_rotation": family_tick_rotation}
FAMILIES = dict(FAMILIES_BASE)


def family_limits(a):
    A = a
    ex = [
        dict(id="readback", text=f"Default, manual, back to automatic. A new axis fits its range to the data (`limit_mode` is `AutoFit`, 0); `{A}_limits` stores the range and switches to `Manual` (1); `auto_{A}()` returns to automatic.",
             code=plot(a, f"{A}_limits_readback") + f"""
auto show = [&] {{
  const auto& axis = pane.{A}_axis();
  std::cout << "mode " << static_cast<int>(axis.limit_mode) << ", limits " << (axis.limits ? std::to_string(axis.limits->low) + " .. " + std::to_string(axis.limits->high) : std::string("none")) << "\\n";
}};
std::cout << "new axis: "; show();
pane.{A}_limits(Range1d{{-2.0, 10.0}});
std::cout << "after {A}_limits: "; show();
pane.auto_{A}();
std::cout << "after auto_{A}: "; show();
pane.{A}_limits(Range1d{{-2.0, 10.0}});
fig.save_svg("figure.svg");"""),
        dict(id="effect", text="The effect on the picture. The range {-2, 10} is much wider than the data, so the data occupies part of the axis and the ticks follow the range; the figure differs from the automatic one.",
             code=f"""auto build = [&](bool manual) {{
{plot(a, f"{A}_limits_effect", indent="  ")}
  {only_axis(a).replace(chr(10), chr(10)+'  ')}
  if (manual) pane.{A}_limits(Range1d{{-2.0, 10.0}});
  return fig.to_svg();
}};
{TEXTS}
std::cout << "automatic ticks: " << texts(build(false)) << "\\n";
std::cout << "manual ticks:    " << texts(build(true)) << "\\n";
{plot(a, f"{A}_limits_effect_out")}
pane.{A}_limits(Range1d{{-2.0, 10.0}});
fig.save_svg("figure.svg");"""),
        dict(id="narrower", text="A range narrower than the data. Only the part of the data inside the range is shown; the rest is cut at the edge of the plot area and nothing is drawn outside it.",
             code=narrow_plot(a, f"{A}_limits_narrow") + f"""
pane.{A}_limits(Range1d{{1.0, 3.0}});
std::cout << "range: " << pane.{A}_axis().limits->low << " .. " << pane.{A}_axis().limits->high << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="auto", text=f"Back to automatic. After `auto_{A}()` the figure equals the one that never set a range, and the limit method (`{A}_limit_method`) applies again.",
             code=f"""auto build = [&](bool set_then_auto) {{
{plot(a, f"{A}_limits_auto", indent="  ")}
  if (set_then_auto) pane.{A}_limits(Range1d{{-50.0, 50.0}}).auto_{A}();
  return fig.to_svg();
}};
std::cout << "equal: " << (build(true) == build(false)) << "\\n";
{plot(a, f"{A}_limits_auto_out")}
fig.save_svg("figure.svg");"""),
        dict(id="invalid", text="Invalid ranges are refused and the previous range is kept. The range must be finite and strictly increasing, so a reversed range, an empty one and a non-finite one each throw `pluss::Error`.",
             code=f"""{plot(a, f"{A}_limits_invalid")}
pane.{A}_limits(Range1d{{0.0, 5.0}});
const double inf = std::numeric_limits<double>::infinity();
const Range1d bad[] = {{{{5.0, 0.0}}, {{3.0, 3.0}}, {{0.0, inf}}, {{std::numeric_limits<double>::quiet_NaN(), 1.0}}}};
for (const Range1d& range : bad) {{
  try {{
    pane.{A}_limits(range);
  }} catch (const pluss::Error& e) {{
    std::cout << range.low << " .. " << range.high << ": " << e.message() << "\\n";
  }}
}}
std::cout << "kept: " << pane.{A}_axis().limits->low << " .. " << pane.{A}_axis().limits->high << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="log", text=f"With a log scale the range must be positive. After `scale_{A}(log10)` a range that includes zero throws, and switching to `log10` while such a range is set throws as well; the axis keeps its previous state in both cases.",
             code=f"""{plot(a, f"{A}_limits_log", vals="{1, 10, 100, 1000}")}
pane.scale_{A}(ScalePolicy::log10());
try {{
  pane.{A}_limits(Range1d{{0.0, 100.0}});
}} catch (const pluss::Error& e) {{
  std::cout << "after log10: " << e.message() << "\\n";
}}
pane.{A}_limits(Range1d{{1.0, 1000.0}});
std::cout << "positive range accepted: " << pane.{A}_axis().limits->low << " .. " << pane.{A}_axis().limits->high << "\\n";
auto other = studio("{A}_limits_log_order");
auto& flat = other.pane2("flat");
flat.trend(std::vector<double>{{1, 2, 3}}, std::vector<double>{{2, 4, 3}});
flat.{A}_limits(Range1d{{-1.0, 5.0}});
try {{
  flat.scale_{A}(ScalePolicy::log10());
}} catch (const pluss::Error& e) {{
  std::cout << "limits first, then log10: " << e.message() << "\\n";
}}
fig.save_svg("figure.svg");"""),
    ]
    return dict(
        slug=f"{A}_limits", title=f"Pane::{A}_limits", signature=f"Pane& Pane::{A}_limits(Range1d limits)\nPane& Pane::auto_{A}() noexcept",
        intro=f"""Sets the range the {A} axis displays, overriding the automatic fit to the data. The range is stored in `{A}_axis().limits` and the axis switches from `LimitMode::AutoFit` to `LimitMode::Manual`; the data is then drawn against exactly that range, and the part of it outside the range is cut at the edge of the plot area. [`auto_{A}`](auto_{A}.md) is the way back: it discards the range and returns to automatic fitting, which is then governed by [`{A}_limit_method`]({A}_limit_method.md); on an axis that is already automatic it does nothing.

The range must be finite on both ends and strictly increasing (`low < high`): a reversed, empty or non-finite range throws `pluss::Error` and the previous range is kept. With a logarithmic scale ([`scale_{A}`](scale_{A}.md) with `ScalePolicy::log10()`) both ends must also be positive; the same rule is applied the other way round when the scale is switched to log10 with a manual range already set. The call takes effect when the figure is rendered, so it can precede or follow the traces.""",
        parameters="""**limits** (`Range1d`)
    The displayed range, `{low, high}`. Finite and strictly increasing; positive for a log scale.""",
        returns="""`Pane&`
    The same pane, for chaining.""",
        examples=ex,
        notes=f"""- `{A}_limits` throws, so it is not `noexcept`; `auto_{A}` never throws.
- A manual range ignores [`{A}_limit_method`]({A}_limit_method.md) completely; the method only shapes the automatic range.""",
        see=[f"[`Pane::auto_{A}`](auto_{A}.md): back to automatic.", f"[`Pane::{A}_limit_method`]({A}_limit_method.md): how the automatic range is chosen.", f"[`Pane::scale_{A}`](scale_{A}.md): linear, log and symmetric-log axes.", *SEE_AXES("limits", A)],
    )


def family_limit_method(a):
    A = a
    ex = [
        dict(id="readback", text="The default is `TickAligned` (0). `Tight` is 1 and `Padded` is 2; each call is read back, and the last call wins.",
             code=plot(a, f"{A}_method_readback") + f"""
std::cout << "new axis: " << static_cast<int>(pane.{A}_axis().limit_method) << "\\n";
for (LimitMethod m : {{LimitMethod::Tight, LimitMethod::Padded, LimitMethod::TickAligned}}) {{
  pane.{A}_limit_method(m);
  std::cout << "set " << static_cast<int>(m) << ", read " << static_cast<int>(pane.{A}_axis().limit_method) << "\\n";
}}
fig.save_svg("figure.svg");"""),
        dict(id="tick_aligned", text="`TickAligned`, the default. The range is widened to round tick values, so the axis starts and ends on clean numbers; the ticks of the data 2.3 to 6.2 run from 2 to 7.",
             code=f"""{plot(a, f"{A}_method_aligned", vals="{2.3, 4.7, 3.1, 6.2}")}
{only_axis(a)}
pane.{A}_limit_method(LimitMethod::TickAligned);
{TEXTS}
std::cout << texts(fig.to_svg()) << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="tight", text="`Tight`. The range hugs the data, so the data touches the edges of the plot area and the end ticks sit inside it.",
             code=f"""{plot(a, f"{A}_method_tight", vals="{2.3, 4.7, 3.1, 6.2}")}
{only_axis(a)}
pane.{A}_limit_method(LimitMethod::Tight);
{TEXTS}
std::cout << texts(fig.to_svg()) << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="padded", text="`Padded`. A margin is added around the data, so no point sits on the edge; the three methods give three different figures.",
             code=f"""auto build = [&](LimitMethod method) {{
{plot(a, f"{A}_method_padded", vals="{2.3, 4.7, 3.1, 6.2}", indent="  ")}
  pane.{A}_limit_method(method);
  return fig;
}};
std::cout << "TickAligned vs Tight: " << (build(LimitMethod::TickAligned).to_svg() != build(LimitMethod::Tight).to_svg()) << ", Tight vs Padded: " << (build(LimitMethod::Tight).to_svg() != build(LimitMethod::Padded).to_svg()) << "\\n";
build(LimitMethod::Padded).save_svg("figure.svg");"""),
        dict(id="manual", text="No effect with a manual range. Once `limits` sets a range the method is never consulted: the three methods give byte-identical figures.",
             code=f"""auto build = [&](LimitMethod method) {{
{plot(a, f"{A}_method_manual", indent="  ")}
  pane.{A}_limits(Range1d{{0.0, 10.0}});
  pane.{A}_limit_method(method);
  return fig.to_svg();
}};
std::cout << "all equal: " << (build(LimitMethod::TickAligned) == build(LimitMethod::Tight) && build(LimitMethod::Tight) == build(LimitMethod::Padded)) << "\\n";
{plot(a, f"{A}_method_manual_out")}
fig.save_svg("figure.svg");"""),
    ]
    return dict(
        slug=f"{A}_limit_method", title=f"Pane::{A}_limit_method", signature=f"Pane& Pane::{A}_limit_method(LimitMethod method) noexcept",
        intro=f"""Chooses how the {A} axis range is computed while it is fitted automatically, that is, while no range has been set with [`{A}_limits`]({A}_limits.md). `LimitMethod::TickAligned` (the default) widens the data range to round tick values so the axis starts and ends on clean numbers; `LimitMethod::Tight` uses the data range as it is, so the data touches the edges; `LimitMethod::Padded` adds a margin around the data so that no point lies on the edge. See [`LimitMethod`](../types/LimitMethod.md).

The method is stored in `{A}_axis().limit_method` and is kept when the axis is switched between manual and automatic, so [`auto_{A}`](auto_{A}.md) returns to the method that was chosen. With a manual range the method has no effect at all. The call never throws.""",
        parameters="""**method** (`LimitMethod`)
    `TickAligned`, `Tight` or `Padded`.""",
        returns="""`Pane&`
    The same pane, for chaining.""",
        examples=ex,
        notes=f"""- `noexcept`. The method concerns the automatic range only; the displayed range of a manual axis is exactly the range that was set.""",
        see=[f"[`Pane::{A}_limits`]({A}_limits.md): a manual range, which overrides the method.", f"[`Pane::auto_{A}`](auto_{A}.md): back to automatic fitting.", "[`LimitMethod`](../types/LimitMethod.md): the values.", *SEE_AXES("limit_method", A)],
    )


FAMILIES.update({"limits": family_limits, "limit_method": family_limit_method})


def only_data(a, name, vals):
    """plot() with only the tick text of axis `a` left in the SVG."""
    return plot(a, name, vals) + "\n" + only_axis(a)


def family_tick_format(a):
    A = a
    fmts = [("Auto", "chooses plain or scientific notation from the size of the numbers"), ("Plain", "plain decimal numbers with no trailing zeros"),
            ("Fixed3", "three decimals"), ("Fixed6", "six decimals"), ("Scientific", "scientific notation, `2.00e+06`"),
            ("Engineering", "engineering notation, exponents that are multiples of three"), ("Integer", "rounded to whole numbers"), ("Percent", "the value times 100 with a percent sign")]
    table = "\n".join(f"| `{n}` | {d}. |" for n, d in fmts)
    ex = [
        dict(id="readback", text="The default is `Auto` (0); each call is read back, and the last call wins.",
             code=plot(a, f"{A}_format_readback") + f"""
std::cout << "new axis: " << static_cast<int>(pane.{A}_axis().tick_format) << " (Auto = 0)\\n";
pane.{A}_tick_format(TickFormat::Percent);
std::cout << "after Percent: " << static_cast<int>(pane.{A}_axis().tick_format) << "\\n";
pane.{A}_tick_format(TickFormat::Auto);
std::cout << "back to Auto: " << static_cast<int>(pane.{A}_axis().tick_format) << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="small", text="Every format on the same small values. The axis runs 0 to 1, and each format writes the ticks in its own way; `Percent` multiplies by 100.",
             code=f"""{TEXTS}
const std::pair<const char*, TickFormat> formats[] = {{{{"Auto", TickFormat::Auto}}, {{"Plain", TickFormat::Plain}}, {{"Fixed3", TickFormat::Fixed3}}, {{"Fixed6", TickFormat::Fixed6}},
                                                    {{"Scientific", TickFormat::Scientific}}, {{"Engineering", TickFormat::Engineering}}, {{"Integer", TickFormat::Integer}}, {{"Percent", TickFormat::Percent}}}};
for (const auto& [name, format] : formats) {{
{plot(a, f"{A}_format_small", vals="{0.0, 0.25, 0.5, 1.0}").replace(chr(10), chr(10)+'  ')}
  {only_axis(a)}
  pane.{A}_tick_format(format);
  std::cout << name << ": " << texts(fig.to_svg()) << "\\n";
}}
{plot(a, f"{A}_format_small_out", vals="{0.0, 0.25, 0.5, 1.0}")}
pane.{A}_tick_format(TickFormat::Percent);
fig.save_svg("figure.svg");"""),
        dict(id="large", text="Every format on large values. The axis runs to about ten million; `Plain` and `Fixed3` write every digit, `Scientific` and `Engineering` keep the labels short, and `Auto` switches to scientific notation by itself.",
             code=f"""{TEXTS}
const std::pair<const char*, TickFormat> formats[] = {{{{"Auto", TickFormat::Auto}}, {{"Plain", TickFormat::Plain}}, {{"Fixed3", TickFormat::Fixed3}}, {{"Scientific", TickFormat::Scientific}}, {{"Engineering", TickFormat::Engineering}}, {{"Integer", TickFormat::Integer}}}};
for (const auto& [name, format] : formats) {{
{plot(a, f"{A}_format_large", vals="{1200, 34000, 560000, 7800000, 9000000}").replace(chr(10), chr(10)+'  ')}
  {only_axis(a)}
  pane.{A}_tick_format(format);
  std::cout << name << ": " << texts(fig.to_svg()) << "\\n";
}}
{plot(a, f"{A}_format_large_out", vals="{1200, 34000, 560000, 7800000, 9000000}")}
pane.{A}_tick_format(TickFormat::Scientific);
fig.save_svg("figure.svg");"""),
        dict(id="distinct", text="Eight formats, eight figures. The formats that look alike on some data differ on others; here all pairs of neighbouring formats on the small values give different figures, and setting the same format twice changes nothing.",
             code=f"""auto build = [&](TickFormat format) {{
{plot(a, f"{A}_format_distinct", vals="{0.0, 0.25, 0.5, 1.0}", indent="  ")}
  pane.{A}_tick_format(format);
  return fig.to_svg();
}};
std::cout << "Plain vs Fixed3: " << (build(TickFormat::Plain) != build(TickFormat::Fixed3)) << ", Fixed3 vs Fixed6: " << (build(TickFormat::Fixed3) != build(TickFormat::Fixed6)) << ", Scientific vs Engineering: " << (build(TickFormat::Scientific) != build(TickFormat::Engineering)) << "\\n";
std::cout << "same format twice: " << (build(TickFormat::Percent) == build(TickFormat::Percent)) << "\\n";
{plot(a, f"{A}_format_distinct_out", vals="{0.0, 0.25, 0.5, 1.0}")}
fig.save_svg("figure.svg");"""),
        dict(id="explicit_ticks", text="Explicit tick labels keep their text. Labels given with `{A}_ticks` are drawn as written, whatever the format; the format applies to the automatic ticks.".replace("{A}", A),
             code=f"""{TEXTS}
{plot(a, f"{A}_format_explicit", vals="{1, 2, 3, 4}")}
{only_axis(a)}
pane.{A}_ticks(std::vector<double>{{1, 2, 3}}, {{"low", "mid", "high"}});
pane.{A}_tick_format(TickFormat::Percent);
std::cout << texts(fig.to_svg()) << "\\n";
fig.save_svg("figure.svg");"""),
    ]
    return dict(
        slug=f"{A}_tick_format", title=f"Pane::{A}_tick_format", signature=f"Pane& Pane::{A}_tick_format(TickFormat format) noexcept",
        intro=f"""Chooses how the numbers on the {A} axis ticks are written. The format is stored in `{A}_axis().tick_format`; `TickFormat::Auto` (the default) picks plain or scientific notation from the size of the numbers, and the other formats force one style:

| Format | Ticks are written as |
|---|---|
{table}

The format shapes the automatic tick labels. Labels given explicitly with [`{A}_ticks`]({A}_ticks.md) or [`{A}_categories`]({A}_categories.md) are text, and are drawn as they were written. The number of decimals of the automatic labels in `Auto` is set with [`{A}_auto_tick_decimals`]({A}_auto_tick_decimals.md). The call never throws, and is read when the figure is rendered.""",
        parameters="""**format** (`TickFormat`)
    One of the eight formats; see [`TickFormat`](../types/TickFormat.md).""",
        returns="""`Pane&`
    The same pane, for chaining.""",
        examples=ex,
        notes=f"""- `noexcept`. A format changes only how ticks are written, never where they are.
- The format is per axis: set it on each axis that should use it.""",
        see=[f"[`Pane::{A}_auto_tick_decimals`]({A}_auto_tick_decimals.md): decimals of the automatic labels.", f"[`Pane::{A}_ticks`]({A}_ticks.md): explicit tick labels.", "[`TickFormat`](../types/TickFormat.md): the values.", *SEE_AXES("tick_format", A)],
    )


def family_auto_tick_decimals(a):
    A = a
    ex = [
        dict(id="readback", text="Default, set, clear. A new axis has no setting (`auto_tick_decimals` is empty); the call stores the count and `clear_{A}_auto_tick_decimals()` removes it.".replace("{A}", A),
             code=plot(a, f"{A}_decimals_readback") + f"""
auto show = [&] {{ const auto& v = pane.{A}_axis().auto_tick_decimals; std::cout << (v ? std::to_string(*v) : std::string("none")) << "\\n"; }};
std::cout << "new axis: "; show();
pane.{A}_auto_tick_decimals(2);
std::cout << "after {A}_auto_tick_decimals(2): "; show();
pane.clear_{A}_auto_tick_decimals();
std::cout << "after clear: "; show();
fig.save_svg("figure.svg");"""),
        dict(id="effect", text="The effect. On an axis from 0 to 1 the automatic labels have one decimal by default; with 0, 1, 2 and 5 decimals they are written with that many.",
             code=f"""{TEXTS}
for (int decimals : {{0, 1, 2, 5}}) {{
{plot(a, f"{A}_decimals_effect", vals="{0.0, 0.1234, 0.25, 0.5, 1.0}").replace(chr(10), chr(10)+'  ')}
  {only_axis(a)}
  pane.{A}_auto_tick_decimals(decimals);
  std::cout << decimals << " decimals: " << texts(fig.to_svg()) << "\\n";
}}
{plot(a, f"{A}_decimals_effect_out", vals="{0.0, 0.1234, 0.25, 0.5, 1.0}")}
pane.{A}_auto_tick_decimals(2);
fig.save_svg("figure.svg");"""),
        dict(id="scientific", text="Not when the axis is scientific. Values of millions are written in scientific notation by `Auto`, and a number of decimals does not change that; a plain-number format is the way to write them out.",
             code=f"""{TEXTS}
auto build = [&](bool decimals) {{
{plot(a, f"{A}_decimals_sci", vals="{1200, 34000, 560000, 7800000, 9000000}", indent="  ")}
  {only_axis(a)}
  if (decimals) pane.{A}_auto_tick_decimals(4);
  return fig.to_svg();
}};
std::cout << "no setting: " << texts(build(false)) << "\\n";
std::cout << "4 decimals: " << texts(build(true)) << "\\n";
{plot(a, f"{A}_decimals_sci_out", vals="{1200, 34000, 560000, 7800000, 9000000}")}
fig.save_svg("figure.svg");"""),
        dict(id="invalid", text="Only 0 to 8 decimals. A negative count and a count above 8 are refused with `pluss::Error`, and the stored count is kept; 0 and 8 are valid.",
             code=f"""{plot(a, f"{A}_decimals_invalid")}
pane.{A}_auto_tick_decimals(3);
for (int bad : {{-1, 9, 100}}) {{
  try {{
    pane.{A}_auto_tick_decimals(bad);
  }} catch (const pluss::Error& e) {{
    std::cout << bad << ": " << e.message() << "\\n";
  }}
}}
pane.{A}_auto_tick_decimals(0);
pane.{A}_auto_tick_decimals(8);
std::cout << "0 and 8 accepted, stored: " << *pane.{A}_axis().auto_tick_decimals << "\\n";
pane.{A}_auto_tick_decimals(3);
std::cout << "kept after errors: " << *pane.{A}_axis().auto_tick_decimals << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="clear_restores", text="Clearing restores the default exactly. After `clear_{A}_auto_tick_decimals()` the figure equals the one that never set a count.".replace("{A}", A),
             code=f"""auto build = [&](bool set_then_clear) {{
{plot(a, f"{A}_decimals_clear", vals="{0.0, 0.25, 0.5, 1.0}", indent="  ")}
  if (set_then_clear) pane.{A}_auto_tick_decimals(5).clear_{A}_auto_tick_decimals();
  return fig.to_svg();
}};
std::cout << "equal: " << (build(true) == build(false)) << "\\n";
{plot(a, f"{A}_decimals_clear_out", vals="{0.0, 0.25, 0.5, 1.0}")}
fig.save_svg("figure.svg");"""),
    ]
    return dict(
        slug=f"{A}_auto_tick_decimals", title=f"Pane::{A}_auto_tick_decimals", signature=f"Pane& Pane::{A}_auto_tick_decimals(int decimals)\nPane& Pane::clear_{A}_auto_tick_decimals() noexcept",
        intro=f"""Sets how many decimals the automatic tick labels of the {A} axis are written with, for the axis that uses plain numbers. Without a setting the renderer picks the fewest decimals that tell the ticks apart; with one, every automatic label has exactly that many, so a column of ticks lines up. The count is stored in `{A}_axis().auto_tick_decimals` (empty when unset), and `clear_{A}_auto_tick_decimals()` removes it, giving exactly the figure that never set it.

The count must be from 0 to 8; anything else throws `pluss::Error` and the stored count is kept. The setting shapes automatic labels in plain notation only. An axis whose numbers are written in scientific notation ([`{A}_tick_format`]({A}_tick_format.md) `Auto` on millions, or `Scientific`) is not changed by it, and explicit labels ([`{A}_ticks`]({A}_ticks.md)) are text.""",
        parameters="""**decimals** (`int`)
    The number of decimals, 0 to 8.""",
        returns="""`Pane&`
    The same pane, for chaining.""",
        examples=ex,
        notes=f"""- `{A}_auto_tick_decimals` throws, so it is not `noexcept`; `clear_{A}_auto_tick_decimals` never throws.""",
        see=[f"[`Pane::{A}_tick_format`]({A}_tick_format.md): the notation of the labels.", f"[`Pane::{A}_ticks`]({A}_ticks.md): explicit labels.", *SEE_AXES("auto_tick_decimals", A)],
    )


def family_tick_labels(a):
    A = a
    ex = [
        dict(id="readback", text="Visible by default. `tick_labels_visible` is true for a new axis, `false` hides the labels, and `true` (the default argument) shows them again.",
             code=plot(a, f"{A}_labels_readback") + f"""
std::cout << "new axis: " << pane.{A}_axis().tick_labels_visible << "\\n";
pane.{A}_tick_labels(false);
std::cout << "after false: " << pane.{A}_axis().tick_labels_visible << "\\n";
pane.{A}_tick_labels();
std::cout << "after the default argument: " << pane.{A}_axis().tick_labels_visible << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="effect", text="The effect. With the labels hidden the axis has no tick text; the texts of the figure show the ticks of this axis present and then absent.",
             code=f"""{TEXTS}
auto build = [&](bool visible) {{
{plot(a, f"{A}_labels_effect", indent="  ")}
  {only_axis(a)}
  pane.{A}_tick_labels(visible);
  return fig.to_svg();
}};
std::cout << "visible: " << texts(build(true)) << "\\n";
std::cout << "hidden:  " << texts(build(false)) << "\\n";
{plot(a, f"{A}_labels_effect_out")}
pane.{A}_tick_labels(false);
fig.save_svg("figure.svg");"""),
        dict(id="restore", text="Showing them again restores the figure exactly: hiding and then showing gives the figure that never changed the setting.",
             code=f"""auto build = [&](bool hide_then_show) {{
{plot(a, f"{A}_labels_restore", indent="  ")}
  if (hide_then_show) pane.{A}_tick_labels(false).{A}_tick_labels(true);
  return fig.to_svg();
}};
std::cout << "equal: " << (build(true) == build(false)) << "\\n";
{plot(a, f"{A}_labels_restore_out")}
fig.save_svg("figure.svg");"""),
        dict(id="explicit", text="Explicit tick labels are hidden too. Labels set with `{A}_ticks` are tick labels like any other, so `{A}_tick_labels(false)` hides them while the label of the axis (`{A}_label`) stays.".replace("{A}", A),
             code=f"""{TEXTS}
{plot(a, f"{A}_labels_explicit", vals="{1, 2, 3, 4}")}
{only_axis(a)}
pane.{A}_ticks(std::vector<double>{{1, 2, 3}}, {{"low", "mid", "high"}});
pane.{A}_label("Axis label");
std::cout << "shown: " << texts(fig.to_svg()) << "\\n";
pane.{A}_tick_labels(false);
std::cout << "hidden: " << texts(fig.to_svg()) << "\\n";
fig.save_svg("figure.svg");"""),
    ]
    return dict(
        slug=f"{A}_tick_labels", title=f"Pane::{A}_tick_labels", signature=f"Pane& Pane::{A}_tick_labels(bool enable = true) noexcept",
        intro=f"""Shows or hides the tick labels of the {A} axis, the numbers or words written beside its ticks. They are visible by default. With `false` the labels are not drawn, whether they are the automatic numbers or explicit text set with [`{A}_ticks`]({A}_ticks.md) or [`{A}_categories`]({A}_categories.md); the axis label ([`{A}_label`]({A}_label.md)), the axis line and the grid are not affected, and the axis keeps its range and its ticks. `true` shows them again, exactly as before. The state is stored in `{A}_axis().tick_labels_visible`.

It is per axis, it never throws, and it is read when the figure is rendered.""",
        parameters="""**enable** (`bool`, default `true`)
    `false` hides the tick labels, `true` shows them.""",
        returns="""`Pane&`
    The same pane, for chaining.""",
        examples=ex,
        notes=f"""- `noexcept`. To hide the whole axis including its line, use [`Pane::axes_visible`](axes_visible.md).""",
        see=[f"[`Pane::{A}_label`]({A}_label.md): the axis label, which stays.", f"[`Pane::{A}_ticks`]({A}_ticks.md): explicit tick labels.", "[`Pane::axes_visible`](axes_visible.md): hides all axes.", *SEE_AXES("tick_labels", A)],
    )


FAMILIES.update({"tick_format": family_tick_format, "auto_tick_decimals": family_auto_tick_decimals, "tick_labels": family_tick_labels})


def family_ticks(a):
    A = a
    ex = [
        dict(id="labels", text=f"Explicit labels at chosen positions. Three ticks are placed at 1, 2 and 3 with words for labels; the automatic ticks of this axis are replaced by them.",
             code=f"""{TEXTS}
{plot(a, f"{A}_ticks_labels", vals="{1, 2, 3, 4}")}
{only_axis(a)}
std::cout << "automatic: " << texts(fig.to_svg()) << "\\n";
pane.{A}_ticks(std::vector<double>{{1, 2, 3}}, {{"low", "mid", "high"}});
std::cout << "explicit:  " << texts(fig.to_svg()) << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="labels_from_strings", text="Labels from a list of strings. A `std::vector<std::string_view>` of labels works as well as the braced list, and the positions can be any numeric vector.",
             code=f"""{TEXTS}
{plot(a, f"{A}_ticks_strings", vals="{1, 2, 3, 4}")}
{only_axis(a)}
const std::vector<std::string_view> names = {{"Q1", "Q2", "Q3", "Q4"}};
std::vector<double> positions;
for (std::size_t i = 0; i < names.size(); ++i) positions.push_back(1.0 + static_cast<double>(i));
pane.{A}_ticks(positions, pluss::msharp::RangeView::Of<std::string_view>(names));
std::cout << texts(fig.to_svg()) << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="series", text="Positions as a `Series`. The positions can be passed as a Series, which is what the library's numeric results are; the labels are drawn in order of the positions given.",
             code=f"""{TEXTS}
{plot(a, f"{A}_ticks_series", vals="{1, 2, 3, 4}")}
{only_axis(a)}
auto positions = pluss::make_series(std::vector<double>{{1.0, 4.0}}, {{2}});
pane.{A}_ticks(positions, std::vector<std::string_view>{{"first", "last"}});
std::cout << texts(fig.to_svg()) << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="outside", text="Only ticks inside the range are drawn. The tick at 10 lies outside the axis range and the tick at 0 is below it, so only the middle one is shown; widening the range with `limits` brings them in.",
             code=f"""{TEXTS}
auto build = [&](bool wide) {{
{plot(a, f"{A}_ticks_outside", vals="{1, 2, 3, 4}", indent="  ")}
  {only_axis(a).replace(chr(10), chr(10)+'  ')}
  pane.{A}_ticks(std::vector<double>{{0, 2, 10}}, {{"zero", "two", "ten"}});
  if (wide) pane.{A}_limits(Range1d{{0.0, 10.0}});
  return fig.to_svg();
}};
std::cout << "automatic range: " << texts(build(false)) << "\\n";
std::cout << "range 0 to 10:   " << texts(build(true)) << "\\n";
{plot(a, f"{A}_ticks_outside_out", vals="{1, 2, 3, 4}")}
pane.{A}_ticks(std::vector<double>{{0, 2, 10}}, {{"zero", "two", "ten"}});
pane.{A}_limits(Range1d{{0.0, 10.0}});
fig.save_svg("figure.svg");"""),
        dict(id="clear", text=f"Clearing. `clear_{A}_ticks()` removes the explicit ticks and the automatic ones come back; the figure equals the one that never set any.",
             code=f"""auto build = [&](bool set_then_clear) {{
{plot(a, f"{A}_ticks_clear", indent="  ")}
  if (set_then_clear) pane.{A}_ticks(std::vector<double>{{1, 2}}, {{"a", "b"}}).clear_{A}_ticks();
  return fig.to_svg();
}};
std::cout << "equal: " << (build(true) == build(false)) << "\\n";
{plot(a, f"{A}_ticks_clear_out")}
fig.save_svg("figure.svg");"""),
        dict(id="invalid", text="Invalid specifications are refused and the previous ticks are kept. Empty positions, positions and labels of different lengths, and positions that are not a vector each throw `pluss::Error`.",
             code=f"""{plot(a, f"{A}_ticks_invalid")}
pane.{A}_ticks(std::vector<double>{{1, 2}}, {{"a", "b"}});
auto attempt = [&](const char* label, auto call) {{
  try {{
    call();
  }} catch (const pluss::Error& e) {{
    std::cout << label << ": " << e.message() << "\\n";
  }}
}};
attempt("no positions", [&] {{ pane.{A}_ticks(std::vector<double>{{}}, {{}}); }});
attempt("length mismatch", [&] {{ pane.{A}_ticks(std::vector<double>{{1, 2}}, {{"only one"}}); }});
attempt("not a vector", [&] {{ pane.{A}_ticks(pluss::make_series(std::vector<double>{{1, 2, 3, 4}}, {{2, 2}}), {{"a", "b"}}); }});
std::cout << "kept: " << pane.{A}_axis().ticks->labels.size() << " labels\\n";
fig.save_svg("figure.svg");"""),
    ]
    return dict(
        slug=f"{A}_ticks", title=f"Pane::{A}_ticks", signature=f"Pane& Pane::{A}_ticks(Series positions, RangeView::Of<std::string_view> labels)\nPane& Pane::{A}_ticks(Series positions, std::initializer_list<std::string_view> labels)\nPane& Pane::clear_{A}_ticks() noexcept",
        intro=f"""Replaces the automatic ticks of the {A} axis with ticks you choose: a position for each tick and the text drawn beside it. Positions are in the data units of the axis and need not be sorted; labels are drawn as written, so they can be words, dates or formatted numbers. The ticks are stored in `{A}_axis().ticks`, and the format of the numbers ([`{A}_tick_format`]({A}_tick_format.md), [`{A}_auto_tick_decimals`]({A}_auto_tick_decimals.md)) does not apply to them. A tick whose position lies outside the displayed range is not drawn, so a manual range ([`{A}_limits`]({A}_limits.md)) decides which of the ticks appear.

There are three call forms: a braced list of labels, a range of string views, and a `Series` of positions; positions given as a plain numeric vector are converted. `clear_{A}_ticks()` removes the explicit ticks and restores the automatic ones, exactly. A call is refused with `pluss::Error`, leaving the previous ticks as they were, when the positions are empty or are not a vector, or when the number of labels differs from the number of positions.""",
        parameters="""**positions** (`Series` or numeric vector)
    Where the ticks go, in data units. A non-empty vector.

**labels** (`RangeView::Of<std::string_view>` or braced list)
    The text of each tick; as many as there are positions.""",
        returns="""`Pane&`
    The same pane, for chaining.""",
        examples=ex,
        notes=f"""- `{A}_ticks` throws, so it is not `noexcept`; `clear_{A}_ticks` never throws.
- For labels at the integers 1, 2, 3, ... use [`{A}_categories`]({A}_categories.md), which builds the positions for you.""",
        see=[f"[`Pane::{A}_categories`]({A}_categories.md): labels at evenly spaced positions.", f"[`Pane::{A}_tick_labels`]({A}_tick_labels.md): hide the tick labels.", f"[`Pane::{A}_limits`]({A}_limits.md): the range that decides which ticks show.", *SEE_AXES("ticks", A)],
    )


def family_categories(a):
    A = a
    ex = [
        dict(id="basic", text="Category names on the integers. Four labels with the defaults (`first = 1`, `step = 1`) sit at 1, 2, 3 and 4.",
             code=f"""{TEXTS}
{plot(a, f"{A}_categories_basic", vals="{1, 2, 3, 4}")}
{only_axis(a)}
pane.{A}_categories({{"north", "south", "east", "west"}});
std::cout << texts(fig.to_svg()) << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="first_step", text="Another origin and spacing. `first = 10` and `step = 5` put three labels at 10, 15 and 20, which is where the data is.",
             code=f"""{TEXTS}
{plot(a, f"{A}_categories_step", vals="{10, 15, 20}")}
{only_axis(a)}
pane.{A}_categories({{"low", "mid", "high"}}, 10.0, 5.0);
std::cout << texts(fig.to_svg()) << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="descending", text="A negative step runs the other way. The labels are placed at 3, 2 and 1, so the first name is at the highest position.",
             code=f"""{TEXTS}
{plot(a, f"{A}_categories_desc", vals="{3, 2, 1}")}
{only_axis(a)}
pane.{A}_categories({{"c", "b", "a"}}, 3.0, -1.0);
std::cout << texts(fig.to_svg()) << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="same_as_ticks", text=f"The same as `{A}_ticks`. `{A}_categories` computes the positions and calls `{A}_ticks`, so the two give the same figure.",
             code=f"""auto build = [&](bool through_ticks) {{
{plot(a, f"{A}_categories_same", vals="{1, 2, 3}", indent="  ")}
  if (through_ticks) pane.{A}_ticks(std::vector<double>{{1.0, 2.0, 3.0}}, {{"a", "b", "c"}});
  else pane.{A}_categories({{"a", "b", "c"}});
  return fig.to_svg();
}};
std::cout << "equal: " << (build(true) == build(false)) << "\\n";
{plot(a, f"{A}_categories_same_out", vals="{1, 2, 3}")}
pane.{A}_categories({{"a", "b", "c"}});
fig.save_svg("figure.svg");"""),
        dict(id="invalid", text="Invalid specifications are refused. No labels, a step of zero, and a non-finite first or step each throw `pluss::Error`, and the previous ticks are kept.",
             code=f"""{plot(a, f"{A}_categories_invalid")}
pane.{A}_categories({{"a", "b"}});
auto attempt = [&](const char* label, auto call) {{
  try {{
    call();
  }} catch (const pluss::Error& e) {{
    std::cout << label << ": " << e.message() << "\\n";
  }}
}};
attempt("no labels", [&] {{ pane.{A}_categories({{}}); }});
attempt("step zero", [&] {{ pane.{A}_categories({{"a"}}, 1.0, 0.0); }});
attempt("first not finite", [&] {{ pane.{A}_categories({{"a"}}, std::numeric_limits<double>::quiet_NaN(), 1.0); }});
attempt("step infinite", [&] {{ pane.{A}_categories({{"a"}}, 1.0, std::numeric_limits<double>::infinity()); }});
std::cout << "kept: " << pane.{A}_axis().ticks->labels.size() << " labels\\n";
fig.save_svg("figure.svg");"""),
    ]
    return dict(
        slug=f"{A}_categories", title=f"Pane::{A}_categories", signature=f"Pane& Pane::{A}_categories(RangeView::Of<std::string_view> labels, double first = 1.0, double step = 1.0)\nPane& Pane::{A}_categories(std::initializer_list<std::string_view> labels, double first = 1.0, double step = 1.0)",
        intro=f"""Labels the {A} axis with names instead of numbers: the first label goes at position `first`, the next at `first + step`, and so on. It is the form of [`{A}_ticks`]({A}_ticks.md) for categorical data (bars of a bar chart, groups of a box plot) where the positions are evenly spaced, and it produces exactly the ticks that `{A}_ticks` would with those positions, replacing the automatic ones; [`clear_{A}_ticks`]({A}_ticks.md) removes them.

The defaults put the labels at 1, 2, 3, ..., which is where the bar and box traces draw their categories. A negative `step` runs the labels downwards. Labels cannot be empty, and `first` and `step` must be finite with a non-zero `step`; otherwise the call throws `pluss::Error` and the previous ticks are kept. Ticks that fall outside the displayed range are not drawn.""",
        parameters="""**labels** (`RangeView::Of<std::string_view>` or braced list)
    The category names, at least one.

**first** (`double`, default 1)
    The position of the first label. Finite.

**step** (`double`, default 1)
    The distance between labels. Finite and not zero.""",
        returns="""`Pane&`
    The same pane, for chaining.""",
        examples=ex,
        notes=f"""- Not `noexcept`: it throws for invalid arguments.
- There is no `clear_{A}_categories`; categories are ticks, so use `clear_{A}_ticks`.""",
        see=[f"[`Pane::{A}_ticks`]({A}_ticks.md): labels at arbitrary positions.", f"[`Pane::{A}_tick_labels`]({A}_tick_labels.md): hide the tick labels.", *SEE_AXES("categories", A)],
    )


def grid_family(a, which):
    A = a
    name = f"{A}_{which}_grid"
    other = "minor" if which == "major" else "major"
    ex = [
        dict(id="readback", text=f"Off by default. `{A}_axis().{which}_grid` is false for a new axis; the call with no argument turns it on, `false` turns it off, and the last call wins.",
             code=plot(a, f"{name}_readback") + f"""
std::cout << "new axis: " << pane.{A}_axis().{which}_grid << "\\n";
pane.{name}();
std::cout << "after {name}(): " << pane.{A}_axis().{which}_grid << "\\n";
pane.{name}(false);
std::cout << "after {name}(false): " << pane.{A}_axis().{which}_grid << "\\n";
pane.{name}(true);
fig.save_svg("figure.svg");"""),
        dict(id="effect", text=f"The effect. Turning the {which} grid on adds grid lines to the figure, which can be counted as extra paths in the SVG; turning it off again restores the figure exactly.",
             code=f"""auto build = [&](bool on) {{
{plot(a, f"{name}_effect", indent="  ")}
  pane.{name}(on);
  return fig.to_svg();
}};
auto paths = [](const std::string& svg) {{ std::size_t n = 0; for (auto at = svg.find("<path"); at != std::string::npos; at = svg.find("<path", at + 1)) ++n; return n; }};
std::cout << "paths without: " << paths(build(false)) << ", with: " << paths(build(true)) << "\\n";
std::cout << "off again equals never set: " << (build(false) == build(false)) << "\\n";
{plot(a, f"{name}_effect_out")}
pane.{name}();
fig.save_svg("figure.svg");"""),
        dict(id="per_axis", text=f"Per axis. The {A} grid does not turn on the grid of the other axes, and each axis has its own switches.",
             code=f"""{plot(a, f"{name}_axes")}
pane.{name}();
std::cout << "x: " << pane.x_axis().{which}_grid << ", y: " << pane.y_axis().{which}_grid << ", z: " << pane.z_axis().{which}_grid << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="summary", text=f"The pane's grid summary. `Pane::grid_style()` reports `None` (0), `Major` (1) or `MajorMinor` (2) from the axis switches; the {which} grid of this axis moves it as shown.",
             code=f"""{plot(a, f"{name}_summary")}
std::cout << "no grid: " << static_cast<int>(pane.grid_style()) << "\\n";
pane.{A}_major_grid();
std::cout << "major on: " << static_cast<int>(pane.grid_style()) << "\\n";
pane.{A}_minor_grid();
std::cout << "major and minor on: " << static_cast<int>(pane.grid_style()) << "\\n";
pane.{name}(true);
fig.save_svg("figure.svg");"""),
        dict(id="independent", text=f"Independent of the {other} grid. The {which} grid can be on while the {other} grid is off; the counts show how many paths each combination draws.",
             code=f"""auto paths = [](const std::string& svg) {{ std::size_t n = 0; for (auto at = svg.find("<path"); at != std::string::npos; at = svg.find("<path", at + 1)) ++n; return n; }};
auto build = [&](bool major, bool minor) {{
{plot(a, f"{name}_independent", indent="  ")}
  pane.{A}_major_grid(major).{A}_minor_grid(minor);
  return fig.to_svg();
}};
std::cout << "none: " << paths(build(false, false)) << ", major only: " << paths(build(true, false)) << ", minor only: " << paths(build(false, true)) << ", both: " << paths(build(true, true)) << "\\n";
{plot(a, f"{name}_independent_out")}
pane.{A}_major_grid().{A}_minor_grid();
fig.save_svg("figure.svg");"""),
    ]
    which_text = "major tick positions" if which == "major" else "positions between the major ticks"
    return dict(
        slug=name, title=f"Pane::{name}", signature=f"Pane& Pane::{name}(bool enable = true) noexcept",
        intro=f"""Turns the {which} grid of the {A} axis on or off. The {which} grid is the set of lines drawn across the plot at the {which_text} of the {A} axis; it is off by default. `true` (the default argument) turns it on, `false` turns it off, and the state is stored in `{A}_axis().{which}_grid`. Turning it off again gives exactly the figure that never turned it on.

The two grids of an axis are independent: the minor grid can be on with the major grid off. Each axis has its own, and [`Pane::grid`](grid.md) sets all of them at once; [`Pane::grid_style`](grid_style.md) summarises the state of the whole pane as `None`, `Major` or `MajorMinor`. The call never throws and is read when the figure is rendered.""",
        parameters="""**enable** (`bool`, default `true`)
    `true` draws the grid, `false` removes it.""",
        returns="""`Pane&`
    The same pane, for chaining.""",
        examples=ex,
        notes=f"""- `noexcept`.
- To turn the grid of every axis on or off in one call, use [`Pane::grid`](grid.md).""",
        see=[f"[`Pane::{A}_{other}_grid`]({A}_{other}_grid.md): the {other} grid of the same axis.", "[`Pane::grid`](grid.md): all axes at once.", "[`Pane::grid_style`](grid_style.md): the summary of the grid.", *SEE_AXES(f"{which}_grid", A)],
    )


def family_major_grid(a):
    return grid_family(a, "major")


def family_minor_grid(a):
    return grid_family(a, "minor")


FAMILIES.update({"ticks": family_ticks, "categories": family_categories, "major_grid": family_major_grid, "minor_grid": family_minor_grid})


def family_scale(a):
    A = a
    ex = [
        dict(id="readback", text="Linear by default. A new axis reports `Linear` (0); `log10` is 1 and `symlog` is 2; each call is read back and the last wins.",
             code=plot(a, f"scale_{A}_readback") + f"""
std::cout << "new axis: " << static_cast<int>(pane.{A}_axis().scale.kind) << " (Linear = 0)\\n";
pane.scale_{A}(ScalePolicy::log10());
std::cout << "log10: " << static_cast<int>(pane.{A}_axis().scale.kind) << "\\n";
pane.scale_{A}(ScalePolicy::symlog(10.0, 1.0));
std::cout << "symlog: " << static_cast<int>(pane.{A}_axis().scale.kind) << ", base " << pane.{A}_axis().scale.base << ", threshold " << pane.{A}_axis().scale.linear_threshold << "\\n";
pane.scale_{A}(ScalePolicy::linear());
std::cout << "linear again: " << static_cast<int>(pane.{A}_axis().scale.kind) << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="log", text="A logarithmic axis. Data spanning three decades is spread evenly on a log10 axis and the ticks are written as powers of ten; the same data on the default linear axis crowds the small values together.",
             code=f"""{TEXTS}
auto build = [&](bool log) {{
{plot(a, f"scale_{A}_log", vals="{1, 10, 100, 1000}", indent="  ")}
  {only_axis(a).replace(chr(10), chr(10)+'  ')}
  if (log) pane.scale_{A}(ScalePolicy::log10());
  return fig.to_svg();
}};
std::cout << "linear ticks: " << texts(build(false)) << "\\n";
std::cout << "log10 ticks:  " << texts(build(true)) << "\\n";
{plot(a, f"scale_{A}_log_out", vals="{1, 10, 100, 1000}")}
pane.scale_{A}(ScalePolicy::log10());
fig.save_svg("figure.svg");"""),
        dict(id="symlog", text="A symmetric-log axis. Values on both sides of zero, from -100 to 100, are shown logarithmically beyond the linear threshold and linearly inside it, so negative values, zero and large values share one axis.",
             code=f"""{TEXTS}
{plot(a, f"scale_{A}_symlog", vals="{-100, -1, 0, 1, 100}")}
{only_axis(a)}
pane.scale_{A}(ScalePolicy::symlog());
std::cout << texts(fig.to_svg()) << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="symlog_parameters", text="The symmetric-log parameters. The base and the linear threshold shape the curve; changing either changes the figure, and the stored policy reads back what was given.",
             code=f"""auto build = [&](double base, double threshold) {{
{plot(a, f"scale_{A}_symlog_params", vals="{-100, -10, 0, 10, 100}", indent="  ")}
  pane.scale_{A}(ScalePolicy::symlog(base, threshold));
  return fig.to_svg();
}};
std::cout << "base 10 vs 2: " << (build(10.0, 1.0) != build(2.0, 1.0)) << ", threshold 1 vs 10: " << (build(10.0, 1.0) != build(10.0, 10.0)) << "\\n";
{plot(a, f"scale_{A}_symlog_params_out", vals="{-100, -10, 0, 10, 100}")}
pane.scale_{A}(ScalePolicy::symlog(10.0, 10.0));
std::cout << "stored: base " << pane.{A}_axis().scale.base << ", threshold " << pane.{A}_axis().scale.linear_threshold << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="same_as_default", text="Linear is the default. Setting `ScalePolicy::linear()` gives exactly the figure of an axis that never set a scale, and switching back from log restores it.",
             code=f"""auto build = [&](int mode) {{
{plot(a, f"scale_{A}_default", vals="{1, 10, 100, 1000}", indent="  ")}
  if (mode == 1) pane.scale_{A}(ScalePolicy::linear());
  if (mode == 2) pane.scale_{A}(ScalePolicy::log10()).scale_{A}(ScalePolicy::linear());
  return fig.to_svg();
}};
std::cout << "linear() == default: " << (build(1) == build(0)) << ", log then linear == default: " << (build(2) == build(0)) << "\\n";
{plot(a, f"scale_{A}_default_out", vals="{1, 10, 100, 1000}")}
fig.save_svg("figure.svg");"""),
        dict(id="invalid", text="Invalid policies are refused and the previous scale is kept. A symmetric-log base that is not greater than 1 (or not finite) and a threshold that is not positive throw `pluss::Error`; so does switching to log10 while a manual range that includes zero is set.",
             code=f"""{plot(a, f"scale_{A}_invalid", vals="{1, 10, 100, 1000}")}
auto attempt = [&](const char* label, auto call) {{
  try {{
    call();
  }} catch (const pluss::Error& e) {{
    std::cout << label << ": " << e.message() << "\\n";
  }}
}};
attempt("base 1", [&] {{ pane.scale_{A}(ScalePolicy::symlog(1.0, 1.0)); }});
attempt("base 0.5", [&] {{ pane.scale_{A}(ScalePolicy::symlog(0.5, 1.0)); }});
attempt("threshold 0", [&] {{ pane.scale_{A}(ScalePolicy::symlog(10.0, 0.0)); }});
attempt("threshold -1", [&] {{ pane.scale_{A}(ScalePolicy::symlog(10.0, -1.0)); }});
attempt("base nan", [&] {{ pane.scale_{A}(ScalePolicy::symlog(std::numeric_limits<double>::quiet_NaN(), 1.0)); }});
pane.{A}_limits(Range1d{{0.0, 100.0}});
attempt("log10 over a range that includes zero", [&] {{ pane.scale_{A}(ScalePolicy::log10()); }});
std::cout << "scale kept: " << static_cast<int>(pane.{A}_axis().scale.kind) << " (Linear = 0)\\n";
fig.save_svg("figure.svg");"""),
    ]
    return dict(
        slug=f"scale_{A}", title=f"Pane::scale_{A}", signature=f"Pane& Pane::scale_{A}(ScalePolicy scale)",
        intro=f"""Sets how values are spread along the {A} axis. `ScalePolicy::linear()` (the default) spaces values evenly; `ScalePolicy::log10()` spaces them by their base-10 logarithm, so equal ratios take equal distances and the ticks are powers of ten, which suits data across several orders of magnitude; `ScalePolicy::symlog(base, threshold)` is logarithmic beyond `threshold` and linear inside it, symmetric about zero, so it can show negative values, zero and large values on one axis. The policy is stored in `{A}_axis().scale` and is read when the figure is rendered.

A policy is refused with `pluss::Error`, leaving the previous scale in place, when it cannot be drawn: a symmetric-log `base` must be finite and greater than 1, its `linear_threshold` finite and positive, and `log10` cannot be combined with a manual range ([`{A}_limits`]({A}_limits.md)) that includes zero or negative values. The reverse order is refused too: with `log10` active, a range that is not strictly positive is rejected by `{A}_limits`.""",
        parameters="""**scale** (`ScalePolicy`)
    `ScalePolicy::linear()`, `ScalePolicy::log10()` or `ScalePolicy::symlog(base = 10, linear_threshold = 1)`.""",
        returns="""`Pane&`
    The same pane, for chaining.""",
        examples=ex,
        notes=f"""- Not `noexcept`: it throws for invalid policies.
- Data that cannot be placed on the scale (zero or negative values on a log axis) does not stretch the range; use `symlog` for data that crosses zero.""",
        see=[f"[`Pane::{A}_limits`]({A}_limits.md): a manual range, which must be positive on a log axis.", f"[`Pane::{A}_tick_format`]({A}_tick_format.md): how tick numbers are written.", "[`ScalePolicy`](../types/ScalePolicy.md): the policies and their parameters.", *[f"[`Pane::scale_{b}`](scale_{b}.md): the same setting for the {b} axis." for b in "xyz" if b != A]],
    )


FAMILIES.update({"scale": family_scale})


PATHS = """auto paths = [](const std::string& svg) {
  std::size_t n = 0;
  for (auto at = svg.find("<path"); at != std::string::npos; at = svg.find("<path", at + 1)) ++n;
  return n;
};"""


def family_rule(a):
    A = a
    plane = "volume" if a == "z" else "plane"
    ex = [
        dict(id="basic", text=f"A labelled reference line. A rule at {A} = 2 with the label `target`; it is stored with its axis, value and label, and the label is drawn on the line.",
             code=plot(a, f"rule_{A}_basic", vals="{1, 2, 3, 4}" if a != "y" else "{2, 5, 4, 3}") + f"""
pane.rule_{A}(2.0, "target");
const auto& rule = pane.rules().front();
std::cout << "stored: axis " << static_cast<int>(rule.axis) << ", value " << rule.value << ", label '" << rule.label << "', width " << rule.width << "\\n";
std::cout << "label drawn: " << (fig.to_svg().find(">target<") != std::string::npos) << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="no_label", text="Without a label. The label is optional: the line is drawn and no extra text appears, and the figure has one more path than the one without the rule.",
             code=f"""{PATHS}
auto build = [&](bool with_rule) {{
{plot(a, f"rule_{A}_nolabel", indent="  ")}
  if (with_rule) pane.rule_{A}(2.0);
  return fig.to_svg();
}};
std::cout << "paths without: " << paths(build(false)) << ", with: " << paths(build(true)) << "\\n";
{plot(a, f"rule_{A}_nolabel_out")}
pane.rule_{A}(2.0);
fig.save_svg("figure.svg");"""),
        dict(id="several", text="Several rules. Each call adds a rule; they are kept in the order added and each is drawn, so a pane can carry a baseline and thresholds together.",
             code=f"""{plot(a, f"rule_{A}_several")}
pane.rule_{A}(1.5, "low").rule_{A}(2.5, "mid").rule_{A}(3.5, "high");
std::cout << "rules: " << pane.rules().size() << ":";
for (const auto& r : pane.rules()) std::cout << " " << r.label << "@" << r.value;
std::cout << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="range", text=f"Rules and the {A} range. A rule far outside the data widens an automatic range so it stays visible; with a manual range the axis stays where it was set, and a rule outside that range is not drawn at all.",
             code=f"""{TEXTS}
auto build = [&](bool manual) {{
{plot(a, f"rule_{A}_range", indent="  ")}
  {only_axis(a).replace(chr(10), chr(10)+'  ')}
  if (manual) pane.{A}_limits(Range1d{{0.0, 5.0}});
  pane.rule_{A}(40.0, "far");
  return fig.to_svg();
}};
std::cout << "automatic range: " << texts(build(false)) << "\\n";
std::cout << "manual 0 to 5:   " << texts(build(true)) << "\\n";
{plot(a, f"rule_{A}_range_out")}
pane.rule_{A}(40.0, "far");
fig.save_svg("figure.svg");"""),
        dict(id="invalid", text="Only finite values. A NaN or infinite position throws `pluss::Error` and no rule is added.",
             code=f"""{plot(a, f"rule_{A}_invalid")}
for (double bad : {{std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()}}) {{
  try {{
    pane.rule_{A}(bad, "never");
  }} catch (const pluss::Error& e) {{
    std::cout << bad << ": " << e.message() << "\\n";
  }}
}}
std::cout << "rules added: " << pane.rules().size() << "\\n";
fig.save_svg("figure.svg");"""),
    ]
    extra = ""
    if a == "z":
        ex.append(dict(id="planar", text="On a planar pane. A pane without a third axis has nowhere to draw a z rule: the call is accepted and the rule is stored, and the figure is the same as without it.",
             code=f"""auto build = [](bool with_rule) {{
  auto fig = studio("rule_z_planar");
  auto& pane = fig.pane2("flat");
  pane.trend(std::vector<double>{{1, 2, 3, 4}}, std::vector<double>{{2, 4, 3, 5}});
  if (with_rule) pane.rule_z(1.0, "ignored");
  return fig.to_svg();
}};
std::cout << "same figure: " << (build(true) == build(false)) << "\\n";
auto fig = studio("rule_z_planar_out");
fig.pane2("flat").trend(std::vector<double>{{1, 2, 3, 4}}, std::vector<double>{{2, 4, 3, 5}});
fig.save_svg("figure.svg");"""))
        extra = " On a planar pane a z rule is stored but has no axis to be drawn on."
    return dict(
        slug=f"rule_{A}", title=f"Pane::rule_{A}", signature=f"Pane& Pane::rule_{A}(double value, std::string label = {{}})",
        intro=f"""Adds a reference line at {A} = `value`: a rule drawn across the {plane} at that position, with an optional `label` written on it. Rules mark thresholds, targets and baselines without being data: they are not traces, they are not in the legend, and they take the colours of the pane's theme. Each call adds one rule; they are kept in the order added and listed by [`rules`](rules.md).

The position must be finite; a NaN or infinite value throws `pluss::Error` and nothing is added. A rule takes part in the automatic range of its axis, so a rule outside the data widens the axis to keep it visible, while a manual range ([`{A}_limits`]({A}_limits.md)) is never changed by a rule, and a rule outside it is not drawn. The rule is drawn when the figure is rendered, so it can be added before or after the traces.{extra}""",
        parameters="""**value** (`double`)
    The position of the rule, in data units. Finite.

**label** (`std::string`, default empty)
    Text written on the rule. Empty draws no text.""",
        returns="""`Pane&`
    The same pane, for chaining.""",
        examples=ex,
        notes=f"""- Not `noexcept`: it throws for a non-finite value.
- A rule marks one position; for a range use [`band_{A}`](band_{A}.md).""",
        see=[f"[`Pane::band_{A}`](band_{A}.md): a shaded range.", "[`Pane::rules`](rules.md): the rules of the pane.", "[`Pane::pin`](pin.md): a note attached to a point.", *[f"[`Pane::rule_{b}`](rule_{b}.md): the same for the {b} axis." for b in "xyz" if b != A]],
    )


def family_band(a):
    A = a
    plane = "volume" if a == "z" else "plane"
    ex = [
        dict(id="basic", text=f"A shaded range. A band from {A} = 2 to 3 with the label `target zone`; it is stored with its axis, span and label, and the label is drawn on the band.",
             code=plot(a, f"band_{A}_basic") + f"""
pane.band_{A}(Range1d{{2.0, 3.0}}, "target zone");
const auto& band = pane.bands().front();
std::cout << "stored: axis " << static_cast<int>(band.axis) << ", span " << band.span.low << " .. " << band.span.high << ", label '" << band.label << "'\\n";
std::cout << "label drawn: " << (fig.to_svg().find(">target zone<") != std::string::npos) << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="no_label", text="Without a label. The label is optional; the band is drawn without text and adds shapes to the figure.",
             code=f"""{PATHS}
auto build = [&](bool with_band) {{
{plot(a, f"band_{A}_nolabel", indent="  ")}
  if (with_band) pane.band_{A}(Range1d{{2.0, 3.0}});
  return fig.to_svg();
}};
std::cout << "paths without: " << paths(build(false)) << ", with: " << paths(build(true)) << "\\n";
{plot(a, f"band_{A}_nolabel_out")}
pane.band_{A}(Range1d{{2.0, 3.0}});
fig.save_svg("figure.svg");"""),
        dict(id="several", text="Several bands. Bands are kept in the order added and each is drawn, so a pane can show a normal range and a warning range together.",
             code=f"""{plot(a, f"band_{A}_several")}
pane.band_{A}(Range1d{{1.0, 2.0}}, "low").band_{A}(Range1d{{2.5, 3.5}}, "high");
std::cout << "bands: " << pane.bands().size() << ":";
for (const auto& b : pane.bands()) std::cout << " " << b.label << " [" << b.span.low << ", " << b.span.high << "]";
std::cout << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="range", text=f"Bands and the {A} range. A band far outside the data widens an automatic range so it stays visible; with a manual range the axis stays where it was set, a band partly inside is trimmed to the range, and one entirely outside is not drawn.",
             code=f"""{TEXTS}
auto build = [&](bool manual) {{
{plot(a, f"band_{A}_range", indent="  ")}
  {only_axis(a).replace(chr(10), chr(10)+'  ')}
  if (manual) pane.{A}_limits(Range1d{{0.0, 5.0}});
  pane.band_{A}(Range1d{{30.0, 40.0}}, "far");
  return fig.to_svg();
}};
std::cout << "automatic range: " << texts(build(false)) << "\\n";
std::cout << "manual 0 to 5:   " << texts(build(true)) << "\\n";
{plot(a, f"band_{A}_range_out")}
pane.band_{A}(Range1d{{30.0, 40.0}}, "far");
fig.save_svg("figure.svg");"""),
        dict(id="invalid", text="The span must be finite and strictly increasing. A reversed span, an empty one and a NaN end each throw `pluss::Error`, and no band is added.",
             code=f"""{plot(a, f"band_{A}_invalid")}
const double nan = std::numeric_limits<double>::quiet_NaN();
const Range1d bad[] = {{{{3.0, 2.0}}, {{2.0, 2.0}}, {{nan, 2.0}}}};
for (const Range1d& span : bad) {{
  try {{
    pane.band_{A}(span, "never");
  }} catch (const pluss::Error& e) {{
    std::cout << span.low << " .. " << span.high << ": " << e.message() << "\\n";
  }}
}}
std::cout << "bands added: " << pane.bands().size() << "\\n";
fig.save_svg("figure.svg");"""),
    ]
    return dict(
        slug=f"band_{A}", title=f"Pane::band_{A}", signature=f"Pane& Pane::band_{A}(Range1d span, std::string label = {{}})",
        intro=f"""Adds a shaded band across the {plane} from {A} = `span.low` to `span.high`, with an optional `label`. A band marks a range (an acceptable tolerance, a recession, a season) the way a [rule](rule_{A}.md) marks a position: it is not a trace and is not in the legend, and it takes its colours from the pane's theme. Each call adds one band; they are kept in order and listed by [`bands`](bands.md).

The span must be finite and strictly increasing (`low < high`); anything else throws `pluss::Error` and nothing is added. A band takes part in the automatic range of its axis, so one outside the data widens the axis to keep it visible. A manual range ([`{A}_limits`]({A}_limits.md)) is never changed by it: a band partly inside the range is trimmed to it, and one entirely outside is not drawn. It is drawn when the figure is rendered.""",
        parameters="""**span** (`Range1d`)
    The range, `{low, high}`, in data units. Finite and strictly increasing.

**label** (`std::string`, default empty)
    Text written on the band. Empty draws no text.""",
        returns="""`Pane&`
    The same pane, for chaining.""",
        examples=ex,
        notes=f"""- Not `noexcept`: it throws for an invalid span.""",
        see=[f"[`Pane::rule_{A}`](rule_{A}.md): a single position.", "[`Pane::bands`](bands.md): the bands of the pane.", *[f"[`Pane::band_{b}`](band_{b}.md): the same for the {b} axis." for b in "xyz" if b != A]],
    )


FAMILIES.update({"rule": family_rule, "band": family_band})


def main():
    import sys
    for fam, fn in FAMILIES.items():
        for a in "xyz":
            page = fn(a)
            write(page["slug"], page)
    print("wrote", len(FAMILIES) * 3, "specs")


if __name__ == "__main__":
    main()
