    Studio fig("flash_set", Theme::paper());
    set(fig, Theme::ocean());
    auto& pane = fig.pane2("flash_set");
    lineplot(pane, x, y).label("flash_set");
    save(fig);
