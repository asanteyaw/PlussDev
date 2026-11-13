PAGE = dict(
    slug="auto_z",
    title="Pane::auto_z",
    signature="Pane& Pane::auto_z() noexcept",
    intro="""Returns the z axis of a 3D pane to **automatic fitting**: it discards a manual range set with [`z_limits`](z_limits.md) and puts the axis back in `LimitMode::AutoFit`, so the range is fitted to the data again by the axis's [`LimitMethod`](../types/LimitMethod.md). It is the way back from a manual range; there is no other call that clears one.

The method that was chosen with [`z_limit_method`](z_limit_method.md) is not touched, so it applies again as soon as fitting is automatic. On an axis that is already automatic the call does nothing, and it never throws. It acts on a pane of either kind, but only a 3D pane draws a z axis.""",
    parameters="*(none)*",
    returns="""`Pane&`
    A reference to the same pane, for chaining.""",
    examples=[
        dict(id="back_to_auto", text="A manual range, then `auto_z()`. The mode is `AutoFit` at the start, `Manual` after `z_limits`, and `AutoFit` again after `auto_z()`.",
             code="""auto fig = studio("auto_z_back");
auto& pane = fig.pane3("auto_z");
pane.path3(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0}, std::vector<double>{0.2, 0.9, 0.4});
auto mode = [&]() { return static_cast<int>(pane.z_axis().limit_mode); };
std::cout << "AutoFit = 0, Manual = 1\\n";
std::cout << "at the start:   " << mode() << "\\n";
pane.z_limits(Range1d{-5.0, 5.0});
std::cout << "after z_limits: " << mode() << "\\n";
pane.auto_z();
std::cout << "after auto_z:  " << mode() << ", limits cleared: " << !pane.z_axis().limits.has_value() << "\\n";
fig.save_svg("figure.svg");"""),
        dict(id="identical", text="After `auto_z()` the figure is the figure of a pane that never had a manual range: the SVG text of the two is identical.",
             code="""std::string svg[2];
for (int i = 0; i < 2; ++i) {
  auto fig = studio("auto_z_identical");
  auto& pane = fig.pane3("auto_z");
  pane.path3(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0}, std::vector<double>{0.2, 0.9, 0.4});
  if (i == 1) {
    pane.z_limits(Range1d{-5.0, 5.0});
    pane.auto_z();
  }
  svg[i] = fig.to_svg();
}
std::cout << "never set == set and cleared: " << (svg[0] == svg[1]) << "\\n";
auto fig = studio("auto_z_identical_out");
auto& pane = fig.pane3("auto_z");
pane.path3(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0}, std::vector<double>{0.2, 0.9, 0.4});
fig.save_svg("figure.svg");"""),
        dict(id="method_kept", text="The fitting method survives the round trip. `Tight` is set, a manual range replaces it, and after `auto_z()` the axis is fitted `Tight` again, as the printed method shows.",
             code="""auto fig = studio("auto_z_method");
auto& pane = fig.pane3("auto_z");
pane.path3(std::vector<double>{0, 1, 2}, std::vector<double>{0, 1, 0}, std::vector<double>{0.2, 0.9, 0.4});
pane.z_limit_method(LimitMethod::Tight);
pane.z_limits(Range1d{-5.0, 5.0});
std::cout << "while manual, method: " << static_cast<int>(pane.z_axis().limit_method) << " (TickAligned = 0, Tight = 1, Padded = 2)\\n";
pane.auto_z();
std::cout << "after auto_z, method: " << static_cast<int>(pane.z_axis().limit_method) << ", mode " << static_cast<int>(pane.z_axis().limit_mode) << "\\n";
fig.save_svg("figure.svg");"""),
    ],
    notes="""- `auto_z()` returns the pane, so it chains: `pane.auto_z().z_limit_method(LimitMethod::Padded)`.
- A manual range set with `z_limits` is validated against the axis scale ([`ScalePolicy`](../types/ScalePolicy.md)); `auto_z()` has nothing to validate.
- The mode and the limits are read back through `Pane::z_axis()` ([`axis_accessors`](axis_accessors.md)).""",
    see=["[`Pane::z_limits`](z_limits.md): sets a manual range.", "[`Pane::z_limit_method`](z_limit_method.md): chooses how an automatic range is fitted.", "[`LimitMethod`](../types/LimitMethod.md): the three methods, with a rendered comparison."],
)
