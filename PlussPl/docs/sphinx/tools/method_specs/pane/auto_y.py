PAGE = dict(
    slug="auto_y",
    title="Pane::auto_y",
    signature="Pane& Pane::auto_y() noexcept",
    intro="""Returns the y axis to **automatic fitting**: it discards a manual range set with [`y_limits`](y_limits.md) and puts the axis back in `LimitMode::AutoFit`, so the range is fitted to the data again by the axis's [`LimitMethod`](../types/LimitMethod.md). It is the way back from a manual range; there is no other call that clears one.

The method that was chosen with [`y_limit_method`](y_limit_method.md) is not touched, so it applies again as soon as fitting is automatic. On an axis that is already automatic the call does nothing, and it never throws. """,
    parameters="*(none)*",
    returns="""`Pane&`
    A reference to the same pane, for chaining.""",
    examples=[
        dict(id="back_to_auto", text="A manual range, then `auto_y()`. The mode is `AutoFit` at the start, `Manual` after `y_limits`, and `AutoFit` again after `auto_y()`.",
             code="""auto fig = studio("auto_y_back");
auto& pane = fig.pane2("auto_y");
pane.trend(std::vector<double>{1, 2, 3, 4, 5}, std::vector<double>{2.3, 4.7, 3.1, 5.85, 6.2});
auto mode = [&]() { return static_cast<int>(pane.y_axis().limit_mode); };
std::cout << "AutoFit = 0, Manual = 1\\n";
std::cout << "at the start:   " << mode() << "\\n";
pane.y_limits(Range1d{0.0, 20.0});
std::cout << "after y_limits: " << mode() << "\\n";
pane.auto_y();
std::cout << "after auto_y:  " << mode() << ", limits cleared: " << !pane.y_axis().limits.has_value() << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="identical", text="After `auto_y()` the figure is the figure of a pane that never had a manual range: the SVG text of the two is identical.",
             code="""std::string svg[2];
for (int i = 0; i < 2; ++i) {
  auto fig = studio("auto_y_identical");
  auto& pane = fig.pane2("auto_y");
  pane.trend(std::vector<double>{1, 2, 3, 4, 5}, std::vector<double>{2.3, 4.7, 3.1, 5.85, 6.2});
  if (i == 1) {
    pane.y_limits(Range1d{0.0, 20.0});
    pane.auto_y();
  }
  svg[i] = fig.to_svg();
}
std::cout << "never set == set and cleared: " << (svg[0] == svg[1]) << "\\n";
auto fig = studio("auto_y_identical_out");
auto& pane = fig.pane2("auto_y");
pane.trend(std::vector<double>{1, 2, 3, 4, 5}, std::vector<double>{2.3, 4.7, 3.1, 5.85, 6.2});
fig.save_svg("figure.svg");"""),
        dict(id="method_kept", text="The fitting method survives the round trip. `Tight` is set, a manual range replaces it, and after `auto_y()` the axis is fitted `Tight` again, as the printed method shows.",
             code="""auto fig = studio("auto_y_method");
auto& pane = fig.pane2("auto_y");
pane.trend(std::vector<double>{1, 2, 3, 4, 5}, std::vector<double>{2.3, 4.7, 3.1, 5.85, 6.2});
pane.y_limit_method(LimitMethod::Tight);
pane.y_limits(Range1d{0.0, 20.0});
std::cout << "while manual, method: " << static_cast<int>(pane.y_axis().limit_method) << " (TickAligned = 0, Tight = 1, Padded = 2)\\n";
pane.auto_y();
std::cout << "after auto_y, method: " << static_cast<int>(pane.y_axis().limit_method) << ", mode " << static_cast<int>(pane.y_axis().limit_mode) << "\\n";
fig.save_svg("figure.svg");"""),
    ],
    notes="""- `auto_y()` returns the pane, so it chains: `pane.auto_y().y_limit_method(LimitMethod::Padded)`.
- A manual range set with `y_limits` is validated against the axis scale ([`ScalePolicy`](../types/ScalePolicy.md)); `auto_y()` has nothing to validate.
- The mode and the limits are read back through `Pane::y_axis()` ([`axis_accessors`](axis_accessors.md)).""",
    see=["[`Pane::y_limits`](y_limits.md): sets a manual range.", "[`Pane::y_limit_method`](y_limit_method.md): chooses how an automatic range is fitted.", "[`LimitMethod`](../types/LimitMethod.md): the three methods, with a rendered comparison."],
)
