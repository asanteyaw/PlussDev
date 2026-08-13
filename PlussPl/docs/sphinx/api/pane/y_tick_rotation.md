<!-- pluss-hand-authored -->

# Pane::y_tick_rotation

[Back to Pane API Reference](../../pane_api_reference)

## Signatures

```cpp
Pane& Pane::y_tick_rotation(double degrees)          // sets an explicit rotation
Pane& Pane::clear_y_tick_rotation() noexcept          // clears back to the default
```

## What It Does

The y-axis counterpart of [`x_tick_rotation()`](x_tick_rotation.md)/
[`clear_x_tick_rotation()`](x_tick_rotation.md). Every detail carries over unchanged —
only validated for finiteness, no `[0, 360)` normalization — see
[`x_tick_rotation()`](x_tick_rotation.md) for the full worked-example set.

## Parameters

**degrees** (`double`, setter only)
    The rotation angle in degrees. Must be finite.

## Returns

`Pane&`
    A reference to the same pane, for chaining.

## Examples

Example 1. A negative rotation on the y-axis.

```cpp
using namespace pluss::pl;

auto fig = studio("y_tick_rotation_neg30");
auto& pane = fig.pane2();
pane.trend(std::vector<double>{1.0, 2.0, 3.0}, std::vector<double>{2.0, 4.0, 3.0});
pane.y_tick_rotation(-30.0);
fig.show();

std::cout << *pane.y_axis().tick_rotation_deg << "\n";
```

### Output

```text
-30
```

![Output of Example 1](img/y_tick_rotation-1-neg30.svg)

## Notes

See [`x_tick_rotation()`](x_tick_rotation.md)'s Notes — every detail applies identically
here.

## See Also

- [`Pane::x_tick_rotation`](x_tick_rotation.md): the x-axis equivalent, with the full
  worked-example set for validation failures.
- [`Pane::z_tick_rotation`](z_tick_rotation.md): the z-axis equivalent.
- [`Pane::y_label_rotation`](y_label_rotation.md): rotates the axis *label*, not its ticks.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="x_tick_rotation.html">
    <span class="label">Previous</span>
    <span class="title">x_tick_rotation</span>
  </a>
  <a class="next-link" href="z_tick_rotation.html">
    <span class="label">Next</span>
    <span class="title">z_tick_rotation</span>
  </a>
</div>
