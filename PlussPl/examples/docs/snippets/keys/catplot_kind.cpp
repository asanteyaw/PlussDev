    Studio fig("catplot_kind", Theme::paper());
    auto& pane = fig.pane2("catplot_kind");
    CatPlotOptions options;
    options.kind = catplot_kind("bar");
    options.errorbar = CatErrorBar::SE;
    catplot(
        pane,
        make_note({"A", "A", "A", "B", "B", "B", "C", "C", "C"}),
        make_series({0.9, 1.2, 1.4, 1.1, 1.6, 1.9, 0.7, 0.8, 1.0}),
        options)
        .label("catplot_kind");
    save(fig);
