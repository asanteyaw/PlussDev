<!-- pluss-hand-authored -->

# Pane::y_major_grid

[Back to Pane API Reference](../../pane_api_reference)

## Signature

```cpp
Pane& Pane::y_major_grid(bool enable = true) noexcept
```

## What It Does

The y-axis counterpart of [`x_major_grid()`](x_major_grid.md): toggles major gridlines for
the y-axis specifically. Every detail carries over unchanged, including recomputing the
aggregate [`grid_style()`](grid.md) on every call — see
[`x_major_grid()`](x_major_grid.md)'s page for the full worked-example set.

## Parameters

**enable** (`bool`, default `true`)
    `true` to show major gridlines on the y-axis, `false` to hide them.

## Returns

`Pane&`
    A reference to the same pane, for chaining.

## Examples

Example 1. Enabling major gridlines on the y-axis only.

```cpp
using namespace pluss::pl;

auto fig = studio("y_major_grid_check");
auto& pane = fig.pane2();
pane.trend(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0}, std::vector<double>{2.0, 4.0, 3.0, 5.0, 6.0});
pane.y_major_grid(true);
fig.show();

std::cout << pane.y_axis().major_grid << "\n";
std::cout << pane.x_axis().major_grid << "\n";
std::cout << (pane.grid_style() == GridStyle::Major) << "\n";
```

### Output

```text
1
0
1
```

![Output of Example 1](img/y_major_grid-1-check.svg)

## Notes

See [`x_major_grid()`](x_major_grid.md)'s Notes — every detail applies identically here.

## See Also

- [`Pane::x_major_grid`](x_major_grid.md): the x-axis equivalent, with the full
  worked-example set.
- [`Pane::z_major_grid`](z_major_grid.md): the z-axis equivalent.
- [`Pane::y_minor_grid`](y_minor_grid.md): the minor-gridline counterpart for the y-axis.
- [`Pane::grid`](grid.md): the uniform, all-axes setter.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="x_major_grid.html">
    <span class="label">Previous</span>
    <span class="title">x_major_grid</span>
  </a>
  <a class="next-link" href="z_major_grid.html">
    <span class="label">Next</span>
    <span class="title">z_major_grid</span>
  </a>
</div>
