    Studio fig("core_pane_tick_format", Theme::paper());
    auto& pane = fig.pane2("tick format");
    pane.x_tick_format(TickFormat::Integer)
        .y_tick_format(TickFormat::Fixed3)
        .x_auto_tick_decimals(0)
        .y_auto_tick_decimals(3);
    pane.trend(x, y.mul(123.456)).label("formatted");
    save(fig);
