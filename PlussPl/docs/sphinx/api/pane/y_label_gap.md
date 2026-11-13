<!-- pluss-hand-authored -->

# Pane::y_label_gap

[Back to Pane API Reference](../../pane_api_reference)

## Signature

```cpp
Pane& Pane::y_label_gap(double points)
Pane& Pane::clear_y_label_gap() noexcept
```

## What It Does

Sets the gap, in drawing units, between the y axis (and its tick labels) and the axis label. The setting is stored in `y_axis().label_gap` as an optional value: empty until a gap is set, which means the renderer chooses a gap from the font size. `y_label_gap` stores an explicit gap and `clear_y_label_gap()` removes it, restoring the automatic choice, so a figure that sets and then clears a gap is exactly the figure that never set one.

The gap must be finite and not negative. Anything else (a negative number, infinity or NaN) is refused with `pluss::Error`, the stored value is left as it was, and zero is valid and puts the two elements as close as the renderer allows. The gap is read when the figure is rendered. It is per axis: the other axes keep their own.

## Parameters

**points** (`double`)
    The gap in drawing units. Finite and not negative.

## Returns

`Pane&`
    The same pane, for chaining.

## Examples

Example 1. Default, set, clear. A new pane has no override (`label_gap` is empty); the call stores the value and `clear_y_label_gap()` removes it again.

```cpp
auto fig = studio("y_label_gap_readback");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{1, 2, 3, 4}, std::vector<double>{2, 5, 4, 3});
auto show = [&] { const auto& v = pane.y_axis().label_gap; std::cout << (v ? std::to_string(*v) : std::string("none")) << "\n"; };
std::cout << "new pane: "; show();
pane.y_label_gap(24.0);
std::cout << "after y_label_gap(24): "; show();
pane.clear_y_label_gap();
std::cout << "after clear_y_label_gap(): "; show();
fig.save_svg("figure.svg");
```

### Output

```text
new pane: none
after y_label_gap(24): 24.000000
after clear_y_label_gap(): none
```

![Output of Example 1](img/y_label_gap-1-readback.svg)

Example 2. The effect. With a label set, a gap of 0, 14 and 60 gives three different figures; the saved figure uses 60, which pushes the y label well away from the axis.

```cpp
auto build = [&](double gap) {
  auto fig = studio("y_label_gap_effect");
  auto& pane = fig.pane2("p");
  pane.trend(std::vector<double>{1, 2, 3, 4}, std::vector<double>{2, 5, 4, 3});
  pane.y_label("Axis label");
  pane.y_label_gap(gap);
  return fig;
};
std::cout << "0 vs 14: " << (build(0.0).to_svg() != build(14.0).to_svg()) << ", 14 vs 60: " << (build(14.0).to_svg() != build(60.0).to_svg()) << "\n";
build(60.0).save_svg("figure.svg");
```

### Output

```text
0 vs 14: 1, 14 vs 60: 1
```

![Output of Example 2](img/y_label_gap-2-effect.svg)

Example 3. Clearing restores the default exactly. After `clear_y_label_gap()` the figure equals the one that never set a gap.

```cpp
auto build = [&](bool set_then_clear) {
  auto fig = studio("y_label_gap_clear");
  auto& pane = fig.pane2("p");
  pane.trend(std::vector<double>{1, 2, 3, 4}, std::vector<double>{2, 5, 4, 3});
  pane.y_label("Axis label");
  if (set_then_clear) pane.y_label_gap(40.0).clear_y_label_gap();
  return fig.to_svg();
};
std::cout << "equal: " << (build(true) == build(false)) << "\n";
auto fig = studio("y_label_gap_clear_out");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{1, 2, 3, 4}, std::vector<double>{2, 5, 4, 3});
pane.y_label("Axis label");
fig.save_svg("figure.svg");
```

### Output

```text
equal: 1
```

![Output of Example 3](img/y_label_gap-3-clear_restores.svg)

Example 4. Invalid gaps are refused and the stored value is kept. A negative number, infinity and NaN throw `pluss::Error`; zero is a valid gap.

```cpp
auto fig = studio("y_label_gap_invalid");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{1, 2, 3, 4}, std::vector<double>{2, 5, 4, 3});
pane.y_label("Axis label");
pane.y_label_gap(10.0);
for (double bad : {-1.0, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()}) {
  try {
    pane.y_label_gap(bad);
  } catch (const pluss::Error& e) {
    std::cout << bad << ": " << e.message() << "\n";
  }
}
pane.y_label_gap(0.0);
std::cout << "zero accepted: " << *pane.y_axis().label_gap << "\n";
pane.y_label_gap(10.0);
std::cout << "kept after errors: " << *pane.y_axis().label_gap << "\n";
fig.save_svg("figure.svg");
```

### Output

```text
-1: y_label_gap: spacing must be finite and non-negative
inf: y_label_gap: spacing must be finite and non-negative
nan: y_label_gap: spacing must be finite and non-negative
zero accepted: 0
kept after errors: 10
```

![Output of Example 4](img/y_label_gap-4-invalid.svg)

## Notes

- `y_label_gap` throws, so it is not `noexcept`; `clear_y_label_gap` never throws.
- The related gap for the other element is [`y_tick_label_gap`](y_tick_label_gap.md).

## See Also

- [`Pane::y_label`](y_label.md): the label the gap is measured to.
- [`Pane::y_tick_label_gap`](y_tick_label_gap.md): the other gap.
- [`Pane::x_label_gap`](x_label_gap.md): the same setting for the x axis.
- [`Pane::z_label_gap`](z_label_gap.md): the same setting for the z axis.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="y_label_centered.html">
    <span class="label">Previous</span>
    <span class="title">y_label_centered</span>
  </a>
  <a class="next-link" href="y_label_rotation.html">
    <span class="label">Next</span>
    <span class="title">y_label_rotation</span>
  </a>
</div>
