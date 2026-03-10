    Studio fig("core_wedge", Theme::paper());
    auto& pane = fig.pane2("wedge");
    pane.wedge(make_series({15.0, 23.0, 18.0, 44.0}),
               make_series({0.02, 0.04, 0.01, 0.0}),
               {"A", "B", "C", "D"})
        .label("wedge");
    save(fig);
