    Studio fig("horizontal_grid", Theme::paper());
    auto& pane = fig.pane2("horizontal_grid");
    set_style(pane, FlashStyle::Lightgrid);
    horizontal_grid(pane, true);
    lineplot(pane, x, y).label("signal");
    save(fig);
