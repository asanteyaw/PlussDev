<!-- pluss-hand-authored -->

# flash::plot_surface

[Back to flash API Reference](../../flash_api_reference)

## Signature

```cpp
template <class X, class Y, class Z>
Trace& flash::plot_surface(Pane& pane, X&& x, Y&& y, Z&& z)
```

## What It Does

A pure, one-line rename of [`flash::sheet`](sheet.md)'s bare `(x, y, z)` overload —
`plot_surface(pane, x, y, z)` is exactly `sheet(pane, x, y, z)`, nothing more. Exists
purely to offer a shorter, more familiar name some callers already expect.
Directly verified byte-identical. Note only the bare-arity overload is aliased —
[`flash::sheet`](sheet.md)'s separate-color-matrix `(x, y, z, c)` overload has no
`plot_surface` equivalent.

## Parameters

**pane** (`Pane&`)
    The target pane. Must be `Volumetric`.

**x**, **y**, **z** (series-convertible, must each be rank ≤ 3, and `z` rank-2 to satisfy
[`Pane::sheet`](../pane/sheet.md)'s own deeper requirement)
    The surface grid — identical requirements to [`flash::sheet`](sheet.md)'s bare
    overload.

## Returns

`Trace&`
    A reference to the newly created trace, for chaining.

## Examples

Example 1. Same data as [`flash::sheet`](sheet.md)'s own first example.

```cpp
using namespace pluss::pl;

auto fig = studio("flash_plot_surface");
auto& pane = fig.pane3();
std::vector<double> x{0.0, 1.0, 2.0, 3.0};
std::vector<double> y{0.0, 1.0, 2.0, 3.0};
auto z = pluss::series::linspace(0.0, 1.0, 16, pluss::ScalarType::Float64).reshape({4, 4});
flash::plot_surface(pane, x, y, z);
fig.show();
```

![Output of Example 1](img/flash-plot-surface-1-basic.svg)

Example 2 (verification, not a distinct rendering). `flash::plot_surface` renders
byte-identical to `flash::sheet`'s bare overload given the same arguments.

```cpp
using namespace pluss::pl;

std::vector<double> x{0.0, 1.0, 2.0, 3.0};
std::vector<double> y{0.0, 1.0, 2.0, 3.0};
auto z = pluss::series::linspace(0.0, 1.0, 16, pluss::ScalarType::Float64).reshape({4, 4});

auto fig1 = studio("flash_plot_surface_cmp1");
auto& pane1 = fig1.pane3();
flash::plot_surface(pane1, x, y, z);

auto fig2 = studio("flash_plot_surface_cmp2");
auto& pane2 = fig2.pane3();
flash::sheet(pane2, x, y, z);

std::cout << "plot_surface == sheet: " << (fig1.to_svg() == fig2.to_svg()) << "\n";
```

Output:

```text
plot_surface == sheet: 1
```

## Notes

- Not `noexcept`.
- No separate-color-matrix equivalent overload — use [`flash::sheet`](sheet.md) directly
  for a decoupled `c` fill matrix.

## See Also

- [`flash::sheet`](sheet.md): the function this is a pure rename of (bare overload
  only).
- [`flash::plot3D`](plot3D.md), [`flash::scatter3D`](scatter3D.md),
  [`flash::plot_wireframe`](plot_wireframe.md), [`flash::contour3D`](contour3D.md),
  [`flash::bar3d`](bar3d.md): the other familiar-naming aliases in this family.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="scatter3D.html">
    <span class="label">Previous</span>
    <span class="title">scatter3D</span>
  </a>
  <a class="next-link" href="plot_wireframe.html">
    <span class="label">Next</span>
    <span class="title">plot_wireframe</span>
  </a>
</div>
