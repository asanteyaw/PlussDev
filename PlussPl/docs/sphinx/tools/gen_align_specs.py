#!/usr/bin/env python3
"""Writes the method specs of the Pane alignment-group family (align_x/y/z, align_xy/xyz, clear_align, the group
accessors) into tools/method_specs/pane/. Run, then `python3 tools/plotdocs.py --methods pane`."""
import sys, pathlib
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from gen_axis_specs import write, TEXTS

LAYOUT = {"x": "fig.mosaic(2, 1);", "y": "fig.mosaic(1, 2);", "z": "fig.mosaic(1, 2);"}


def member(a, var, name, scale, indent=""):
    """C++ lines for one pane whose `a` axis spans 0..3*scale and whose other tick labels are hidden."""
    vals = "{" + ", ".join(str(i * scale) for i in range(4)) + "}"
    other = "{0, 1, 0, 1}"
    steps = "{0, 1, 2, 3}"
    if a == "x":
        body = f'auto& {var} = fig.pane2("{name}");\n{var}.trend(std::vector<double>{vals}, std::vector<double>{other});\n{var}.y_tick_labels(false);'
    elif a == "y":
        body = f'auto& {var} = fig.pane2("{name}");\n{var}.trend(std::vector<double>{steps}, std::vector<double>{vals});\n{var}.x_tick_labels(false);'
    else:
        body = (f'auto& {var} = fig.pane3("{name}");\n{var}.path3(std::vector<double>{other}, std::vector<double>{steps}, std::vector<double>{vals});\n'
                f'{var}.x_tick_labels(false);\n{var}.y_tick_labels(false);')
    return "\n".join(indent + l for l in body.splitlines())


def two_panes(a, group=None, indent=""):
    """`fig` with a small-range pane `small` and a large-range pane `large`, optionally in one group."""
    lines = [f'auto fig = studio("align_{a}");', LAYOUT[a], member(a, "small", "small", 1), member(a, "large", "large", 10)]
    if group:
        lines += [f'small.align_{a}("{group}");', f'large.align_{a}("{group}");']
    return "\n".join(indent + l for l in "\n".join(lines).splitlines())


