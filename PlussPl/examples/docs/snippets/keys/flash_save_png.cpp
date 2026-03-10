    Studio fig("flash_save_png", Theme::paper());
    auto& pane = fig.pane2("flash_save_png");
    lineplot(pane, x, y).label("flash_save_png");
    const auto png_out = out_svg.parent_path() / "flash_save_png_tmp.png";
    save_png(fig, png_out.string(), 144);
    save(fig);
