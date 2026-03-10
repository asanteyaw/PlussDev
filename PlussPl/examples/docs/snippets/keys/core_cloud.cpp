    Studio fig("core_cloud", Theme::paper());
    auto& pane = fig.pane2("cloud");
    pane.cloud(x, y, y.abs().mul(18.0).add(8.0), y).label("cloud");
    save(fig);
