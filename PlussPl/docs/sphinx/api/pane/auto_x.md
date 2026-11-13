<!-- pluss-hand-authored -->

# Pane::auto_x

[Back to Pane API Reference](../../pane_api_reference)

## Signature

```cpp
Pane& Pane::auto_x() noexcept
```

## What It Does

Returns the x axis to **automatic fitting**: it discards a manual range set with [`x_limits`](x_limits.md) and puts the axis back in `LimitMode::AutoFit`, so the range is fitted to the data again by the axis's [`LimitMethod`](../types/LimitMethod.md). It is the way back from a manual range; there is no other call that clears one.

The method that was chosen with [`x_limit_method`](x_limit_method.md) is not touched, so it applies again as soon as fitting is automatic. On an axis that is already automatic the call does nothing, and it never throws. 

## Parameters

*(none)*

## Returns

`Pane&`
    A reference to the same pane, for chaining.

## Examples

Example 1. A manual range, then `auto_x()`. The mode is `AutoFit` at the start, `Manual` after `x_limits`, and `AutoFit` again after `auto_x()`.

```cpp
auto fig = studio("auto_x_back");
auto& pane = fig.pane2("auto_x");
pane.trend(std::vector<double>{1, 2, 3, 4, 5}, std::vector<double>{2.3, 4.7, 3.1, 5.85, 6.2});
auto mode = [&]() { return static_cast<int>(pane.x_axis().limit_mode); };
std::cout << "AutoFit = 0, Manual = 1\n";
std::cout << "at the start:   " << mode() << "\n";
pane.x_limits(Range1d{0.0, 20.0});
std::cout << "after x_limits: " << mode() << "\n";
pane.auto_x();
std::cout << "after auto_x:  " << mode() << ", limits cleared: " << !pane.x_axis().limits.has_value() << "\n";
fig.save_svg("figure.svg");
```

### Output

```text
AutoFit = 0, Manual = 1
at the start:   0
after x_limits: 1
after auto_x:  0, limits cleared: 1
```

![Output of Example 1](img/auto_x-1-back_to_auto.svg)

Example 2. After `auto_x()` the figure is the figure of a pane that never had a manual range: the SVG text of the two is identical.

```cpp
std::string svg[2];
for (int i = 0; i < 2; ++i) {
  auto fig = studio("auto_x_identical");
  auto& pane = fig.pane2("auto_x");
  pane.trend(std::vector<double>{1, 2, 3, 4, 5}, std::vector<double>{2.3, 4.7, 3.1, 5.85, 6.2});
  if (i == 1) {
    pane.x_limits(Range1d{0.0, 20.0});
    pane.auto_x();
  }
  svg[i] = fig.to_svg();
}
std::cout << "never set == set and cleared: " << (svg[0] == svg[1]) << "\n";
auto fig = studio("auto_x_identical_out");
auto& pane = fig.pane2("auto_x");
pane.trend(std::vector<double>{1, 2, 3, 4, 5}, std::vector<double>{2.3, 4.7, 3.1, 5.85, 6.2});
fig.save_svg("figure.svg");
```

### Output

```text
never set == set and cleared: 1
```

![Output of Example 2](img/auto_x-2-identical.svg)

Example 3. The fitting method survives the round trip. `Tight` is set, a manual range replaces it, and after `auto_x()` the axis is fitted `Tight` again, as the printed method shows.

```cpp
auto fig = studio("auto_x_method");
auto& pane = fig.pane2("auto_x");
pane.trend(std::vector<double>{1, 2, 3, 4, 5}, std::vector<double>{2.3, 4.7, 3.1, 5.85, 6.2});
pane.x_limit_method(LimitMethod::Tight);
pane.x_limits(Range1d{0.0, 20.0});
std::cout << "while manual, method: " << static_cast<int>(pane.x_axis().limit_method) << " (TickAligned = 0, Tight = 1, Padded = 2)\n";
pane.auto_x();
std::cout << "after auto_x, method: " << static_cast<int>(pane.x_axis().limit_method) << ", mode " << static_cast<int>(pane.x_axis().limit_mode) << "\n";
fig.save_svg("figure.svg");
```

### Output

```text
while manual, method: 1 (TickAligned = 0, Tight = 1, Padded = 2)
after auto_x, method: 1, mode 0
```

![Output of Example 3](img/auto_x-3-method_kept.svg)

## Notes

- `auto_x()` returns the pane, so it chains: `pane.auto_x().x_limit_method(LimitMethod::Padded)`.
- A manual range set with `x_limits` is validated against the axis scale ([`ScalePolicy`](../types/ScalePolicy.md)); `auto_x()` has nothing to validate.
- The mode and the limits are read back through `Pane::x_axis()` ([`axis_accessors`](axis_accessors.md)).

## See Also

- [`Pane::x_limits`](x_limits.md): sets a manual range.
- [`Pane::x_limit_method`](x_limit_method.md): chooses how an automatic range is fitted.
- [`LimitMethod`](../types/LimitMethod.md): the three methods, with a rendered comparison.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="auto_color.html">
    <span class="label">Previous</span>
    <span class="title">auto_color</span>
  </a>
  <a class="next-link" href="auto_y.html">
    <span class="label">Next</span>
    <span class="title">auto_y</span>
  </a>
</div>
