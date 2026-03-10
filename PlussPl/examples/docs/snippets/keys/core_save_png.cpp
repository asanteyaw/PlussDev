    Studio fig("core_save_png", Theme::paper());
    auto& pane = fig.pane2("save_png");
    pane.trend(x, y).label("signal");
    const auto png = out_svg.parent_path() / "core_save_png_export.png";
    fig.save_png(png.string(), 144);
    save(fig);
