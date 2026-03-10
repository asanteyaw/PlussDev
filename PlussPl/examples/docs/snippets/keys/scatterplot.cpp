    Studio fig("scatterplot", Theme::paper());
    set_context(fig, 960.0, 620.0, 1.0, 1.0);
    auto& pane = fig.pane2("scatterplot");
    scatterplot(pane, x, y).label("scatterplot");
    save(fig);
