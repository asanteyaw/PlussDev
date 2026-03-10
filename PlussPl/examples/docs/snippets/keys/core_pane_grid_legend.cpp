    Studio fig("core_pane_grid_legend", Theme::paper());
    auto& pane = fig.pane2("grid + legend");
    pane.grid(GridStyle::MajorMinor)
        .legend(KeyPlace::NorthEast)
        .legend_frame(KeyFrame::Framed)
        .legend_shape(KeyShape::Vertical);
    pane.trend(x, y).label("trend");
    pane.cloud(x, y.add(0.1)).label("cloud");
    save(fig);
