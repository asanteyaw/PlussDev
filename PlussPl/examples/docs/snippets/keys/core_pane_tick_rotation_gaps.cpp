    Studio fig("core_pane_tick_rotation_gaps", Theme::paper());
    auto& pane = fig.pane2("rotation + gaps");
    pane.x_categories({"Jan", "Feb", "Mar", "Apr", "May", "Jun"})
        .x_tick_rotation(30.0)
        .x_tick_label_gap(8.0)
        .x_label_gap(18.0)
        .x_label("month")
        .y_label("score")
        .y_label_centered(true);
    pane.columns(make_series({13.0, 16.0, 14.0, 18.0, 17.0, 19.0})).label("scores");
    save(fig);
