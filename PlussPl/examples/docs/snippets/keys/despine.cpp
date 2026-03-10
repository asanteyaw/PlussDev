    Studio fig("despine", Theme::paper());
    auto& pane = fig.pane2("despine");
    set_style(pane, FlashStyle::Light);
    despine(pane, true);
    lineplot(pane, x, y).label("signal");
    save(fig);
