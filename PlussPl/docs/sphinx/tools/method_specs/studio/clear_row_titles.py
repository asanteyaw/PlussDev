PAGE = dict(
    slug="clear_row_titles",
    title="Studio::clear_row_titles",
    signature="Studio& Studio::clear_row_titles() noexcept",
    intro="""Removes every row title at once: [`row_titles`](row_titles.md) becomes empty and nothing is drawn for any row. On a figure with no titles it does nothing. It never throws, and the grid, the panes and every other setting are untouched, so the figure afterwards is exactly the figure that never set a title.""",
    parameters="*(none)*",
    returns="""`Studio&`
    The same figure, for chaining.""",
    examples=[
        dict(id="all", text="Removing all titles. Three titles are set, `clear_row_titles` empties the map, and the figure equals the one that never set any.",
             code="""auto build = [](bool titled) {
  auto fig = studio("clear_row_titles_all");
  fig.mosaic(3, 1);
  if (titled) fig.row_title(0, "a").row_title(1, "b").row_title(2, "c").clear_row_titles();
  for (int i = 0; i < 3; ++i) fig.pane2("p" + std::to_string(i)).trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
  return fig;
};
auto cleared = build(true), never = build(false);
std::cout << "entries: " << cleared.row_titles().size() << ", equals the untitled figure: " << (cleared.to_svg() == never.to_svg()) << "\\n";
cleared.save_svg("figure.svg");"""),
        dict(id="nothing", text="On an untitled figure it does nothing. The call is harmless and can be used to reset a figure before titling it again.",
             code="""auto fig = studio("clear_row_titles_none");
fig.clear_row_titles();
std::cout << "entries: " << fig.row_titles().size() << "\\n";
fig.mosaic(2, 1);
fig.row_title(0, "fresh").row_title(1, "start");
std::cout << "after titling again: " << fig.row_titles().size() << "\\n";
for (int i = 0; i < 2; ++i) fig.pane2("p" + std::to_string(i)).trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
fig.save_svg("figure.svg");"""),
        dict(id="rest_untouched", text="Only the titles go. The grid, the panes and the headline are unchanged, so the figure keeps its layout.",
             code="""auto fig = studio("clear_row_titles_rest");
fig.headline("Kept headline");
fig.mosaic(2, 2);
fig.row_title(0, "top").row_title(1, "bottom");
for (int i = 0; i < 4; ++i) fig.pane2("p" + std::to_string(i)).trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
fig.clear_row_titles();
std::cout << "grid: " << fig.mosaic_layout().rows << " x " << fig.mosaic_layout().cols << ", panes: " << fig.pane_count() << ", headline: " << fig.headline_text() << "\\n";
fig.save_svg("figure.svg");"""),
    ],
    notes="""- `noexcept`. To remove one row's title use [`clear_row_title`](clear_row_title.md).""",
    see=["[`Studio::clear_row_title`](clear_row_title.md): removes one.", "[`Studio::row_title`](row_title.md): stores a title.", "[`Studio::row_titles`](row_titles.md): the stored titles."],
)
