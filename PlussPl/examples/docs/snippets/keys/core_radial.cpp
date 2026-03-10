    Studio fig("core_radial", Theme::paper());
    auto& pane = fig.pane2("radial");
    const auto theta = linspace(0.0, 2.0 * kPi, 180, ScalarType::Float64);
    const auto radius = theta.mul(3.0).sin().abs().add(0.35);
    pane.radial(theta, radius).label("radial");
    save(fig);
