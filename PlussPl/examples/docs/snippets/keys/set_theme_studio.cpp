    Studio fig("set_theme_studio", Theme::paper());
    set_theme(
        fig,
        FlashThemeOptions{
            .style = FlashStyle::Sandgrid,
            .context = FlashContext::Talk,
            .borderless = false,
            .horizontal_grid = true,
        });
    auto& pane = fig.pane2("set_theme_studio");
    lineplot(pane, x, y).label("signal");
    save(fig);
