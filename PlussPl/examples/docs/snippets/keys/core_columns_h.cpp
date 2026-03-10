    Studio fig("core_columns_h", Theme::paper());
    auto& pane = fig.pane2("columns_h");
    pane.y_categories({"A", "B", "C", "D"});
    pane.columns_h(make_series({6.0, 9.0, 11.0, 7.0})).label("columns_h");
    save(fig);
