    Studio fig("jointgrid_map_plot", Theme::paper());
    jointgrid_map_plot(
        fig,
        [&](Pane& joint) {
          scatterplot(joint, x, y).label("joint");
        },
        [&](Pane& marginal, bool top_axis) {
          if (top_axis) {
            histplot(marginal, x, 20).label("top");
          } else {
            histplot(marginal, y, 20).label("right");
          }
        });
    save(fig);
