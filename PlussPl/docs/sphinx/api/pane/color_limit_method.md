<!-- pluss-hand-authored -->

# Pane::color_limit_method

[Back to Pane API Reference](../../pane_api_reference)

## Signature

```cpp
Pane& Pane::color_limit_method(LimitMethod method) noexcept
```

## What It Does

The color-axis counterpart of [`x_limit_method()`](x_limit_method.md)/
[`y_limit_method()`](y_limit_method.md)/[`z_limit_method()`](z_limit_method.md): stores which
`LimitMethod` (`TickAligned`/`Tight`/`Padded`) governs automatic color-range fitting, the
same structural pattern as the three position-axis equivalents, and reads back correctly
through [`color_axis()`](axis_accessors.md).

```{important} No observed visual effect on rendered output, unlike the position axes
Directly tested with `fieldmap()`-driven color data: figures identical in every other way
but differing only in `color_limit_method()` (`TickAligned` vs. `Tight` vs. `Padded`, every
pairing) produced **byte-for-byte identical SVG output** in every case tried. This is
unlike [`x_limit_method()`](x_limit_method.md)/[`y_limit_method()`](y_limit_method.md)/
[`z_limit_method()`](z_limit_method.md), which do visibly change rendered output for their
respective position axes. The color axis's "padding" concept — spacing a spatial axis's
plotted points away from the plot area's edge — doesn't have an obvious equivalent for a
color legend, which may explain why no visible difference was found. Treat this as an
observed result for the cases tested here, not a documented guarantee that the setting is
inert in every circumstance — but don't expect `color_limit_method()` to visibly change a
color-mapped trace's appearance in ordinary use.
```

## Parameters

**method** (`LimitMethod`)
    `TickAligned` (default), `Tight`, or `Padded`.

## Returns

`Pane&`
    A reference to the same pane, for chaining.

## Examples

Example 1. Setting and reading back the value — this part works exactly as expected, even
though the rendered output doesn't visibly change (see the important callout above).

```cpp
using namespace pluss::pl;

auto fig = studio("color_limit_method_tight");
auto& pane = fig.pane2();
pane.fieldmap(std::vector<double>{1.0, 2.0}, std::vector<double>{1.0, 2.0}, pluss::series::linspace(0.0, 1.0, 4, pluss::ScalarType::Float64).reshape({2, 2}));
pane.color_limit_method(LimitMethod::Tight);
fig.show();

std::cout << (pane.color_axis().limit_method == LimitMethod::Tight) << "\n";
```

### Output

```text
1
```

![Output of Example 1](img/color_limit_method-1-tight.svg)

Example 2. All three values tested pairwise against each other on identical color data —
every comparison finds byte-for-byte identical SVG content.

```cpp
using namespace pluss::pl;

auto build = [](LimitMethod method) {
  auto fig = studio("a");
  auto& pane = fig.pane2();
  pane.fieldmap(std::vector<double>{1.0, 2.0}, std::vector<double>{1.0, 2.0}, pluss::series::linspace(0.0, 1.0, 4, pluss::ScalarType::Float64).reshape({2, 2}));
  pane.color_limit_method(method);
  return fig.to_svg();
};

const std::string tick_aligned = build(LimitMethod::TickAligned);
const std::string tight = build(LimitMethod::Tight);
const std::string padded = build(LimitMethod::Padded);

std::cout << (tick_aligned == tight) << "\n";
std::cout << (tight == padded) << "\n";
std::cout << (tick_aligned == padded) << "\n";
```

### Output

```text
1
1
1
```

## Notes

- `noexcept` — no validation, every `LimitMethod` value is valid.
- Structurally identical to `x_limit_method()`/`y_limit_method()`/`z_limit_method()`, but
  see the important callout above regarding its lack of observed visual effect.
- Still only relevant while [`color_limits()`](color_limits.md) hasn't set an explicit
  range — the same `AutoFit`-only relationship as the position axes, even though the
  effect of *which* method is chosen wasn't observable in testing.

## See Also

- [`Pane::x_limit_method`](x_limit_method.md) / [`Pane::y_limit_method`](y_limit_method.md) /
  [`Pane::z_limit_method`](z_limit_method.md): the position-axis equivalents, which do
  visibly affect rendered output.
- [`Pane::color_limits`](color_limits.md): sets an explicit range, making this method
  irrelevant either way.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="z_limit_method.html">
    <span class="label">Previous</span>
    <span class="title">z_limit_method</span>
  </a>
  <a class="next-link" href="axis_accessors.html">
    <span class="label">Next</span>
    <span class="title">axis_accessors</span>
  </a>
</div>
