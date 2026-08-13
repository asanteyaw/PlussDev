<!-- pluss-hand-authored -->

# Pane::scale_guide

[Back to Pane API Reference](../../pane_api_reference)

## Signatures

```cpp
Pane& Pane::scale_guide(bool enable = true) noexcept
bool Pane::scale_guide_enabled() const noexcept
```

## What It Does

Toggles a scale guide overlay on the pane — a supplementary visual reference distinct from
ordinary [`grid()`](grid.md) gridlines. `false` (the constructor default) draws nothing
extra; `true` enables it. The setter and accessor have different names
(`scale_guide`/`scale_guide_enabled`), unlike the several same-named setter/getter pairs
elsewhere in `Pane` (`legend_frame`, `legend_shape`, ...).

## Parameters

**enable** (`bool`, default `true`, setter only)
    `true` to show the scale guide, `false` to hide it.

## Returns

**Setter:** `Pane&` — a reference to the same pane, for chaining.

**Getter (`scale_guide_enabled`):** `bool` — whether it's currently enabled.

## Examples

Example 1. The default: disabled.

```cpp
using namespace pluss::pl;

auto fig = studio("scale_guide_default");
auto& pane = fig.pane2();
pane.trend(std::vector<double>{1.0, 2.0, 3.0}, std::vector<double>{2.0, 4.0, 3.0});
fig.show();

std::cout << pane.scale_guide_enabled() << "\n";
```

### Output

```text
0
```

![Output of Example 1](img/scale_guide-1-default.svg)

Example 2. Enabled — verified via direct SVG diff to render genuinely different output
from Example 1 (both happen to share the same byte length).

```cpp
using namespace pluss::pl;

auto fig = studio("scale_guide_on");
auto& pane = fig.pane2();
pane.trend(std::vector<double>{1.0, 2.0, 3.0}, std::vector<double>{2.0, 4.0, 3.0});
pane.scale_guide(true);
fig.show();

std::cout << pane.scale_guide_enabled() << "\n";
```

### Output

```text
1
```

![Output of Example 2](img/scale_guide-2-on.svg)

## Notes

- Both `scale_guide()` and `scale_guide_enabled()` are `noexcept`.
- `enable` defaults to `true`, so bare `scale_guide()` turns it on.
- Setter and getter have different names — unlike `legend_frame`/`legend_shape`, there's no
  overload-resolution ambiguity to worry about.

## See Also

- [`Pane::grid`](grid.md): the ordinary gridline system, a separate visual layer.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="legend_shape.html">
    <span class="label">Previous</span>
    <span class="title">legend_shape</span>
  </a>
  <a class="next-link" href="caption.html">
    <span class="label">Next</span>
    <span class="title">caption</span>
  </a>
</div>
