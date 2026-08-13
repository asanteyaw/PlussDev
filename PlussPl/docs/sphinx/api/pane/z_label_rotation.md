<!-- pluss-hand-authored -->

# Pane::z_label_rotation

[Back to Pane API Reference](../../pane_api_reference)

## Signatures

```cpp
Pane& Pane::z_label_rotation(double degrees)          // sets an explicit rotation
Pane& Pane::clear_z_label_rotation() noexcept          // clears back to the default
```

## What It Does

The z-axis counterpart of [`x_label_rotation()`](x_label_rotation.md)/
[`y_label_rotation()`](y_label_rotation.md), meaningful primarily on 3D (`Volumetric`)
panes. Every detail carries over unchanged — see
[`x_label_rotation()`](x_label_rotation.md) for the full worked-example set.

## Parameters

**degrees** (`double`, setter only)
    The rotation angle in degrees. Must be finite.

## Returns

`Pane&`
    A reference to the same pane, for chaining.

## Examples

Example 1. A 45-degree rotation of the z-axis label on a 3D pane.

```cpp
using namespace pluss::pl;

auto fig = studio("z_label_rotation_45");
auto& pane = fig.pane3();
pane.path3(std::vector<double>{1.0, 2.0, 3.0}, std::vector<double>{1.0, 2.0, 3.0}, std::vector<double>{1.0, 2.0, 3.0});
pane.z_label("Depth");
pane.z_label_rotation(45.0);
fig.show();

std::cout << *pane.z_axis().label_rotation_deg << "\n";
```

### Output

```text
45
```

![Output of Example 1](img/z_label_rotation-1-45deg.svg)

## Notes

See [`x_label_rotation()`](x_label_rotation.md)'s Notes — every detail applies identically
here. Most useful on 3D panes; on 2D panes the z-axis state exists but is inert.

## See Also

- [`Pane::x_label_rotation`](x_label_rotation.md) / [`Pane::y_label_rotation`](y_label_rotation.md):
  the x- and y-axis equivalents, with the full worked-example set.
- [`Pane::z_tick_rotation`](z_tick_rotation.md): rotates the tick labels, not the axis
  label.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="y_label_rotation.html">
    <span class="label">Previous</span>
    <span class="title">y_label_rotation</span>
  </a>
  <a class="next-link" href="x_tick_labels.html">
    <span class="label">Next</span>
    <span class="title">x_tick_labels</span>
  </a>
</div>
