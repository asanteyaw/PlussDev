    Studio fig("lineplot", Theme::paper());
    set_context(fig, 960.0, 620.0, 1.0, 1.0);
    auto& pane = fig.pane2("lineplot");
    lineplot(pane, x, y).label("lineplot");
    save(fig);
