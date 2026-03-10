    Studio fig("load_dataset", Theme::paper());
    if (auto data = load_dataset("tips")) {
      pairplot(fig, *data);
    } else {
      auto& pane = fig.pane2("fallback");
      lineplot(pane, x, y).label("fallback");
    }
    save(fig);
