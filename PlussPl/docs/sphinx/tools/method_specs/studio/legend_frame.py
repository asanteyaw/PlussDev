PAGE = dict(
    slug="legend_frame",
    title="Studio::legend_frame",
    signature="Studio& Studio::legend_frame(KeyFrame frame) noexcept\nKeyFrame Studio::legend_frame() const noexcept",
    intro="""Sets, or reads, the frame of the docked legend ([`legend`](legend.md)): `KeyFrame::Framed` draws a bordered box, `KeyFrame::Soft` a light filled panel with no hard edge (the default), and `KeyFrame::None` only the entries, with no box at all. It styles the figure-level legend only; each pane's own inset legend has its own setting ([`Pane::legend_frame`](../pane/legend_frame.md)), and panes created afterwards take [`pane_style_defaults`](pane_style_defaults.md), not this value. The setting is read when the figure is rendered.

The setter takes any `KeyFrame` and never throws. With no docked legend ( `KeyDock::None`, or no labelled trace) the frame has no effect on the picture, although it is stored. The getter returns the stored value.""",
    parameters="""**frame** (`KeyFrame`, setter)
    `Framed`, `Soft` or `None`; see [`KeyFrame`](../types/KeyFrame.md).""",
    returns="""`Studio&` (setter)
    The same figure, for chaining.

`KeyFrame` (getter)
    The stored frame.""",
    examples=[
        dict(id="default", text="The default is `Soft`. A new figure reports `Soft`; each setting is read back, and the last call wins.",
             code="""auto name = [](KeyFrame f) { return f == KeyFrame::Framed ? "Framed" : f == KeyFrame::Soft ? "Soft" : "None"; };
auto fig = studio("legend_frame_default");
std::cout << "new figure: " << name(fig.legend_frame()) << "\\n";
fig.legend_frame(KeyFrame::Framed);
std::cout << "after Framed: " << name(fig.legend_frame()) << "\\n";
fig.legend_frame(KeyFrame::None);
std::cout << "after None: " << name(fig.legend_frame()) << "\\n";
fig.pane2("p").trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3}).label("series");
fig.save_svg("figure.svg");"""),
        dict(id="framed", text="`Framed`. The docked legend is drawn in a bordered box.",
             code="""auto fig = studio("legend_frame_framed");
fig.legend(KeyDock::East).legend_frame(KeyFrame::Framed);
fig.pane2("p").trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3}).label("series a");
fig.save_svg("figure.svg");"""),
        dict(id="soft", text="`Soft`, the default. A light filled panel with no hard edge.",
             code="""auto fig = studio("legend_frame_soft");
fig.legend(KeyDock::East).legend_frame(KeyFrame::Soft);
fig.pane2("p").trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3}).label("series a");
fig.save_svg("figure.svg");"""),
        dict(id="none", text="`None`. Only the entries are drawn, with no box; the three frames give three different figures.",
             code="""auto build = [](KeyFrame frame) {
  auto fig = studio("legend_frame_none");
  fig.legend(KeyDock::East).legend_frame(frame);
  fig.pane2("p").trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3}).label("series a");
  return fig;
};
std::cout << "Framed vs Soft: " << (build(KeyFrame::Framed).to_svg() != build(KeyFrame::Soft).to_svg()) << ", Soft vs None: " << (build(KeyFrame::Soft).to_svg() != build(KeyFrame::None).to_svg()) << "\\n";
build(KeyFrame::None).save_svg("figure.svg");"""),
        dict(id="no_legend", text="No legend, no effect. With no docked legend the frame is stored but the figure is unchanged, and it does not touch a pane's own legend.",
             code="""auto build = [](bool set_frame) {
  auto fig = studio("legend_frame_nolegend");
  if (set_frame) fig.legend_frame(KeyFrame::Framed);
  auto& pane = fig.pane2("p");
  pane.trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3}).label("series");
  pane.legend(KeyPlace::NorthEast);
  return fig;
};
std::cout << "figure frame set, pane legend unchanged: " << (build(true).to_svg() == build(false).to_svg()) << "\\n";
build(false).save_svg("figure.svg");"""),
    ],
    notes="""- The setter is `noexcept`; the getter has no effect on the figure.
- The frame is figure state read at render time, so it can be set before or after the legend and the panes.""",
    see=["[`Studio::legend`](legend.md): docks the legend.", "[`Studio::legend_shape`](legend_shape.md): the proportions of the key.", "[`Pane::legend_frame`](../pane/legend_frame.md): the inset legend of one pane.", "[`KeyFrame`](../types/KeyFrame.md): the values."],
)
