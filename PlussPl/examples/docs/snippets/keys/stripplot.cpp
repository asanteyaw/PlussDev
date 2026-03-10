    Studio fig("stripplot", Theme::paper());
    auto& pane = fig.pane2("stripplot");
    const Note cats = make_note({"A", "A", "A", "B", "B", "B", "C", "C", "C"});
    const Series vals = make_series({0.9, 1.2, 1.4, 1.1, 1.6, 1.9, 0.7, 0.8, 1.0});
    stripplot(pane, cats, vals).label("stripplot");
    save(fig);
