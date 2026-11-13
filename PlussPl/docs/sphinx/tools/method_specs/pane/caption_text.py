PAGE = dict(
    slug='caption_text',
    title='Pane::caption_text',
    signature='const std::string& Pane::caption_text() const noexcept',
    intro="Returns the pane's caption, as set by [`caption`](caption.md); an empty string means none. A new pane has none.\n\nUse it to check what a pane carries before changing it, or to collect the captions of a figure's panes into a note. The reference is to the pane's own string and follows later calls; copy it to keep the value.",
    parameters='*(none)*',
    returns='`const std::string&`\n    The caption, empty for none.',
    examples=[
        dict(id='default', text='A new pane has no caption.',
             code='auto fig = studio("caption_text_default");\nauto& pane = fig.pane2("p");\npane.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});\nstd::cout << "empty: " << pane.caption_text().empty() << "\\n";\nfig.save_svg("figure.svg");'),
        dict(id='after', text='After `caption()` it returns the text; after clearing it, the empty string.',
             code='auto fig = studio("caption_text_after");\nauto& pane = fig.pane2("p");\npane.trend(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0});\npane.caption("a note");\nstd::cout << "\'" << pane.caption_text() << "\'\\n";\npane.caption("");\nstd::cout << "\'" << pane.caption_text() << "\'\\n";\nfig.save_svg("figure.svg");'),
        dict(id='collect', text='Collecting the captions of a figure.',
             code='auto fig = studio("caption_text_collect");\nfig.mosaic(1, 2);\nauto& a = fig.pane2("a");\na.trend(std::vector<double>{0, 1}, std::vector<double>{0, 1});\na.caption("first note");\nauto& b = fig.pane2("b");\nb.trend(std::vector<double>{0, 1}, std::vector<double>{0, 1});\nfor (const auto& pane : fig.panes()) {\n  std::cout << pane.title_text() << ": " << (pane.caption_text().empty() ? "(none)" : pane.caption_text()) << "\\n";\n}\nfig.save_svg("figure.svg");'),
    ],
    notes='- `noexcept`; reads only.',
    see=['[`Pane::caption`](caption.md): set the caption.', '[`Pane::title_text`](title_text.md): the title.'],
)
