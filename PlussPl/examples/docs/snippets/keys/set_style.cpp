    Studio fig("set_style", Theme::paper());
    auto& pane = fig.pane2("set_style");
    set_style(pane, FlashStyle::Darkgrid);
    lineplot(pane, x, y).label("signal");
    save(fig);
