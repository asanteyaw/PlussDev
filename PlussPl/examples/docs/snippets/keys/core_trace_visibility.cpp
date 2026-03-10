    Studio fig("core_trace_visibility", Theme::paper());
    auto& pane = fig.pane2("trace visibility");
    pane.trend(x, y).label("visible");
    auto& muted = pane.trend(x, y.mul(0.35).add(0.35)).label("hidden then shown");
    muted.hide().show();
    pane.trend(x, y.mul(0.2).sub(0.4)).label("hidden final").hide();
    save(fig);
