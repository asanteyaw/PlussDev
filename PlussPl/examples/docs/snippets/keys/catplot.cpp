    Studio fig("catplot", Theme::paper());
    auto& pane = fig.pane2("catplot");
    catplot(pane,
            make_note({"A", "A", "A", "B", "B", "B", "C", "C", "C"}),
            make_series({0.9, 1.2, 1.4, 1.1, 1.6, 1.9, 0.7, 0.8, 1.0}),
            "violin")
        .label("catplot");
    save(fig);
