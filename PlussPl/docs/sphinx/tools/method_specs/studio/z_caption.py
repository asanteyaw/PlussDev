PAGE = dict(
    slug="z_caption",
    title="Studio::z_caption",
    signature="Studio& Studio::z_caption(std::string group, std::string text)",
    intro="""Sets a **shared z caption**: one line of text drawn once for the z axes of every pane that belongs to the alignment group `group`. A pane joins a group with [`Pane::align_z`](../pane/align_z.md) (or `align_xy` / `align_xyz`); the caption and the panes are tied together only by that string. Several panes in one group get a single caption, placed above the group's plot areas.

The caption is drawn only when something shows it. A caption whose group no visible pane belongs to, and a caption whose text is empty, are stored and read back by [`z_captions`](z_captions.md) but draw nothing and take no space: the figure is the same as one without the call. The caption takes its room (the top of the plot) as soon as at least one visible pane belongs to its group, so adding or hiding panes changes the layout the way you would expect.

Calling `z_caption` again with the same group replaces the text; there is one caption per group. Different groups are independent, so two columns of panes can each carry their own caption.""",
    parameters="""**group** (`std::string`)
    The alignment-group name. Panes join it with `align_z`, `align_xy` or `align_xyz` using the same string; the match is exact and case-sensitive.

**text** (`std::string`)
    The caption. Math text between dollar signs is typeset. An empty string draws nothing.""",
    returns="""`Studio&`
    The same figure, for chaining.""",
    examples=[
        dict(id="shared", text="One caption for two panes. Both panes join the group `shared` and the caption is drawn once, above them.",
             code="""auto fig = studio("z_caption_shared");
fig.mosaic(1, 2);
fig.z_caption("shared", "Shared z caption");
auto& top = fig.pane3("top");
top.path3(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0}, std::vector<double>{0, 1, 2});
top.align_z("shared");
auto& bottom = fig.pane3("bottom");
bottom.path3(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0}, std::vector<double>{0, 1, 2});
bottom.align_z("shared");
std::cout << "captions: " << fig.z_captions().size() << ", text: " << fig.z_captions().at("shared") << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="overwrite", text="Calling again with the same group replaces the text. The map still holds one entry; the figure shows the second text only.",
             code="""auto fig = studio("z_caption_overwrite");
fig.mosaic(1, 2);
auto& t1 = fig.pane3("top");
t1.path3(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0}, std::vector<double>{0, 1, 2});
t1.align_z("g");
auto& t2 = fig.pane3("bottom");
t2.path3(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0}, std::vector<double>{0, 1, 2});
t2.align_z("g");
fig.z_caption("g", "first text");
fig.z_caption("g", "second text");
std::cout << "entries: " << fig.z_captions().size() << ", text: " << fig.z_captions().at("g") << "\\n";
const std::string svg = fig.to_svg();
std::cout << "first text drawn: " << (svg.find("first text") != std::string::npos) << ", second text drawn: " << (svg.find("second text") != std::string::npos) << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="groups", text="Two groups, two captions. The panes of each group are captioned separately; a pane in neither group has no caption.",
             code="""auto fig = studio("z_caption_groups");
fig.mosaic(2, 2);
auto& a1 = fig.pane3("a1");
a1.path3(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0}, std::vector<double>{0, 1, 2});
a1.align_z("left");
auto& a2 = fig.pane3("a2");
a2.path3(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0}, std::vector<double>{0, 1, 2});
a2.align_z("right");
auto& a3 = fig.pane3("a3");
a3.path3(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0}, std::vector<double>{0, 1, 2});
a3.align_z("left");
auto& a4 = fig.pane3("a4");
a4.path3(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0}, std::vector<double>{0, 1, 2});
fig.z_caption("left", "left group");
fig.z_caption("right", "right group");
std::cout << "captions: " << fig.z_captions().size() << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="unmatched", text="A caption nobody shows. The group `nobody` has no pane, so the caption is stored but the SVG is identical to the figure without the call: no text, no reserved space. An empty text behaves the same way.",
             code="""auto build = [](int variant) {
  auto fig = studio("z_caption_unmatched");
  auto& pane = fig.pane3("p");
  pane.path3(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0}, std::vector<double>{0, 1, 2});
  pane.align_z("g");
  if (variant == 1) fig.z_caption("nobody", "never drawn");
  if (variant == 2) fig.z_caption("g", "");
  return fig.to_svg();
};
std::cout << "no call == unmatched group: " << (build(0) == build(1)) << "\\n";
std::cout << "no call == empty text:      " << (build(0) == build(2)) << "\\n";
auto fig = studio("z_caption_unmatched_out");
auto& pane = fig.pane3("p");
  pane.path3(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0}, std::vector<double>{0, 1, 2});
  pane.align_z("g");
fig.z_caption("nobody", "never drawn");
fig.save_svg("figure.svg");"""),
        dict(id="hidden", text="Hiding the panes of a group removes the caption and its room. The caption is drawn while at least one pane of the group is visible, and is not drawn once every one of them is hidden.",
             code="""auto build = [](bool hide_a, bool hide_b) {
  auto fig = studio("z_caption_hidden");
  fig.mosaic(1, 2);
  fig.z_caption("g", "group caption");
  auto& a = fig.pane3("a");
  a.path3(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0}, std::vector<double>{0, 1, 2});
  a.align_z("g");
  if (hide_a) a.hide();
  auto& b = fig.pane3("b");
  b.path3(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0}, std::vector<double>{0, 1, 2});
  b.align_z("g");
  if (hide_b) b.hide();
  return fig.to_svg();
};
auto drawn = [](const std::string& svg) { return svg.find("group caption") != std::string::npos; };
std::cout << "both visible:  " << drawn(build(false, false)) << "\\n";
std::cout << "a hidden:      " << drawn(build(true, false)) << "\\n";
std::cout << "both hidden:   " << drawn(build(true, true)) << "\\n";
auto fig = studio("z_caption_hidden_out");
fig.mosaic(1, 2);
fig.z_caption("g", "group caption");
auto& a = fig.pane3("a");
a.path3(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0}, std::vector<double>{0, 1, 2});
a.align_z("g");
a.hide();
auto& b = fig.pane3("b");
b.path3(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0}, std::vector<double>{0, 1, 2});
b.align_z("g");
fig.save_svg("figure.svg");"""),
    ],
    notes="""- The group string is the only link between the caption and the panes, so a misspelt group draws nothing. [`z_captions`](z_captions.md) lists the registered groups, which makes a mismatch easy to spot.
- A caption is figure-level text. For text that belongs to one pane use the pane's own axis label ([`Pane::z_label`](../pane/z_label.md)).
- Pane groups are set on the pane, so they follow the pane when it is moved with [`mosaic`](mosaic.md) or [`subpane2`](subpane2.md).""",
    see=["[`Studio::z_captions`](z_captions.md): reads the registered captions back.", "[`Pane::align_z`](../pane/align_z.md): how a pane joins a group.", "[`Studio::x_caption`](x_caption.md), [`y_caption`](y_caption.md), [`z_caption`](z_caption.md): the three axes.", "[`Studio::row_title`](row_title.md): a title for a whole row of the mosaic."],
)
