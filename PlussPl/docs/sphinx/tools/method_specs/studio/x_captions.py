PAGE = dict(
    slug="x_captions",
    title="Studio::x_captions",
    signature="const std::map<std::string, std::string>& Studio::x_captions() const noexcept",
    intro="""Returns every shared x caption that [`x_caption`](x_caption.md) has registered, as a map from group name to text, sorted by group name (not by the order of the calls). It is what is registered, not what is drawn: a caption whose group no visible pane belongs to, and a caption with empty text, are in the map but draw nothing and take no room. The map is empty for a figure that never called `x_caption`, and a caption cannot be removed, only replaced (an empty text draws nothing).

The reference is to the figure's own map, so it shows later calls and should be copied when a snapshot is needed.""",
    parameters="*(none)*",
    returns="""`const std::map<std::string, std::string>&`
    The registered captions, keyed by group.""",
    examples=[
        dict(id="read", text="Reading the registry. Empty by default; each group is one entry, sorted by group name, and calling again for a group replaces its text.",
             code="""auto fig = studio("x_captions_read");
std::cout << "empty: " << fig.x_captions().empty() << "\\n";
fig.x_caption("zeta", "last group").x_caption("alpha", "first group");
fig.x_caption("alpha", "first group, revised");
for (const auto& [group, text] : fig.x_captions()) std::cout << group << " => " << text << "\\n";
fig.pane2("p").trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
fig.save_svg("figure.svg");"""),
        dict(id="registered_not_drawn", text="Registered is not drawn. A caption for a group nobody joined is in the map while the figure is exactly the figure without it; a pane joining the group makes it appear.",
             code="""auto build = [](bool join) {
  auto fig = studio("x_captions_drawn");
  fig.x_caption("g", "caption text");
  auto& pane = fig.pane2("p");
  pane.trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
  if (join) pane.align_x("g");
  return fig;
};
auto alone = build(false), joined = build(true);
std::cout << "registered either way: " << alone.x_captions().size() << " and " << joined.x_captions().size() << "\\n";
std::cout << "drawn without the group: " << (alone.to_svg().find("caption text") != std::string::npos) << ", with it: " << (joined.to_svg().find("caption text") != std::string::npos) << "\\n";
joined.save_svg("figure.svg");"""),
        dict(id="axes_separate", text="The three axes are separate registries. A caption set for one axis is not in the maps of the others.",
             code="""auto fig = studio("x_captions_axes");
fig.x_caption("g", "x only");
std::cout << "x: " << fig.x_captions().size() << ", y: " << fig.y_captions().size() << ", z: " << fig.z_captions().size() << "\\n";
fig.pane2("p").trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
fig.save_svg("figure.svg");"""),
    ],
    notes="""- `noexcept`, and has no effect on the figure.
- Groups are matched exactly and case-sensitively against the names panes pass to [`Pane::align_x`](../pane/align_x.md).""",
    see=["[`Studio::x_caption`](x_caption.md): registers a caption and says when it is drawn.", "[`Pane::align_x`](../pane/align_x.md): how a pane joins a group.", "[`Studio::x_captions`](x_captions.md), [`y_captions`](y_captions.md), [`z_captions`](z_captions.md): the three axes."],
)
