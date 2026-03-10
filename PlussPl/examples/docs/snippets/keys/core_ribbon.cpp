    Studio fig("core_ribbon", Theme::paper());
    auto& pane = fig.pane2("ribbon");
    pane.ribbon(x, y, -0.25).label("ribbon");
    save(fig);
