    Studio fig("core_bucket", Theme::paper());
    auto& pane = fig.pane2("bucket");
    pane.bucket(y, 16).label("bucket");
    save(fig);
