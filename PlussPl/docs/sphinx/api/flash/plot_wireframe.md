<!-- pluss-hand-authored -->

# flash::plot_wireframe

[Back to flash API Reference](../../flash_api_reference)

## Signature

```cpp
template <class X, class Y, class Z>
Trace& flash::plot_wireframe(Pane& pane, X&& x, Y&& y, Z&& z)
```

## What It Does

A pure, one-line rename of [`flash::lattice`](lattice.md)'s bare `(x, y, z)` overload —
`plot_wireframe(pane, x, y, z)` is exactly `lattice(pane, x, y, z)`, nothing more. Exists
purely to offer a shorter, more familiar name some callers already expect.
Directly verified byte-identical. Note only the bare-arity overload is aliased —
[`flash::lattice`](lattice.md)'s separate-color-matrix `(x, y, z, c)` overload (which has
no observed rendering effect anyway — see that page's important callout) has no
`plot_wireframe` equivalent.

## Parameters

**pane** (`Pane&`)
    The target pane. Must be `Volumetric`.

**x**, **y**, **z** (series-convertible, must each be rank ≤ 3, and `z` rank-2 to satisfy
[`Pane::lattice`](../pane/lattice.md)'s own deeper requirement)
    The wireframe grid — identical requirements to [`flash::lattice`](lattice.md)'s bare
    overload.

## Returns

`Trace&`
    A reference to the newly created trace, for chaining.

## Examples

Example 1. Same data as [`flash::lattice`](lattice.md)'s own first example.

```cpp
using namespace pluss::pl;

auto fig = studio("flash_plot_wireframe");
auto& pane = fig.pane3();
std::vector<double> x{0.0, 1.0, 2.0, 3.0};
std::vector<double> y{0.0, 1.0, 2.0, 3.0};
auto z = pluss::series::linspace(0.0, 1.0, 16, pluss::ScalarType::Float64).reshape({4, 4});
flash::plot_wireframe(pane, x, y, z);
fig.show();
```

![Output of Example 1](img/flash-plot-wireframe-1-basic.svg)

Example 2 (verification, not a distinct rendering). `flash::plot_wireframe` renders
byte-identical to `flash::lattice`'s bare overload given the same arguments.

```cpp
using namespace pluss::pl;

std::vector<double> x{0.0, 1.0, 2.0, 3.0};
std::vector<double> y{0.0, 1.0, 2.0, 3.0};
auto z = pluss::series::linspace(0.0, 1.0, 16, pluss::ScalarType::Float64).reshape({4, 4});

auto fig1 = studio("flash_plot_wireframe_cmp1");
auto& pane1 = fig1.pane3();
flash::plot_wireframe(pane1, x, y, z);

auto fig2 = studio("flash_plot_wireframe_cmp2");
auto& pane2 = fig2.pane3();
flash::lattice(pane2, x, y, z);

std::cout << "plot_wireframe == lattice: " << (fig1.to_svg() == fig2.to_svg()) << "\n";
```

Output:

```text
plot_wireframe == lattice: 1
```

## Notes

- Not `noexcept`.
- No separate-color-matrix equivalent overload.

## See Also

- [`flash::lattice`](lattice.md): the function this is a pure rename of (bare overload
  only).
- [`flash::plot3D`](plot3D.md), [`flash::scatter3D`](scatter3D.md),
  [`flash::plot_surface`](plot_surface.md), [`flash::contour3D`](contour3D.md),
  [`flash::bar3d`](bar3d.md): the other familiar-naming aliases in this family.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="plot_surface.html">
    <span class="label">Previous</span>
    <span class="title">plot_surface</span>
  </a>
  <a class="next-link" href="contour3D.html">
    <span class="label">Next</span>
    <span class="title">contour3D</span>
  </a>
</div>