def axis_page(a):
    A = a
    kind = "3D panes" if a == "z" else "panes"
    page = dict(
        slug=f"align_{a}",
        title=f"Pane::align_{a}",
        signature=f"Pane& Pane::align_{a}(std::string group)\nPane& Pane::clear_{a}_align() noexcept",
        intro=f"""Puts the {A} axis of this pane into a named **alignment group**, and `clear_{a}_align()` takes it out again. Panes whose {A} axes are in the same group are drawn as one: they share the same {A} range, and a figure-level caption can be written once for the whole group.

What a group does, in full:

| Effect | When |
|---|---|
| The panes share one {A} range, so equal values sit at equal ticks in every pane. | Always. The range is the union of the ranges the members would have on their own. |
| If a member sets its {A} limits by hand ([`{A}_limits`]({A}_limits.md)), the group shares the union of the hand-set limits instead, and the data of the other members is clipped to it. | A hand-set limit is the choice of window; the data of another pane never overrides it. |
| One caption is drawn for the group by [`Studio::{A}_caption`](../studio/{A}_caption.md). | The caption's group name matches and at least one member is visible. |

The group is only a name: any string is accepted, and a name that no other pane uses makes a group of one. The match is exact and case-sensitive. The empty string means "no group", which is the state of a new pane. The group ties the panes in no other way: titles, the other axes, the format of ticks and the grid stay each pane's own.

Joining a group does not change the settings of the axis. Leaving it (`clear_{a}_align()` or `align_{a}("")`) restores the range the pane has on its own.""",
        parameters=f"""**group** (`std::string`, setter only)
    The name of the group. Panes with the same name share the {A} range. An empty string leaves the group.""",
        returns=f"""`Pane&`
    The same pane, for chaining. Both calls return it; `clear_{a}_align()` is `noexcept`.""",
        examples=[
            dict(id="share", text=f"Sharing a range. Two {kind} with {A} ranges 0..3 and 0..30. Alone, each has ticks for its own data. In one group, both show the same ticks over 0..30, so a value of 3 sits at the same place in both.",
                 code=f"""auto build = [&](bool grouped) {{
{two_panes(a, None, '  ')}
  if (grouped) {{
    small.align_{a}("range");
    large.align_{a}("range");
  }}
  return fig.to_svg();
}};
{TEXTS}
std::cout << "alone:   " << texts(build(false)) << "\\n";
std::cout << "grouped: " << texts(build(true)) << "\\n";
{two_panes(a, 'range')}
fig.save_svg("figure.svg");"""),
            dict(id="caption", text=f"One caption for the group. Both {kind} are in the group `range`; [`Studio::{A}_caption`](../studio/{A}_caption.md) writes its text once, for the group, instead of once for each pane.",
                 code=f"""{two_panes(a, 'range')}
fig.{A}_caption("range", "Shared {A} axis");
const std::string svg = fig.to_svg();
std::cout << "members: " << small.{A}_align_group() << ", " << large.{A}_align_group() << "\\n";
std::cout << "caption drawn once: " << (svg.find(">Shared {A} axis<") == svg.rfind(">Shared {A} axis<")) << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="manual", text=f"A limit set by hand wins. `small` sets its {A} limits to 0..5; the group shares that window, and the data of `large`, which runs to 30, is clipped to it. The window is the choice the limit expresses, and the data of other members does not widen it.",
                 code=f"""auto build = [&](bool manual) {{
{two_panes(a, 'range', '  ')}
  if (manual) small.{A}_limits(Range1d{{0.0, 5.0}});
  return fig.to_svg();
}};
{TEXTS}
std::cout << "data only:     " << texts(build(false)) << "\\n";
std::cout << "hand-set 0..5: " << texts(build(true)) << "\\n";
{two_panes(a, 'range')}
small.{A}_limits(Range1d{{0.0, 5.0}});
fig.save_svg("figure.svg");"""),
            dict(id="separate", text="Groups are separate from each other. The first two panes share the group `pair`; the third has its own range and is not in it. Only the members of one group are tied.",
                 code=f"""auto fig = studio("align_{a}_groups");
{LAYOUT[a].replace('(2, 1)', '(3, 1)').replace('(1, 2)', '(1, 3)')}
{member(a, 'first', 'first', 1)}
{member(a, 'second', 'second', 10)}
{member(a, 'third', 'third', 100)}
first.align_{a}("pair");
second.align_{a}("pair");
{TEXTS}
std::cout << texts(fig.to_svg()) << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="leave", text=f"Leaving a group. `clear_{a}_align()` and `align_{a}(\"\")` are the same: the pane is in no group and has its own range again, and the group string reads back empty. The figure is then exactly the figure of panes that never joined.",
                 code=f"""auto build = [&](int mode) {{
{two_panes(a, None, '  ')}
  if (mode >= 1) {{
    small.align_{a}("range");
    large.align_{a}("range");
  }}
  if (mode == 2) {{
    small.clear_{a}_align();
    large.align_{a}("");
  }}
  return fig.to_svg();
}};
std::cout << "left == never joined: " << (build(2) == build(0)) << "\\n";
std::cout << "joined == never joined: " << (build(1) == build(0)) << "\\n";
{two_panes(a)}
small.align_{a}("range");
std::cout << "group: '" << small.{A}_align_group() << "'\\n";
small.clear_{a}_align();
std::cout << "group after clear: '" << small.{A}_align_group() << "'\\n";
fig.save_svg("figure.svg");"""),
            dict(id="alone", text="A group of one is harmless. A name nobody else uses ties the pane to nothing, no error is raised, and the pane keeps its own range; the name only matters once a second pane or a caption uses it.",
                 code=f"""auto build = [&](bool named) {{
{two_panes(a, None, '  ')}
  if (named) small.align_{a}("only_me");
  return fig.to_svg();
}};
std::cout << "same figure as without a group: " << (build(true) == build(false)) << "\\n";
{two_panes(a)}
small.align_{a}("only_me");
fig.save_svg("figure.svg");"""),
        ],
        notes=f"""- Any string is a valid group; there is no registry and nothing to throw. A slip in a group name makes a group of one, and the panes then do not share a range: read the name back with [`{A}_align_group`](align_group_accessors.md) to check.
- The group shares the {A} range before the axis is drawn, so ticks, grid lines and tick labels of every member agree. The other settings of the axis (`{A}_ticks`, `{A}_tick_format`, `{A}_scale`) stay each pane's own.
- A hidden pane ([`visible`](visible.md)) draws nothing and takes no part in the group: the range is that of the panes that are drawn, and the caption is placed under them.
- `align_{a}` is not `noexcept` (it stores a string); `clear_{a}_align()` is.""",
        see=[f"[`Pane::align_{b}`](align_{b}.md): the same for the {b} axis." for b in "xyz" if b != a]
            + ["[`Pane::align_xy`](align_xy.md), [`align_xyz`](align_xyz.md): join several axes in one call.",
               "[`Pane::clear_align`](clear_align.md): leave every group at once.",
               f"[`Pane::{A}_align_group`](align_group_accessors.md): read the group back.",
               f"[`Studio::{A}_caption`](../studio/{A}_caption.md): the caption a group shares.",
               f"[`Pane::{A}_limits`]({A}_limits.md): hand-set limits, which win in a group."],
    )
    write(f"align_{a}", page)


