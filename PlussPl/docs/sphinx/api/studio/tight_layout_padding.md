<!-- pluss-hand-authored -->

# Studio::tight_layout_padding

[Back to Studio API Reference](../../studio_api_reference)

## Signature

```cpp
double Studio::tight_layout_padding() const noexcept
```

## What It Does

Returns the padding value currently associated with tight layout — whatever
[`tight_layout()`](tight_layout.md) most recently set `padding` to, or `6.0` (the
constructor default) if `tight_layout()` was never called. Note that this value is stored
(and returned) regardless of whether tight layout is actually **enabled** — see
[`tight_layout_enabled()`](tight_layout_enabled.md).

## Parameters

*(none)*

## Returns

`double`
    The current padding value.

## Examples

Example 1. The default, and after an explicit custom padding.

```cpp
using namespace pluss::pl;

auto fig = studio("padding_check");
std::cout << fig.tight_layout_padding() << "\n";

fig.tight_layout(true, 20.0);
std::cout << fig.tight_layout_padding() << "\n";
```

### Output

```text
6
20
```

## Notes

- `noexcept`; `6.0` is the well-defined default.
- Reports the stored padding value even if tight layout is currently disabled — it's not
  reset back to `6.0` by `tight_layout(false)`, only overwritten by a subsequent
  `tight_layout(..., padding)` call with a new value.

## See Also

- [`Studio::tight_layout`](tight_layout.md): the only way to change this.
- [`Studio::tight_layout_enabled`](tight_layout_enabled.md): whether this value is
  currently in effect at all.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tight_layout_enabled.html">
    <span class="label">Previous</span>
    <span class="title">tight_layout_enabled</span>
  </a>
  <a class="next-link" href="pane_style_defaults.html">
    <span class="label">Next</span>
    <span class="title">pane_style_defaults</span>
  </a>
</div>
