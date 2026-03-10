    Studio fig("make_dataset", Theme::paper());
    const auto ds = make_dataset(matrix, {"f1", "f2", "f3", "f4", "f5"}, Axis::Cols);
    auto& pane = fig.pane2("make_dataset");
    if (ds.numeric_values.size() >= 2) {
      scatterplot(pane, ds.numeric_values[0], ds.numeric_values[1]).label("dataset features");
    } else {
      lineplot(pane, x, y).label("fallback");
    }
    save(fig);
