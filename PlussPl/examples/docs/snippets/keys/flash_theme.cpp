    Studio fig("flash_theme", Theme::paper());
    fig.theme(flash_theme(FlashStyle::Sandgrid, FlashPalette::Bright));
    auto& pane = fig.pane2("flash_theme");
    lineplot(pane, x, y).label("flash_theme");
    save(fig);
