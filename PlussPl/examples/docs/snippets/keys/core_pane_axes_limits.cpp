    Studio fig("core_pane_axes_limits", Theme::paper());
    auto& pane = fig.pane2("axes + limits");
    pane.x_label("x").y_label("y");
    pane.x_limits({0.0, 12.0})
        .y_limits({-1.3, 1.3})
        .x_limit_method(LimitMethod::Tight)
        .y_limit_method(LimitMethod::Padded);
    pane.trend(x, y).label("signal");
    save(fig);
