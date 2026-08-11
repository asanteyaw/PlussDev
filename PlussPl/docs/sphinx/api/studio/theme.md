<!-- pluss-hand-authored -->

# Studio::theme

[Back to Studio API Reference](../../studio_api_reference)

## Signature

```cpp
Studio& Studio::theme(Theme theme)
```

## What It Does

Replaces the figure's theme after construction — the same effect as passing an explicit
`theme` to the `Studio` constructor (see [`studio()`](studio.md)'s Example 3), just usable
after the figure already exists. `Theme` bundles a name, an accent color sequence, a
background color, a foreground (text/line) color, and a surface color ramp
(`PaletteRamp`) — every visual color choice in the figure ultimately traces back to it.

```{important} Only affects panes created afterward
Like [`context_scales()`](context_scales.md), `theme()` changes a **default** that new
panes pick up when they're created — panes already added to the figure (via
[`pane2()`](pane2.md)/[`pane3()`](pane3.md)) keep whatever theme was in effect at the
moment *they* were created, and calling `theme()` afterward does not retroactively
restyle them. Call `theme()` before adding panes for it to have any visible effect on
those panes.
```

## Parameters

**theme** (`Theme`)
    The new theme. Two built-in presets ship as static factories:

    - `Theme::paper()` — light background, dark foreground; the default every `Studio`
      starts on unless told otherwise.
    - `Theme::ocean()` — a distinct accent palette and surface ramp.

    The free functions `pluss::pl::themes::paper()`/`themes::ocean()` are equivalent
    aliases for the same two presets, useful when a caller wants a `Theme` value without
    reaching through the `Theme` class name itself.

## Returns

`Studio&`
    A reference to the same figure, for chaining.

## Examples

Example 1. The default theme (`Theme::paper()`), never explicitly set.

```cpp
using namespace pluss::pl;

auto fig = studio("paper_theme");
auto& pane = fig.pane2();
flash::lineplot(pane, std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0}, std::vector<double>{2.0, 4.0, 3.0, 5.0, 6.0});
fig.show();

std::cout << fig.theme_tokens().name() << "\n";
```

### Output

```text
paper
```

![Output of Example 1](img/theme-1-paper.svg)

Example 2. Switching to `Theme::ocean()` before adding the pane — the rendered output
genuinely differs (a different byte count, not just a different theme name reported back).

```cpp
using namespace pluss::pl;

auto fig = studio("ocean_theme");
fig.theme(Theme::ocean());

auto& pane = fig.pane2();
flash::lineplot(pane, std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0}, std::vector<double>{2.0, 4.0, 3.0, 5.0, 6.0});
fig.show();

std::cout << fig.theme_tokens().name() << "\n";
```

### Output

```text
ocean
```

![Output of Example 2](img/theme-1-ocean.svg)

## Notes

- Only affects panes created after the call — see the important callout above.
- `Theme::paper()`/`Theme::ocean()` are the two built-in presets; `themes::paper()`/
  `themes::ocean()` (free functions in `pluss::pl::themes`) return the same values.
- Returns `*this` by reference — chains with any other mutator.

## See Also

- [`studio()`](studio.md): set an initial theme at construction time instead.
- [`Studio::theme_tokens`](theme_tokens.md): reads the current theme back, and — via its
  non-`const` overload — allows direct in-place field mutation without a full `theme()`
  replacement.
- [`Studio::pane_style_defaults`](pane_style_defaults.md): a related "affects panes created
  afterward" default, for axis/grid/legend style rather than color.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rename.html">
    <span class="label">Previous</span>
    <span class="title">rename</span>
  </a>
  <a class="next-link" href="headline.html">
    <span class="label">Next</span>
    <span class="title">headline</span>
  </a>
</div>
