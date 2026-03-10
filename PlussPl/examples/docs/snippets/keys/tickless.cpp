    Studio fig("tickless", Theme::paper());
    auto& pane = fig.pane2("tickless");
    set_style(pane, FlashStyle::Lightgrid);
    tickless(pane, true);
    lineplot(pane, x, y).label("tickless");
    save(fig);
