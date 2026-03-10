    Studio fig("core_save_svg", Theme::paper());
    auto& pane = fig.pane2("save_svg");
    pane.trend(x, y).label("signal");
    const auto svg = out_svg.parent_path() / "core_save_svg_export.svg";
    fig.save_svg(svg.string());
    save(fig);
