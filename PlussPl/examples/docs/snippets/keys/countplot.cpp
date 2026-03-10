    Studio fig("countplot", Theme::paper());
    auto& pane = fig.pane2("countplot");
    countplot(pane, make_note({"A", "B", "A", "C", "A", "B", "B", "C", "A"})).label("countplot");
    save(fig);
