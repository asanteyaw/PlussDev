    Studio fig("pairgrid", Theme::paper());
    PairGrid(fig, 3, 12.0, 12.0);
    for (std::int64_t r = 0; r < 3; ++r) {
      for (std::int64_t c = 0; c < 3; ++c) {
        auto& pane = fig.pane2();
        pane.place(r, c);
        if (r == c) {
          histplot(pane, columns[static_cast<std::size_t>(r)], 16).label("diag");
        } else {
          scatterplot(
              pane,
              columns[static_cast<std::size_t>(c)],
              columns[static_cast<std::size_t>(r)])
              .label("off-diag");
        }
      }
    }
    save(fig);
