    Studio fig("core_cloud3", Theme::paper());
    auto& pane = fig.pane3("cloud3");
    pane.cloud3(x, y, z, y.abs().mul(18.0).add(8.0), y).label("cloud3");
    save(fig);
