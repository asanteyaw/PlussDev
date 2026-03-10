    Studio fig("core_path3", Theme::paper());
    auto& pane = fig.pane3("path3");
    pane.path3(x, y, z).label("path3");
    save(fig);
