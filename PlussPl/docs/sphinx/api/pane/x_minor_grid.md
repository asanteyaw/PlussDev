<!-- pluss-hand-authored -->

# Pane::x_minor_grid

[Back to Pane API Reference](../../pane_api_reference)

## Signature

```cpp
Pane& Pane::x_minor_grid(bool enable = true) noexcept
```

## What It Does

Turns the minor grid of the x axis on or off. The minor grid is the set of lines drawn across the plot at the positions between the major ticks of the x axis; it is off by default. `true` (the default argument) turns it on, `false` turns it off, and the state is stored in `x_axis().minor_grid`. Turning it off again gives exactly the figure that never turned it on.

The two grids of an axis are independent: the minor grid can be on with the major grid off. Each axis has its own, and [`Pane::grid`](grid.md) sets all of them at once; [`Pane::grid_style`](grid_style.md) summarises the state of the whole pane as `None`, `Major` or `MajorMinor`. The call never throws and is read when the figure is rendered.

## Parameters

**enable** (`bool`, default `true`)
    `true` draws the grid, `false` removes it.

## Returns

`Pane&`
    The same pane, for chaining.

## Examples

Example 1. Off by default. `x_axis().minor_grid` is false for a new axis; the call with no argument turns it on, `false` turns it off, and the last call wins.

```cpp
auto fig = studio("x_minor_grid_readback");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{1, 2, 3, 4}, std::vector<double>{2, 5, 4, 3});
std::cout << "new axis: " << pane.x_axis().minor_grid << "\n";
pane.x_minor_grid();
std::cout << "after x_minor_grid(): " << pane.x_axis().minor_grid << "\n";
pane.x_minor_grid(false);
std::cout << "after x_minor_grid(false): " << pane.x_axis().minor_grid << "\n";
pane.x_minor_grid(true);
fig.save_svg("figure.svg");
```

### Output

```text
new axis: 0
after x_minor_grid(): 1
after x_minor_grid(false): 0
```

![Output of Example 1](img/x_minor_grid-1-readback.svg)

Example 2. The effect. Turning the minor grid on adds grid lines to the figure, which can be counted as extra paths in the SVG; turning it off again restores the figure exactly.

```cpp
auto build = [&](bool on) {
  auto fig = studio("x_minor_grid_effect");
  auto& pane = fig.pane2("p");
  pane.trend(std::vector<double>{1, 2, 3, 4}, std::vector<double>{2, 5, 4, 3});
  pane.x_minor_grid(on);
  return fig.to_svg();
};
auto paths = [](const std::string& svg) { std::size_t n = 0; for (auto at = svg.find("<path"); at != std::string::npos; at = svg.find("<path", at + 1)) ++n; return n; };
std::cout << "paths without: " << paths(build(false)) << ", with: " << paths(build(true)) << "\n";
std::cout << "off again equals never set: " << (build(false) == build(false)) << "\n";
auto fig = studio("x_minor_grid_effect_out");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{1, 2, 3, 4}, std::vector<double>{2, 5, 4, 3});
pane.x_minor_grid();
fig.save_svg("figure.svg");
```

### Output

```text
paths without: 19, with: 43
off again equals never set: 1
```

![Output of Example 2](img/x_minor_grid-2-effect.svg)

Example 3. Per axis. The x grid does not turn on the grid of the other axes, and each axis has its own switches.

```cpp
auto fig = studio("x_minor_grid_axes");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{1, 2, 3, 4}, std::vector<double>{2, 5, 4, 3});
pane.x_minor_grid();
std::cout << "x: " << pane.x_axis().minor_grid << ", y: " << pane.y_axis().minor_grid << ", z: " << pane.z_axis().minor_grid << "\n";
fig.save_svg("figure.svg");
```

### Output

```text
x: 1, y: 0, z: 0
```

![Output of Example 3](img/x_minor_grid-3-per_axis.svg)

Example 4. The pane's grid summary. `Pane::grid_style()` reports `None` (0), `Major` (1) or `MajorMinor` (2) from the axis switches; the minor grid of this axis moves it as shown.

```cpp
auto fig = studio("x_minor_grid_summary");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{1, 2, 3, 4}, std::vector<double>{2, 5, 4, 3});
std::cout << "no grid: " << static_cast<int>(pane.grid_style()) << "\n";
pane.x_major_grid();
std::cout << "major on: " << static_cast<int>(pane.grid_style()) << "\n";
pane.x_minor_grid();
std::cout << "major and minor on: " << static_cast<int>(pane.grid_style()) << "\n";
pane.x_minor_grid(true);
fig.save_svg("figure.svg");
```

### Output

```text
no grid: 0
major on: 1
major and minor on: 2
```

![Output of Example 4](img/x_minor_grid-4-summary.svg)

Example 5. Independent of the major grid. The minor grid can be on while the major grid is off; the counts show how many paths each combination draws.

```cpp
auto paths = [](const std::string& svg) { std::size_t n = 0; for (auto at = svg.find("<path"); at != std::string::npos; at = svg.find("<path", at + 1)) ++n; return n; };
auto build = [&](bool major, bool minor) {
  auto fig = studio("x_minor_grid_independent");
  auto& pane = fig.pane2("p");
  pane.trend(std::vector<double>{1, 2, 3, 4}, std::vector<double>{2, 5, 4, 3});
  pane.x_major_grid(major).x_minor_grid(minor);
  return fig.to_svg();
};
std::cout << "none: " << paths(build(false, false)) << ", major only: " << paths(build(true, false)) << ", minor only: " << paths(build(false, true)) << ", both: " << paths(build(true, true)) << "\n";
auto fig = studio("x_minor_grid_independent_out");
auto& pane = fig.pane2("p");
pane.trend(std::vector<double>{1, 2, 3, 4}, std::vector<double>{2, 5, 4, 3});
pane.x_major_grid().x_minor_grid();
fig.save_svg("figure.svg");
```

### Output

```text
none: 19, major only: 26, minor only: 43, both: 50
```

![Output of Example 5](img/x_minor_grid-5-independent.svg)

## Notes

- `noexcept`.
- To turn the grid of every axis on or off in one call, use [`Pane::grid`](grid.md).

## See Also

- [`Pane::x_major_grid`](x_major_grid.md): the major grid of the same axis.
- [`Pane::grid`](grid.md): all axes at once.
- [`Pane::grid_style`](grid_style.md): the summary of the grid.
- [`Pane::y_minor_grid`](y_minor_grid.md): the same setting for the y axis.
- [`Pane::z_minor_grid`](z_minor_grid.md): the same setting for the z axis.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="x_major_grid.html">
    <span class="label">Previous</span>
    <span class="title">x_major_grid</span>
  </a>
  <a class="next-link" href="x_tick_format.html">
    <span class="label">Next</span>
    <span class="title">x_tick_format</span>
  </a>
</div>
