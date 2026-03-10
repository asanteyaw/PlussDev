    Studio fig("flash_show", Theme::paper());
    auto& pane = fig.pane2("flash_show");
    lineplot(pane, x, y).label("flash_show");
    const bool ok = show(fig, "flash_show", 920, 620, false);
    pane.caption(ok ? "show() call succeeded" : "show() call returned false");
    save(fig);
