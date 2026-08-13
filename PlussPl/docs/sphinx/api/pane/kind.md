<!-- pluss-hand-authored -->

# Pane::kind

[Back to Pane API Reference](../../pane_api_reference)

## Signature

```cpp
PaneKind Pane::kind() const noexcept
```

## What It Does

Returns which of the two flavors of pane this is: `PaneKind::Planar` (an ordinary 2D
plotting surface, created via [`Studio::pane2()`](../studio/pane2.md)) or
`PaneKind::Volumetric` (a 3D plotting surface, created via
[`Studio::pane3()`](../studio/pane3.md)). There is no setter — `kind()` is fixed
permanently at construction time by which factory method built the pane, and never
changes again over the pane's lifetime.

```{important} kind() gates which trace-creation methods are even callable
Every 2D-only trace method (`trend`, `cloud`, `columns`, `bucket`, `violin`, `contour`, …)
throws if called on a `Volumetric` pane, and every 3D-only trace method (`path3`,
`cloud3`, `spire3`, `plot_surface`-family helpers, …) throws if called on a `Planar` one.
`kind()` is how a pane's own API surface — practically speaking, which trace methods will
succeed rather than throw — is determined at runtime. See the individual trace-creation
method pages (for example [`Pane::trend`](trend.md)) for the exact throw message each one
uses.
```

Beyond gating which trace methods succeed, `kind()` also determines the pane's *default*
[`lens()`](lens.md) orientation at construction: a `Planar` pane starts with `Lens::planar()`
(a top-down, orthographic view suited to flat 2D data), while a `Volumetric` pane starts
with `Lens::spatial()` (an angled perspective-friendly view suited to 3D data). Both
defaults can be freely overridden afterward through `lens()`; `kind()` only controls the
*starting point*.

## Parameters

*(none)*

## Returns

`PaneKind`
    `PaneKind::Planar` or `PaneKind::Volumetric`.

## Examples

Example 1. A pane created via `pane2()` reports `Planar`.

```cpp
using namespace pluss::pl;

auto fig = studio("kind_planar");
auto& pane = fig.pane2();
pane.trend(std::vector<double>{1.0, 2.0, 3.0}, std::vector<double>{2.0, 4.0, 3.0});
fig.show();

std::cout << (pane.kind() == PaneKind::Planar) << "\n";
```

### Output

```text
1
```

![Output of Example 1](img/kind-1-planar.svg)

Example 2. A pane created via `pane3()` reports `Volumetric`.

```cpp
using namespace pluss::pl;

auto fig = studio("kind_volumetric");
auto& pane = fig.pane3();
pane.path3(std::vector<double>{0.0, 1.0}, std::vector<double>{0.0, 1.0}, std::vector<double>{0.0, 1.0});
fig.show();

std::cout << (pane.kind() == PaneKind::Volumetric) << "\n";
```

### Output

```text
1
```

![Output of Example 2](img/kind-2-volumetric.svg)

## Notes

- `noexcept`, `const` — a pure, cheap readback.
- Fixed permanently at construction; there is no way to change a pane's `kind()` after
  creation. Building the "other" kind of pane means creating a fresh one via
  `pane2()`/`pane3()`/`subpane2()`/`subpane3()`.
- Determines which trace-creation methods succeed vs. throw, and the pane's default
  `lens()` orientation — see the important callout above.

## See Also

- [`Studio::pane2`](../studio/pane2.md) / [`Studio::pane3`](../studio/pane3.md): the two
  factories that fix a pane's `kind()` for its whole lifetime.
- [`Pane::lens`](lens.md): the camera/orientation state whose default depends on `kind()`.
- [`Pane::trend`](trend.md), [`Pane::path3`](path3.md): representative 2D-only and
  3D-only trace-creation methods gated by `kind()`.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="title_text.html">
    <span class="label">Next</span>
    <span class="title">title_text</span>
  </a>
</div>
