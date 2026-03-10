    Studio fig("plotting_context", Theme::paper());
    plotting_context(fig, 1200.0, 780.0, 1.0, 1.0);
    auto& pane = fig.pane2("plotting_context");
    lineplot(pane, x, y).label("signal");
    save(fig);
