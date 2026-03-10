    Studio fig("core_pane_scales", Theme::paper());
    auto& pane = fig.pane2("scale_x/log + scale_y/symlog");
    const auto xs = linspace(1.0, 120.0, 180, ScalarType::Float64);
    const auto ys = xs.mul(0.08).sin().mul(xs.mul(0.02).add(0.6));
    pane.scale_x(ScalePolicy::log10()).scale_y(ScalePolicy::symlog(10.0, 0.2));
    pane.trend(xs, ys).label("scaled");
    save(fig);
