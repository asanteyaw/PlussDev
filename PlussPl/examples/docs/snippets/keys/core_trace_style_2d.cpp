    Studio fig("core_trace_style_2d", Theme::paper());
    auto& pane = fig.pane2("trace styling 2d");
    auto& trace = pane.trend(x, y);
    trace.label("styled")
        .width(2.8F)
        .stroke(Rgba::from_u8(178, 65, 37))
        .palette(PaletteRamp::ocean())
        .color_limits({-1.0, 1.0});
    pane.cloud(x, y.add(0.1), y.abs().mul(12.0).add(8.0), y).label("cloud");
    save(fig);
