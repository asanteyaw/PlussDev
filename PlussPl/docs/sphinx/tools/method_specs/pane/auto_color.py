PAGE = dict(
    slug="auto_color",
    title="Pane::auto_color",
    signature="Pane& Pane::auto_color() noexcept",
    intro="""Returns the colour axis to **automatic fitting**: it discards a manual range set with [`color_limits`](color_limits.md) and puts the axis back in `LimitMode::AutoFit`, so the range is fitted to the data again by the axis's [`LimitMethod`](../types/LimitMethod.md). It is the way back from a manual range; there is no other call that clears one.

The method that was chosen with [`color_limit_method`](color_limit_method.md) is not touched, so it applies again as soon as fitting is automatic. On an axis that is already automatic the call does nothing, and it never throws. The colour axis maps a trace's colour values through its palette; its range is the range of values the palette spans.""",
    parameters="*(none)*",
    returns="""`Pane&`
    A reference to the same pane, for chaining.""",
    examples=[
        dict(id="back_to_auto", text="A manual range, then `auto_color()`. The mode is `AutoFit` at the start, `Manual` after `color_limits`, and `AutoFit` again after `auto_color()`.",
             code="""auto fig = studio("auto_color_back");
auto& pane = fig.pane2("auto_color");
pane.cloud(std::vector<double>{1, 2, 3, 4, 5}, std::vector<double>{1, 3, 2, 5, 4}, std::vector<double>(5, 8.0), std::vector<double>{2.3, 4.7, 3.1, 5.85, 6.2});
auto mode = [&]() { return static_cast<int>(pane.color_axis().limit_mode); };
std::cout << "AutoFit = 0, Manual = 1\\n";
std::cout << "at the start:   " << mode() << "\\n";
pane.color_limits(Range1d{0.0, 10.0});
std::cout << "after color_limits: " << mode() << "\\n";
pane.auto_color();
std::cout << "after auto_color:  " << mode() << ", limits cleared: " << !pane.color_axis().limits.has_value() << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="identical", text="After `auto_color()` the figure is the figure of a pane that never had a manual range: the SVG text of the two is identical.",
             code="""std::string svg[2];
for (int i = 0; i < 2; ++i) {
  auto fig = studio("auto_color_identical");
  auto& pane = fig.pane2("auto_color");
  pane.cloud(std::vector<double>{1, 2, 3, 4, 5}, std::vector<double>{1, 3, 2, 5, 4}, std::vector<double>(5, 8.0), std::vector<double>{2.3, 4.7, 3.1, 5.85, 6.2});
  if (i == 1) {
    pane.color_limits(Range1d{0.0, 10.0});
    pane.auto_color();
  }
  svg[i] = fig.to_svg();
}
std::cout << "never set == set and cleared: " << (svg[0] == svg[1]) << "\\n";
auto fig = studio("auto_color_identical_out");
auto& pane = fig.pane2("auto_color");
pane.cloud(std::vector<double>{1, 2, 3, 4, 5}, std::vector<double>{1, 3, 2, 5, 4}, std::vector<double>(5, 8.0), std::vector<double>{2.3, 4.7, 3.1, 5.85, 6.2});
fig.save_svg("figure.svg");"""),
        dict(id="method_kept", text="The fitting method survives the round trip. `Tight` is set, a manual range replaces it, and after `auto_color()` the axis is fitted `Tight` again, as the printed method shows.",
             code="""auto fig = studio("auto_color_method");
auto& pane = fig.pane2("auto_color");
pane.cloud(std::vector<double>{1, 2, 3, 4, 5}, std::vector<double>{1, 3, 2, 5, 4}, std::vector<double>(5, 8.0), std::vector<double>{2.3, 4.7, 3.1, 5.85, 6.2});
pane.color_limit_method(LimitMethod::Tight);
pane.color_limits(Range1d{0.0, 10.0});
std::cout << "while manual, method: " << static_cast<int>(pane.color_axis().limit_method) << " (TickAligned = 0, Tight = 1, Padded = 2)\\n";
pane.auto_color();
std::cout << "after auto_color, method: " << static_cast<int>(pane.color_axis().limit_method) << ", mode " << static_cast<int>(pane.color_axis().limit_mode) << "\\n";
fig.save_svg("figure.svg");"""),
    ],
    notes="""- `auto_color()` returns the pane, so it chains: `pane.auto_color().color_limit_method(LimitMethod::Padded)`.
- A manual range set with `color_limits` is validated against the axis scale ([`ScalePolicy`](../types/ScalePolicy.md)); `auto_color()` has nothing to validate.
- The mode and the limits are read back through `Pane::color_axis()` ([`axis_accessors`](axis_accessors.md)).""",
    see=["[`Pane::color_limits`](color_limits.md): sets a manual range.", "[`Pane::color_limit_method`](color_limit_method.md): chooses how an automatic range is fitted.", "[`LimitMethod`](../types/LimitMethod.md): the three methods, with a rendered comparison."],
)
