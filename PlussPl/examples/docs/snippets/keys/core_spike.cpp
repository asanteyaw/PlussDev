    Studio fig("core_spike", Theme::paper());
    auto& pane = fig.pane2("spike");
    pane.spike(x, y.abs(), 0.0).label("spike");
    save(fig);
