    Studio fig("set_theme_pane", Theme::paper());
    auto& pane = fig.pane2("set_theme_pane");
    set_theme(
        pane,
        FlashThemeOptions{
            .style = FlashStyle::Lightgrid,
            .context = FlashContext::Notebook,
            .borderless = false,
            .horizontal_grid = false,
        });
    lineplot(pane, x, y).label("signal");
    save(fig);
