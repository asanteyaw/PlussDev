<!-- pluss-hand-authored -->

# flash::set_context

[Back to flash API Reference](../../flash_api_reference)

## Signatures

```cpp
enum class FlashContext : std::uint8_t { Inherit, Paper, Notebook, Talk, Poster };

Studio& flash::set_context(
    Studio& studio, double width, double height,
    double tick_font_scale = 1.0, double axis_label_font_scale = 1.0)

Studio& flash::plotting_context(
    Studio& studio, double width, double height,
    double tick_font_scale = 1.0, double axis_label_font_scale = 1.0)

Studio& flash::set_context(
    Studio& studio, FlashContext context,
    std::optional<double> width = std::nullopt, std::optional<double> height = std::nullopt)
```

## What It Does

A figure-sizing helper — two independent overloads:

- **`(Studio&, double, double, double, double)`** sets the figure's canvas size
  directly (via [`Studio::size`](../studio/size.md)) and its tick/axis-label font
  scale (via [`Studio::context_scales`](../studio/context_scales.md)), then re-applies
  that same tick font scale to every pane already in the figure. `plotting_context` is
  an exact alias, verified byte-identical below.
- **`(Studio&, FlashContext, optional<double>, optional<double>)`** picks one of five
  named presets — each a fixed `(width, height, tick_scale, axis_scale)` quadruple — and
  forwards into the raw-numbers overload above, with `width`/`height` letting you
  override just the size while keeping the preset's font scale.

## Parameters

**studio** (`Studio&`)
    The figure to resize/rescale.

**width**, **height** (`double`, raw overload)
    The new canvas size in points, forwarded to [`Studio::size`](../studio/size.md).

**tick_font_scale**, **axis_label_font_scale** (`double`, default `1.0` each, raw
overload)
    Forwarded to [`Studio::context_scales`](../studio/context_scales.md), and also
    re-applied per-pane to every pane currently in the figure (via the same internal
    tick-font-scale helper `flash::set_theme` uses).

**context** (`FlashContext`, enum overload)
    See the field table below.

**width**, **height** (`std::optional<double>`, default empty, enum overload)
    When set, used **instead of** the preset's own base width/height (the preset's tick/
    axis font scale is still applied). When `context == FlashContext::Inherit` and
    either is set, only [`Studio::size`](../studio/size.md) is called (no font-scale
    change) — see the important callout below.

## `FlashContext` values

Each non-`Inherit` value maps to a fixed `(width, height, tick_font_scale,
axis_label_font_scale)` quadruple:

**Paper** — `920 × 640`, scale `0.8`/`0.8`
    The smallest preset — sized for print/publication figures.

**Notebook** — `1200 × 780`, scale `1.0`/`1.0`
    The baseline preset — identical values to `Inherit`'s own fallback numbers (see
    below), just reached via an explicit named context instead of leaving sizing alone.

**Talk** — `1440 × 900`, scale `1.3`/`1.3`
    Larger canvas and font scale, sized for presentation/talk slides.

**Poster** — `1840 × 1160`, scale `1.6`/`1.6`
    The largest preset, sized for poster printing.

```{important} Inherit is not "no preset" -- it is genuinely special-cased
FlashContext::Inherit skips the preset table entirely: if width/height are also both
empty, set_context(Studio&, FlashContext, ...) does nothing at all (early return, no
Studio::size or Studio::context_scales call). If either width or height IS set while
context is Inherit, ONLY Studio::size is called with that value (falling back to the
studio's current extent() for whichever of width/height was left empty) -- tick/axis
font scale is left completely untouched in this path. This is the one case where
set_context does not call the raw-numbers overload's font-scale logic at all.
```

## Returns

`Studio&`
    A reference to `studio`, for chaining.

## Examples

Example 1. Raw overload — explicit size and font scale.

```cpp
using namespace pluss::pl;

auto fig = studio("flash_set_context_raw");
auto& pane = fig.pane2();
pane.trend(std::vector<double>{1, 2, 3}, std::vector<double>{1, 4, 9});
flash::set_context(fig, 900.0, 600.0, 1.2, 1.2);
std::cout << "set_context extent after call: " << fig.extent().width << "x" << fig.extent().height << "\n";
fig.show();
```

Output:

```text
set_context extent after call: 900x600
```

Example 2 (verification, not a distinct rendering). `plotting_context` is byte-identical
to the raw `set_context` overload.

```cpp
using namespace pluss::pl;

auto fig1 = studio("flash_set_context_raw");
auto& p1 = fig1.pane2();
p1.trend(std::vector<double>{1, 2, 3}, std::vector<double>{1, 4, 9});
flash::set_context(fig1, 900.0, 600.0, 1.2, 1.2);

auto fig2 = studio("flash_plotting_context_alias");
auto& p2 = fig2.pane2();
p2.trend(std::vector<double>{1, 2, 3}, std::vector<double>{1, 4, 9});
flash::plotting_context(fig2, 900.0, 600.0, 1.2, 1.2);
std::cout << "set_context(raw) vs plotting_context byte-identical: " << (fig1.to_svg() == fig2.to_svg()) << "\n";
```

Output:

```text
set_context(raw) vs plotting_context byte-identical: 1
```

Example 3. `FlashContext::Talk` preset.

```cpp
using namespace pluss::pl;

auto fig = studio("flash_set_context_enum_talk");
auto& pane = fig.pane2();
pane.trend(std::vector<double>{1, 2, 3}, std::vector<double>{1, 4, 9});
flash::set_context(fig, flash::FlashContext::Talk);
std::cout << "set_context(Talk) extent: " << fig.extent().width << "x" << fig.extent().height << "\n";
```

Output:

```text
set_context(Talk) extent: 1440x900
```

Example 4 (edge case). `FlashContext::Inherit` with an explicit `width`/`height`
override — only the size changes; font scale is left untouched (see the important
callout above).

```cpp
using namespace pluss::pl;

auto fig = studio("flash_set_context_enum_inherit_resize");
auto& pane = fig.pane2();
pane.trend(std::vector<double>{1, 2, 3}, std::vector<double>{1, 4, 9});
flash::set_context(fig, flash::FlashContext::Inherit, 555.0, 333.0);
std::cout << "set_context(Inherit, w=555,h=333) extent: " << fig.extent().width << "x" << fig.extent().height << "\n";
```

Output:

```text
set_context(Inherit, w=555,h=333) extent: 555x333
```

## Notes

- Not `noexcept` for either overload.
- The raw overload's font-scale re-application loops over every pane
  [`Studio::panes`](../studio/panes.md) currently returns — panes added to the figure
  *after* this call do not retroactively get the scale (unless
  [`flash::set_theme(Studio&, ...)`](set_theme.md) was also used, which additionally
  stores `pane_style_defaults`).
- `Notebook`'s quadruple (`1200×780`, `1.0`/`1.0`) is numerically identical to what
  `Inherit` falls back to internally — the two are only distinguishable by whether the
  preset-table code path or the early-return code path actually ran.

## See Also

- [`flash::set_theme`](set_theme.md): calls this function's `FlashContext` overload
  internally as part of its own broader theming bundle.
- [`Studio::size`](../studio/size.md), [`Studio::context_scales`](../studio/context_scales.md):
  the two lower-level methods this function's raw overload forwards into directly.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="set_style.html">
    <span class="label">Previous</span>
    <span class="title">set_style</span>
  </a>
  <a class="next-link" href="move_legend.html">
    <span class="label">Next</span>
    <span class="title">move_legend</span>
  </a>
</div>
