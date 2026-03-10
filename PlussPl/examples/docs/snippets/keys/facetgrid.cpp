    Studio fig("facetgrid", Theme::paper());
    FacetGrid(fig, 2, 2, 14.0, 14.0);
    for (std::int64_t r = 0; r < 2; ++r) {
      for (std::int64_t c = 0; c < 2; ++c) {
        auto& pane = fig.pane2();
        pane.place(r, c);
        lineplot(pane, x, y.mul(0.6 + 0.2 * static_cast<double>(r + c))).label("facet");
      }
    }
    save(fig);
