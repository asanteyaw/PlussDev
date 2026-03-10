    Studio fig("core_columns", Theme::paper());
    auto& pane = fig.pane2("columns");
    pane.x_categories({"infra", "models", "data", "ops"});
    pane.columns(make_series({21.0, 28.0, 24.0, 19.0})).label("columns");
    save(fig);
