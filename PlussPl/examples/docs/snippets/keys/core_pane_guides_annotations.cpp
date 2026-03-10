    Studio fig("core_pane_guides_annotations", Theme::paper());
    auto& pane = fig.pane2("rules + bands + pins");
    pane.rule_x(6.0, "event")
        .rule_y(0.0, "zero")
        .band_x({3.0, 5.0}, "window")
        .band_y({-0.25, 0.25}, "neutral")
        .pin("peak", 8.0, 1.0)
        .caption("overlay guides");
    pane.trend(x, y).label("signal");
    save(fig);