def multi_page(slug, axes, doc):
    A = axes
    fn = slug
    is3 = "z" in A
    pane_setup = ('auto& pane = fig.pane3("p");\npane.path3(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0}, std::vector<double>{0, 1, 2});'
                  if is3 else 'auto& pane = fig.pane2("p");\npane.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});')
    if is3:
        readback = f"""auto fig = studio("{slug}_readback");
{pane_setup}
pane.{fn}("g");
std::cout << "x '" << pane.x_align_group() << "', y '" << pane.y_align_group() << "', z '" << pane.z_align_group() << "'\\n";
fig.save_svg("figure.svg");"""
    else:
        readback = f"""auto fig = studio("{slug}_readback");
{pane_setup}
pane.align_z("depth");
pane.{fn}("g");
std::cout << "x '" << pane.x_align_group() << "', y '" << pane.y_align_group() << "', z (not covered) '" << pane.z_align_group() << "'\\n";
fig.save_svg("figure.svg");"""
    page = dict(
        slug=slug,
        title=f"Pane::{fn}",
        signature=f"Pane& Pane::{fn}(std::string group)",
        intro=f"""{doc}

The call is the same as {" and ".join(f"[`align_{c}(group)`](align_{c}.md)" for c in A)}: each axis joins the group `group`, and everything [`align_x`](align_x.md) says about a group (a shared range, hand-set limits winning, one shared caption) holds for each axis separately. The axes are independent groups that happen to carry the same name, so a caption for the x group and one for the y group are two captions, written by [`x_caption`](../studio/x_caption.md) and [`y_caption`](../studio/y_caption.md).

Use it for panes that are copies of one another in layout (small multiples), where every axis should line up. When only one axis should line up, call the single-axis form; when the axes need different names, call each form with its own name.""",
        parameters=f"""**group** (`std::string`)
    The name given to the groups of the {" and ".join(A)} axes. An empty string leaves all of them.""",
        returns="""`Pane&`
    The same pane, for chaining.""",
        examples=[
            dict(id="readback", text="The same name on every covered axis. After the call, the group of each axis reads back as `group`; an axis the call does not cover is untouched.",
                 code=readback),
            dict(id="grid", text=f"Small multiples. Four panes of a 2x2 mosaic with different data all call `{fn}(\"grid\")`: the whole figure shares one x range and one y range, and the panes line up.",
                 code=f"""auto fig = studio("{slug}_grid");
fig.mosaic(2, 2);
const double scales[] = {{1.0, 4.0, 2.0, 8.0}};
for (int i = 0; i < 4; ++i) {{
  auto& pane = fig.pane2("scale " + std::to_string(static_cast<int>(scales[i])));
  pane.trend(std::vector<double>{{0, 1, 2, 3}}, std::vector<double>{{0, scales[i], scales[i] / 2, scales[i]}});
  pane.align_xy("grid");
}}
fig.save_svg("figure.svg");""".replace("align_xy", fn) if not is3 else f"""auto fig = studio("{slug}_grid");
fig.mosaic(1, 2);
for (int i = 0; i < 2; ++i) {{
  auto& pane = fig.pane3("scale " + std::to_string(i + 1));
  const double s = i == 0 ? 1.0 : 5.0;
  pane.path3(std::vector<double>{{0, s, 0}}, std::vector<double>{{0, s, 2 * s}}, std::vector<double>{{0, s, 3 * s}});
  pane.{fn}("grid");
}}
fig.save_svg("figure.svg");"""),
            dict(id="captions", text="Captions follow the axes. One `x_caption` and one `y_caption` for the same group name each appear once for the whole figure; both match because the call set both axes' groups.",
                 code=f"""auto fig = studio("{slug}_captions");
fig.mosaic(1, 2);
fig.x_caption("grid", "time");
fig.y_caption("grid", "level");
for (int i = 0; i < 2; ++i) {{
  auto& pane = fig.{'pane3' if is3 else 'pane2'}("run " + std::to_string(i + 1));
  {'pane.path3(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1.0 + i, 0.5}, std::vector<double>{0, 1, 2});' if is3 else 'pane.trend(std::vector<double>{0, 1, 2, 3}, std::vector<double>{0, 1.0 + i, 0.5, 2.0});'}
  pane.{fn}("grid");
}}
std::cout << "x captions: " << fig.x_captions().size() << ", y captions: " << fig.y_captions().size() << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="override", text=f"Mixing with the single-axis calls. Calling `{fn}` and then `align_y` with another name changes only the y group; the x group keeps the shared name.",
                 code=f"""auto fig = studio("{slug}_override");
{pane_setup}
pane.{fn}("g");
pane.align_y("other");
std::cout << "x '" << pane.x_align_group() << "', y '" << pane.y_align_group() << "'\\n";
fig.save_svg("figure.svg");"""),
        ],
        notes="""- Equivalent to the single-axis calls with the same name; nothing else changes on the pane.
- Not `noexcept`. Any string is accepted; the empty string leaves the groups.
- On a planar pane a z group is stored but there is no z axis to share, so it has no visible effect.""",
        see=["[`Pane::align_x`](align_x.md), [`align_y`](align_y.md), [`align_z`](align_z.md): one axis at a time, with the full account of what a group does.",
             "[`Pane::clear_align`](clear_align.md): leave all groups.",
             "[`Pane::x_align_group`](align_group_accessors.md): read a group back."],
    )
    write(slug, page)


def clear_page():
    page = dict(
        slug="clear_align",
        title="Pane::clear_align",
        signature="Pane& Pane::clear_align() noexcept",
        intro="""Takes the pane out of every alignment group at once: the x, y and z groups all become empty. It undoes [`align_xyz`](align_xyz.md) and [`align_xy`](align_xy.md), and it undoes any mix of `align_x`, `align_y` and `align_z`, whatever names they used.

The pane then has the ranges its own data and limits give it, and no shared caption is drawn for it. Other panes are unaffected: if the group had other members, they keep sharing among themselves, and the range they share no longer includes this pane's data.""",
        parameters="*(none)*",
        returns="""`Pane&`
    The same pane, for chaining.""",
        examples=[
            dict(id="all_three", text="All three groups are left. A 3D pane in groups on all three axes reads back empty on all of them.",
                 code="""auto fig = studio("clear_align_all");
auto& pane = fig.pane3("p");
pane.path3(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0}, std::vector<double>{0, 1, 2});
pane.align_xyz("g");
std::cout << "before: '" << pane.x_align_group() << "' '" << pane.y_align_group() << "' '" << pane.z_align_group() << "'\\n";
pane.clear_align();
std::cout << "after:  '" << pane.x_align_group() << "' '" << pane.y_align_group() << "' '" << pane.z_align_group() << "'\\n";
fig.save_svg("figure.svg");"""),
            dict(id="mixed_names", text="Different names, one call. The three axes were put in groups with three different names; `clear_align()` does not need to know them.",
                 code="""auto fig = studio("clear_align_mixed");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
pane.align_x("columns").align_y("rows").align_z("depth");
pane.clear_align();
std::cout << "'" << pane.x_align_group() << "' '" << pane.y_align_group() << "' '" << pane.z_align_group() << "'\\n";
fig.save_svg("figure.svg");"""),
            dict(id="effect", text="The effect on the figure. Two panes share their ranges through the group `g`; after both call `clear_align()`, each shows the range of its own data again, and the figure is the figure of panes that never joined.",
                 code=f"""auto build = [&](int mode) {{
  auto fig = studio("clear_align_effect");
  fig.mosaic(1, 2);
{member('y', 'small', 'small', 1, '  ')}
{member('y', 'large', 'large', 10, '  ')}
  if (mode >= 1) {{
    small.align_y("g");
    large.align_y("g");
  }}
  if (mode == 2) {{
    small.clear_align();
    large.clear_align();
  }}
  return fig.to_svg();
}};
{TEXTS}
std::cout << "joined:  " << texts(build(1)) << "\\n";
std::cout << "cleared: " << texts(build(2)) << "\\n";
std::cout << "cleared == never joined: " << (build(2) == build(0)) << "\\n";
auto fig = studio("clear_align_effect_out");
fig.mosaic(1, 2);
{member('y', 'small', 'small', 1)}
{member('y', 'large', 'large', 10)}
fig.save_svg("figure.svg");"""),
            dict(id="partial", text="Only this pane leaves. Of three panes in the group `g`, one calls `clear_align()`; the other two keep sharing a range, which no longer covers the one that left.",
                 code=f"""auto fig = studio("clear_align_partial");
fig.mosaic(1, 3);
{member('y', 'a', 'a', 1)}
{member('y', 'b', 'b', 10)}
{member('y', 'c', 'c', 100)}
a.align_y("g");
b.align_y("g");
c.align_y("g");
c.clear_align();
{TEXTS}
std::cout << texts(fig.to_svg()) << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="noop", text="Nothing to clear is fine. On a pane in no group the call does nothing, and it can be called any number of times.",
                 code="""auto fig = studio("clear_align_noop");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
pane.clear_align().clear_align();
std::cout << "empty: " << (pane.x_align_group().empty() && pane.y_align_group().empty() && pane.z_align_group().empty()) << "\\n";
fig.save_svg("figure.svg");"""),
        ],
        notes="""- `noexcept`. The single-axis forms are [`clear_x_align`](align_x.md), [`clear_y_align`](align_y.md) and [`clear_z_align`](align_z.md).
- Clearing a group changes only this pane; it never removes the group from other panes or deletes a caption.""",
        see=["[`Pane::align_xyz`](align_xyz.md), [`align_xy`](align_xy.md): join groups.", "[`Pane::align_x`](align_x.md): the single-axis forms and the full account of what a group does."],
    )
    write("clear_align", page)


def accessor_page():
    page = dict(
        slug="align_group_accessors",
        title="Pane::x_align_group, y_align_group, z_align_group",
        signature="const std::string& Pane::x_align_group() const noexcept\nconst std::string& Pane::y_align_group() const noexcept\nconst std::string& Pane::z_align_group() const noexcept",
        intro="""Read the alignment group of one axis of the pane: the name set by [`align_x`](align_x.md), [`align_y`](align_y.md), [`align_z`](align_z.md), [`align_xy`](align_xy.md) or [`align_xyz`](align_xyz.md). The string is empty when the axis is in no group, which is the state of a new pane and the state after a `clear_*_align` call.

Call them to check which panes share a range (a mistyped group name shows up here as a group of one), to find the panes of a group in code that builds figures from a list, and to copy one pane's grouping onto another. They report the name only; the shared range is worked out when the figure is drawn.""",
        parameters="*(none)*",
        returns="""`const std::string&`
    The group name of that axis, empty for none. The reference is to the pane's own string and follows later calls; copy it to keep the value.""",
        examples=[
            dict(id="defaults", text="A new pane is in no group on any axis.",
                 code="""auto fig = studio("accessors_new");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
std::cout << "x '" << pane.x_align_group() << "', y '" << pane.y_align_group() << "', z '" << pane.z_align_group() << "'\\n";
fig.save_svg("figure.svg");"""),
            dict(id="per_axis", text="Each axis has its own name. The three setters put the axes in three groups, and each accessor returns its own axis's name.",
                 code="""auto fig = studio("accessors_axes");
auto& pane = fig.pane3("p");
pane.path3(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0}, std::vector<double>{0, 1, 2});
pane.align_x("columns").align_y("rows").align_z("depth");
std::cout << pane.x_align_group() << " " << pane.y_align_group() << " " << pane.z_align_group() << "\\n";
fig.save_svg("figure.svg");"""),
            dict(id="members", text="Finding the members of a group. Panes are listed by `Studio::panes()`, so the panes of a group are found by comparing names; this is also the quickest check that two panes really share one.",
                 code="""auto fig = studio("accessors_members");
fig.mosaic(2, 2);
const char* groups[] = {"a", "b", "a", ""};
for (int i = 0; i < 4; ++i) {
  auto& pane = fig.pane2("pane " + std::to_string(i));
  pane.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
  pane.align_x(groups[i]);
}
for (const auto& pane : fig.panes()) {
  std::cout << pane.title_text() << ": '" << pane.x_align_group() << "'\\n";
}
fig.save_svg("figure.svg");"""),
            dict(id="copy", text="Copying a grouping. The names read from one pane are given to another, which then shares the range. The accessors return references, so the values are copied into strings first.",
                 code="""auto fig = studio("accessors_copy");
fig.mosaic(1, 2);
auto& first = fig.pane2("first");
first.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});
first.align_xy("shared");
auto& second = fig.pane2("second");
second.trend(std::vector<double>{0, 10, 20}, std::vector<double>{0, 10, 0});
second.align_x(std::string(first.x_align_group())).align_y(std::string(first.y_align_group()));
std::cout << "second: '" << second.x_align_group() << "' '" << second.y_align_group() << "'\\n";
fig.save_svg("figure.svg");"""),
        ],
        notes="""- All three are `noexcept` and do not change the pane.
- An unset group is the empty string, never an error.""",
        see=["[`Pane::align_x`](align_x.md), [`align_y`](align_y.md), [`align_z`](align_z.md): set a group.", "[`Pane::clear_align`](clear_align.md): leave all groups."],
    )
    write("align_group_accessors", page)


def main():
    for a in "xyz":
        axis_page(a)
    multi_page("align_xy", "xy", "Puts both the x and the y axis of the pane into the alignment group `group`: the one call for panes whose rows and columns should line up.")
    multi_page("align_xyz", "xyz", "Puts the x, y and z axes of the pane into the alignment group `group`: the three-axis form of [`align_xy`](align_xy.md), for volumetric panes.")
    clear_page()
    accessor_page()
    print("wrote align specs")


if __name__ == "__main__":
    main()
