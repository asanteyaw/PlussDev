<!-- pluss-hand-authored -->

# flash::despine

[Back to flash API Reference](../../flash_api_reference)

## Signature

```cpp
Pane& flash::despine(Pane& pane, bool = true) noexcept
```

```{important} Returns the pane unchanged: panes draw no top or right spines
`despine` exists so that code written with the familiar name works. A planar pane draws only its left and bottom axes, so the top and right spines that the name refers to are never drawn and there is nothing to remove; the pane is returned as it is, whatever the `bool` is. The one control over the axes of a pane as a whole is [`Pane::axes_visible`](../pane/axes_visible.md), which hides both axis lines at once and is what [`flash::tickless`](tickless.md) uses.
```

## What It Does

Nothing. See the important callout above.

## Parameters

**pane** (`Pane&`)
    Accepted but not used.

**(unnamed)** (`bool`, default `true`)
    Accepted but not used — the parameter has no name in the signature, which is itself
    a strong signal this was left as a stub.

## Returns

`Pane&`
    A reference to `pane`, completely unmodified.

## Examples

Example 1 (verification). `despine`'s effect on a pane's rendered output: none.

```cpp
using namespace pluss::pl;

auto fig = studio("flash_despine_before");
auto& pane = fig.pane2();
pane.trend(std::vector<double>{1, 2, 3}, std::vector<double>{1, 4, 9});
const std::string before = fig.to_svg();

flash::despine(pane);
const std::string after = fig.to_svg();
std::cout << "despine no-op, svg unchanged: " << (before == after) << "\n";
```

Output:

```text
despine no-op, svg unchanged: 1
```

## Notes

- `noexcept`.
- If you actually want to hide a pane's axes/border entirely, use
  [`Pane::axes_visible(false)`](../pane/axes_visible.md) directly, or
  [`flash::tickless`](tickless.md) (which hides the axes frame while keeping tick
  labels), or [`flash::set_theme`](set_theme.md)'s `borderless` option (which calls
  `axes_visible(false)` internally). None of these give you finer per-side
  (top-only / right-only) spine control, since that granularity doesn't exist anywhere
  in `Pane`'s rendering model.

## See Also

- [`flash::tickless`](tickless.md): the function that actually does hide a pane's axes
  frame, via `Pane::axes_visible`.
- [`Pane::axes_visible`](../pane/axes_visible.md): the coarse, whole-frame visibility
  toggle that is the closest real equivalent available.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tight_layout.html">
    <span class="label">Previous</span>
    <span class="title">tight_layout</span>
  </a>
  <a class="next-link" href="tickless.html">
    <span class="label">Next</span>
    <span class="title">tickless</span>
  </a>
</div>
