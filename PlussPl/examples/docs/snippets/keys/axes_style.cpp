    Studio fig("axes_style", Theme::paper());
    auto& pane = fig.pane2("axes_style");
    axes_style(pane, FlashStyle::Darkgrid);
    lineplot(pane, x, y).label("axes_style");
    save(fig);
