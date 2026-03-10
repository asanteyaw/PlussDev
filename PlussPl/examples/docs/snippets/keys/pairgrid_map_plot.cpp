    Studio fig("pairgrid_map_plot", Theme::paper());
    pairgrid_map_plot(
        fig,
        3,
        [&](Pane& pane, std::int64_t row, std::int64_t col, bool diagonal) {
          if (diagonal) {
            histplot(pane, columns[static_cast<std::size_t>(row)], 16).label("diag");
          } else {
            scatterplot(
                pane,
                columns[static_cast<std::size_t>(col)],
                columns[static_cast<std::size_t>(row)])
                .label("off-diag");
          }
        });
    save(fig);
