PAGE = dict(
    slug="headline_text",
    title="Studio::headline_text",
    signature="const std::string& Studio::headline_text() const noexcept",
    intro="""Returns the headline text exactly as it was given to [`headline`](headline.md): an empty string for a figure that never set one. It is the stored text, not what is drawn, so a long headline is returned whole even though it is wrapped on the canvas, line breaks are returned as the `\\n` that was passed, and a whitespace-only headline is returned as the spaces that were given although nothing is drawn for it.

The reference is to the figure's own string. It stays valid until the next call to `headline` or `rename`'s neighbour setters change the figure, so copy it when the text is needed after changing the figure.""",
    parameters="*(none)*",
    returns="""`const std::string&`
    The headline as given; empty when there is none.""",
    examples=[
        dict(id="default", text="Empty by default, then set. A new figure has no headline; each call replaces the text and the getter reports the latest.",
             code="""auto fig = studio("headline_text_default");
std::cout << "new figure: '" << fig.headline_text() << "', empty: " << fig.headline_text().empty() << "\\n";
fig.headline("First");
std::cout << "after first: '" << fig.headline_text() << "'\\n";
fig.headline("Second");
std::cout << "after second: '" << fig.headline_text() << "'\\n";
fig.pane2("p").trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
fig.save_svg("figure.svg");"""),
        dict(id="stored_not_drawn", text="The stored text, not the drawn text. A headline made only of spaces is stored as given while the figure is drawn as if it had none; a long headline is stored whole while it is wrapped on the canvas.",
             code="""auto plain = studio("headline_text_stored");
plain.pane2("p").trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
auto blank = studio("headline_text_stored");
blank.headline("   ");
blank.pane2("p").trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
std::cout << "stored length: " << blank.headline_text().size() << ", figure equals the plain one: " << (blank.to_svg() == plain.to_svg()) << "\\n";
const std::string long_text(200, 'x');
plain.headline(long_text);
std::cout << "long headline stored whole: " << (plain.headline_text() == long_text) << "\\n";
plain.save_svg("figure.svg");"""),
        dict(id="line_breaks", text="Line breaks come back as given. The getter returns the `\\n` that was passed, so a caller can split the headline itself.",
             code="""auto fig = studio("headline_text_lines");
fig.headline("Sales by region\\nFiscal year 2025");
const std::string& text = fig.headline_text();
const auto at = text.find('\\n');
std::cout << "first line: " << text.substr(0, at) << "\\n";
std::cout << "second line: " << text.substr(at + 1) << "\\n";
fig.pane2("p").trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
fig.save_svg("figure.svg");"""),
        dict(id="independent", text="Separate from the name. Setting the name does not touch the headline and setting the headline does not touch the name.",
             code="""auto fig = studio("original");
fig.headline("A headline");
fig.rename("renamed");
std::cout << fig.name() << " / " << fig.headline_text() << "\\n";
fig.headline("");
std::cout << fig.name() << " / '" << fig.headline_text() << "'\\n";
fig.pane2("p").trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 3});
fig.save_svg("figure.svg");"""),
    ],
    notes="""- `noexcept`, and has no effect on the figure.
- The headline of a copied figure is a separate string; changing one does not change the other.""",
    see=["[`Studio::headline`](headline.md): sets the headline and says how it is drawn.", "[`Studio::name`](name.md): the name, which is never drawn."],
)
