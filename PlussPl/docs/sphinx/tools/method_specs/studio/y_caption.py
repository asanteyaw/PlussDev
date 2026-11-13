PAGE = dict(
    slug="y_caption",
    title="Studio::y_caption",
    signature="Studio& Studio::y_caption(std::string group, std::string text)",
    intro="""Sets a **shared y caption**: one line of text drawn once for the y axes of every pane that belongs to the alignment group `group`. A pane joins a group with [`Pane::align_y`](../pane/align_y.md) (or `align_xy` / `align_xyz`); the caption and the panes are tied together only by that string. Several panes in one group get a single caption, turned to read upward and placed to the left of the group's tick labels, centred on the group's plot areas.

The caption is drawn only when something shows it. A caption whose group no visible pane belongs to, and a caption whose text is empty, are stored and read back by [`y_captions`](y_captions.md) but draw nothing and take no space: the figure is the same as one without the call. The caption takes its room (the left margin) as soon as at least one visible pane belongs to its group, so adding or hiding panes changes the layout the way you would expect.

Calling `y_caption` again with the same group replaces the text; there is one caption per group. Different groups are independent, so two columns of panes can each carry their own caption.""",
    parameters="""**group** (`std::string`)
    The alignment-group name. Panes join it with `align_y`, `align_xy` or `align_xyz` using the same string; the match is exact and case-sensitive.

**text** (`std::string`)
    The caption. Math text between dollar signs is typeset. An empty string draws nothing.""",
    returns="""`Studio&`
    The same figure, for chaining.""",
    examples=[
        dict(id="shared", text="One caption for two panes. Both panes join the group `shared` and the caption is drawn once, to the left of them.",
             code="""auto fig = studio("y_caption_shared");
fig.mosaic(1, 2);
fig.y_caption("shared", "Shared y caption");
auto& top = fig.pane2("top");
top.trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
top.align_y("shared");
auto& bottom = fig.pane2("bottom");
bottom.trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
bottom.align_y("shared");
std::cout << "captions: " << fig.y_captions().size() << ", text: " << fig.y_captions().at("shared") << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="overwrite", text="Calling again with the same group replaces the text. The map still holds one entry; the figure shows the second text only.",
             code="""auto fig = studio("y_caption_overwrite");
fig.mosaic(1, 2);
auto& t1 = fig.pane2("top");
t1.trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
t1.align_y("g");
auto& t2 = fig.pane2("bottom");
t2.trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
t2.align_y("g");
fig.y_caption("g", "first text");
fig.y_caption("g", "second text");
std::cout << "entries: " << fig.y_captions().size() << ", text: " << fig.y_captions().at("g") << "\\n";
const std::string svg = fig.to_svg();
std::cout << "first text drawn: " << (svg.find("first text") != std::string::npos) << ", second text drawn: " << (svg.find("second text") != std::string::npos) << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="groups", text="Two groups, two captions. The panes of each group are captioned separately; a pane in neither group has no caption.",
             code="""auto fig = studio("y_caption_groups");
fig.mosaic(2, 2);
auto& a1 = fig.pane2("a1");
a1.trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
a1.align_y("left");
auto& a2 = fig.pane2("a2");
a2.trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
a2.align_y("right");
auto& a3 = fig.pane2("a3");
a3.trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
a3.align_y("left");
auto& a4 = fig.pane2("a4");
a4.trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
fig.y_caption("left", "left group");
fig.y_caption("right", "right group");
std::cout << "captions: " << fig.y_captions().size() << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="unmatched", text="A caption nobody shows. The group `nobody` has no pane, so the caption is stored but the SVG is identical to the figure without the call: no text, no reserved space. An empty text behaves the same way.",
             code="""auto build = [](int variant) {
  auto fig = studio("y_caption_unmatched");
  auto& pane = fig.pane2("p");
  pane.trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
  pane.align_y("g");
  if (variant == 1) fig.y_caption("nobody", "never drawn");
  if (variant == 2) fig.y_caption("g", "");
  return fig.to_svg();
};
std::cout << "no call == unmatched group: " << (build(0) == build(1)) << "\\n";
std::cout << "no call == empty text:      " << (build(0) == build(2)) << "\\n";
auto fig = studio("y_caption_unmatched_out");
auto& pane = fig.pane2("p");
  pane.trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
  pane.align_y("g");
fig.y_caption("nobody", "never drawn");
fig.save_svg("figure.svg");"""),
        dict(id="hidden", text="Hiding the panes of a group removes the caption and its room. The caption is drawn while at least one pane of the group is visible, and is not drawn once every one of them is hidden.",
             code="""auto build = [](bool hide_a, bool hide_b) {
  auto fig = studio("y_caption_hidden");
  fig.mosaic(1, 2);
  fig.y_caption("g", "group caption");
  auto& a = fig.pane2("a");
  a.trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
  a.align_y("g");
  if (hide_a) a.hide();
  auto& b = fig.pane2("b");
  b.trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
  b.align_y("g");
  if (hide_b) b.hide();
  return fig.to_svg();
};
auto drawn = [](const std::string& svg) { return svg.find("group caption") != std::string::npos; };
std::cout << "both visible:  " << drawn(build(false, false)) << "\\n";
std::cout << "a hidden:      " << drawn(build(true, false)) << "\\n";
std::cout << "both hidden:   " << drawn(build(true, true)) << "\\n";
auto fig = studio("y_caption_hidden_out");
fig.mosaic(1, 2);
fig.y_caption("g", "group caption");
auto& a = fig.pane2("a");
a.trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
a.align_y("g");
a.hide();
auto& b = fig.pane2("b");
b.trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
b.align_y("g");
fig.save_svg("figure.svg");"""),
    ],
    notes="""- The group string is the only link between the caption and the panes, so a misspelt group draws nothing. [`y_captions`](y_captions.md) lists the registered groups, which makes a mismatch easy to spot.
- A caption is figure-level text. For text that belongs to one pane use the pane's own axis label ([`Pane::y_label`](../pane/y_label.md)).
- Pane groups are set on the pane, so they follow the pane when it is moved with [`mosaic`](mosaic.md) or [`subpane2`](subpane2.md).""",
    see=["[`Studio::y_captions`](y_captions.md): reads the registered captions back.", "[`Pane::align_y`](../pane/align_y.md): how a pane joins a group.", "[`Studio::x_caption`](x_caption.md), [`y_caption`](y_caption.md), [`z_caption`](z_caption.md): the three axes.", "[`Studio::row_title`](row_title.md): a title for a whole row of the mosaic."],
)
