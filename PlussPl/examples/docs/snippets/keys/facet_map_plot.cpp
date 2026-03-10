    Studio fig("facet_map_plot", Theme::paper());
    facet_map_plot(
        fig,
        2,
        2,
        [&](Pane& pane, std::int64_t row, std::int64_t col) {
          lineplot(
              pane,
              x,
              y.mul(0.55 + 0.15 * static_cast<double>(row + col))).label("facet-map");
        });
    save(fig);
