PAGE = dict(
    slug="name",
    title="Studio::name",
    signature="const std::string& Studio::name() const noexcept",
    intro="""Returns the figure's name: the string given to [`studio`](studio.md) or to the `Studio` constructor, or the latest one set with [`rename`](rename.md), and empty when none was given. The name is bookkeeping. It is never drawn on the figure (the visible title is the [`headline`](headline.md)), and it does not change the SVG, PDF or PNG: two figures that differ only in name render to identical bytes.

The name is used where a figure needs a label of its own: it is the title of the viewer window opened by [`show`](show.md) when no title is passed (with `"PlussPl Plot"` as the fallback for an empty name), and it is part of the temporary file names the exporters use. Nothing requires names to be unique.""",
    parameters="*(none)*",
    returns="""`const std::string&`
    The name; empty for an unnamed figure.""",
    examples=[
        dict(id="values", text="Where the name comes from. The factory takes it, the constructor takes it, and a figure made with neither is unnamed.",
             code="""auto by_factory = studio("from the factory");
Studio by_constructor("from the constructor");
Studio unnamed;
std::cout << "factory: '" << by_factory.name() << "'\\n";
std::cout << "constructor: '" << by_constructor.name() << "'\\n";
std::cout << "unnamed: '" << unnamed.name() << "', empty: " << unnamed.name().empty() << "\\n";
by_factory.pane2("p").trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
by_factory.save_svg("figure.svg");"""),
        dict(id="not_drawn", text="Never drawn. Two figures with the same content and different names produce the same SVG, and the name does not appear in it.",
             code="""auto build = [](const char* name) {
  auto fig = studio(name);
  fig.pane2("p").trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
  return fig;
};
auto a = build("alpha");
auto b = build("a completely different name");
std::cout << "same SVG: " << (a.to_svg() == b.to_svg()) << "\\n";
std::cout << "name in the SVG: " << (a.to_svg().find("alpha") != std::string::npos) << "\\n";
a.save_svg("figure.svg");"""),
        dict(id="copy", text="A figure owns its name. A copy of a figure has the name at the time of the copy, and renaming one does not rename the other.",
             code="""auto original = studio("original");
original.pane2("p").trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
Studio copy = original;
copy.rename("copy");
std::cout << original.name() << " / " << copy.name() << "\\n";
original.save_svg("figure.svg");"""),
        dict(id="window_title", text="Where the name is used. `show` with no title and `launch = false` renders the figure and writes the viewer page without opening a window; the page is titled with the figure's name (the fallback for an unnamed figure is `PlussPl Plot`).",
             code="""auto fig = studio("named figure");
fig.pane2("p").trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
std::cout << "page written: " << fig.show({}, 1280, 820, false) << "\\n";
std::cout << "name is still: " << fig.name() << "\\n";
fig.save_svg("figure.svg");"""),
    ],
    notes="""- `noexcept`, and has no effect on the figure.
- The returned reference is to the figure's own string and is changed by [`rename`](rename.md); copy it if it is needed afterwards.""",
    see=["[`Studio::rename`](rename.md): changes the name.", "[`Studio::headline`](headline.md): the title that is drawn.", "[`Studio::show`](show.md): uses the name as the window title.", "[`studio`](studio.md): the factory that takes the name."],
)
