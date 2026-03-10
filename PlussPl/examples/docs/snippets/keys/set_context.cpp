    Studio fig("set_context", Theme::paper());
    set_context(fig, 1180.0, 760.0, 1.2, 1.15);
    auto& pane = fig.pane2("set_context");
    lineplot(pane, x, y).label("signal");
    save(fig);
