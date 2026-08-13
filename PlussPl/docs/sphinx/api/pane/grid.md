<!-- pluss-hand-authored -->

# Pane::grid

[Back to Pane API Reference](../../pane_api_reference)

## Signatures

```cpp
Pane& Pane::grid(GridStyle style) noexcept   // sets all three axes uniformly
GridStyle Pane::grid_style() const noexcept  // reads back the aggregate
```

## What It Does

Sets a single, uniform grid style across **all three axes at once** — `GridStyle::None`
(no gridlines, the default), `GridStyle::Major` (gridlines at major tick positions on
every axis), or `GridStyle::MajorMinor` (major *and* minor gridlines on every axis).
`grid_style()` reads the value back.

```{important} grid() and the per-axis toggles are two views onto the SAME underlying flags
`grid()`/`grid_style()` are not independent state — internally, every axis's `major_grid`
and `minor_grid` boolean flags (the same fields
[`x_major_grid`](x_major_grid.md)/[`x_minor_grid`](x_minor_grid.md) and their y/z siblings
read and write) are what actually drive rendering, and `grid_style_` is kept in sync with
them in **both directions**:

- Calling `grid(style)` **overwrites** all six per-axis flags uniformly to match the
  requested style, discarding any prior per-axis customization. See Example 4.
- Calling any per-axis toggle (`x_major_grid`, `y_minor_grid`, ...) instead **recomputes**
  the aggregate `grid_style()` by OR-ing across all three axes: if *any* axis has its
  minor flag on, the aggregate becomes `MajorMinor`; else if *any* axis has its major flag
  on, the aggregate becomes `Major`; else `None`. One axis turning a flag off does **not**
  necessarily drop the aggregate if another axis still has that flag on. See Example 5 on
  [`x_major_grid`](x_major_grid.md) and Example 1 on [`x_minor_grid`](x_minor_grid.md).

In short: `grid()` is a bulk "set everything the same way" convenience; the per-axis
methods are the more granular, real underlying state, and `grid_style()` is always a
derived summary of whatever those six flags currently say — never an independently
stored value of its own.
```

## Parameters

**style** (`GridStyle`, setter only)
    `None`, `Major`, or `MajorMinor` — applied uniformly to all three axes.

## Returns

**Setter:** `Pane&` — a reference to the same pane, for chaining.

**Getter:** `GridStyle` — the current aggregate style.

## Examples

Example 1. The default: `GridStyle::None`, no gridlines.

```cpp
using namespace pluss::pl;

auto fig = studio("grid_none_default");
auto& pane = fig.pane2();
pane.trend(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0}, std::vector<double>{2.0, 4.0, 3.0, 5.0, 6.0});
fig.show();

std::cout << (pane.grid_style() == GridStyle::None) << "\n";
```

### Output

```text
1
```

![Output of Example 1](img/grid-1-none.svg)

Example 2. `GridStyle::Major` — major gridlines on every axis.

```cpp
using namespace pluss::pl;

auto fig = studio("grid_major");
auto& pane = fig.pane2();
pane.trend(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0}, std::vector<double>{2.0, 4.0, 3.0, 5.0, 6.0});
pane.grid(GridStyle::Major);
fig.show();

std::cout << pane.x_axis().major_grid << " " << pane.y_axis().major_grid << " " << pane.z_axis().major_grid << "\n";
std::cout << pane.x_axis().minor_grid << " " << pane.y_axis().minor_grid << " " << pane.z_axis().minor_grid << "\n";
```

### Output

```text
1 1 1
0 0 0
```

![Output of Example 2](img/grid-2-major.svg)

Example 3. `GridStyle::MajorMinor` — major and minor gridlines on every axis.

```cpp
using namespace pluss::pl;

auto fig = studio("grid_majorminor");
auto& pane = fig.pane2();
pane.trend(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0}, std::vector<double>{2.0, 4.0, 3.0, 5.0, 6.0});
pane.grid(GridStyle::MajorMinor);
fig.show();

std::cout << pane.x_axis().minor_grid << " " << pane.y_axis().minor_grid << " " << pane.z_axis().minor_grid << "\n";
```

### Output

```text
1 1 1
```

![Output of Example 3](img/grid-3-majorminor.svg)

Example 4. `grid()` overwrites prior per-axis customization uniformly — a per-axis
difference set up first (`x_major_grid(true)`, y left `false`) is wiped out by a
subsequent `grid(GridStyle::Major)` call, which forces both axes to `true`.

```cpp
using namespace pluss::pl;

auto fig = studio("grid_overwrites_per_axis");
auto& pane = fig.pane2();
pane.trend(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0}, std::vector<double>{2.0, 4.0, 3.0, 5.0, 6.0});

pane.x_major_grid(true).y_major_grid(false);
std::cout << "before grid(): x=" << pane.x_axis().major_grid << " y=" << pane.y_axis().major_grid << "\n";

pane.grid(GridStyle::Major);
std::cout << "after grid(Major): x=" << pane.x_axis().major_grid << " y=" << pane.y_axis().major_grid << "\n";
```

### Output

```text
before grid(): x=1 y=0
after grid(Major): x=1 y=1
```

## Notes

- Both overloads are `noexcept`.
- `GridStyle::None` is `0`, `Major` is `1`, `MajorMinor` is `2`.
- See the important callout above: `grid()`/`grid_style()` and the six per-axis toggles
  are two views onto the same underlying state, kept in sync in both directions, not
  independent settings.
- `grid(style)` is the simplest way to apply one grid style uniformly; reach for the
  per-axis toggles (`x_major_grid`, `y_minor_grid`, ...) when different axes need
  different grid treatment.

## See Also

- [`Pane::x_major_grid`](x_major_grid.md) / [`Pane::y_major_grid`](y_major_grid.md) /
  [`Pane::z_major_grid`](z_major_grid.md): the per-axis major-gridline toggles.
- [`Pane::x_minor_grid`](x_minor_grid.md) / [`Pane::y_minor_grid`](y_minor_grid.md) /
  [`Pane::z_minor_grid`](z_minor_grid.md): the per-axis minor-gridline toggles.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="z_label.html">
    <span class="label">Previous</span>
    <span class="title">z_label</span>
  </a>
  <a class="next-link" href="x_major_grid.html">
    <span class="label">Next</span>
    <span class="title">x_major_grid</span>
  </a>
</div>
