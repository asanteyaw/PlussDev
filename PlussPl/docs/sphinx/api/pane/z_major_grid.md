<!-- pluss-hand-authored -->

# Pane::z_major_grid

[Back to Pane API Reference](../../pane_api_reference)

## Signature

```cpp
Pane& Pane::z_major_grid(bool enable = true) noexcept
```

## What It Does

The z-axis counterpart of [`x_major_grid()`](x_major_grid.md)/
[`y_major_grid()`](y_major_grid.md), meaningful primarily on 3D (`Volumetric`) panes.
Every detail carries over unchanged, including recomputing the aggregate
[`grid_style()`](grid.md) on every call.

## Parameters

**enable** (`bool`, default `true`)
    `true` to show major gridlines on the z-axis, `false` to hide them.

## Returns

`Pane&`
    A reference to the same pane, for chaining.

## Examples

Example 1. Enabling major gridlines on the z-axis of a 3D pane only — the aggregate
`grid_style()` still reports `Major`, even with x's own flag untouched.

```cpp
using namespace pluss::pl;

auto fig = studio("z_major_grid_check");
auto& pane = fig.pane3();
pane.path3(std::vector<double>{1.0, 2.0, 3.0}, std::vector<double>{1.0, 2.0, 3.0}, std::vector<double>{1.0, 2.0, 3.0});
pane.z_major_grid(true);
fig.show();

std::cout << pane.z_axis().major_grid << "\n";
std::cout << pane.x_axis().major_grid << "\n";
std::cout << (pane.grid_style() == GridStyle::Major) << "\n";
```

### Output

```text
1
0
1
```

![Output of Example 1](img/z_major_grid-1-check.svg)

## Notes

See [`x_major_grid()`](x_major_grid.md)'s Notes — every detail applies identically here.
Most useful on 3D panes; on 2D panes the z-axis state exists but is inert.

## See Also

- [`Pane::x_major_grid`](x_major_grid.md) / [`Pane::y_major_grid`](y_major_grid.md): the
  x- and y-axis equivalents, with the full worked-example set.
- [`Pane::z_minor_grid`](z_minor_grid.md): the minor-gridline counterpart for the z-axis.
- [`Pane::grid`](grid.md): the uniform, all-axes setter.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="y_major_grid.html">
    <span class="label">Previous</span>
    <span class="title">y_major_grid</span>
  </a>
  <a class="next-link" href="x_minor_grid.html">
    <span class="label">Next</span>
    <span class="title">x_minor_grid</span>
  </a>
</div>
