<!-- pluss-hand-authored -->

# Studio::tight_layout_enabled

[Back to Studio API Reference](../../studio_api_reference)

## Signature

```cpp
bool Studio::tight_layout_enabled() const noexcept
```

## What It Does

Returns whether tight layout is currently on — whatever [`tight_layout()`](tight_layout.md)
most recently set `enable` to, or `false` if `tight_layout()` was never called.

## Parameters

*(none)*

## Returns

`bool`
    `true` if tight layout is enabled, `false` otherwise.

## Examples

Example 1. Before and after enabling.

```cpp
using namespace pluss::pl;

auto fig = studio("tight_check");
std::cout << fig.tight_layout_enabled() << "\n";

fig.tight_layout(true);
std::cout << fig.tight_layout_enabled() << "\n";
```

### Output

```text
0
1
```

## Notes

- `noexcept`; `false` is the well-defined default.

## See Also

- [`Studio::tight_layout`](tight_layout.md): the only way to change this.
- [`Studio::tight_layout_padding`](tight_layout_padding.md): the companion accessor for the
  padding value.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tight_layout.html">
    <span class="label">Previous</span>
    <span class="title">tight_layout</span>
  </a>
  <a class="next-link" href="tight_layout_padding.html">
    <span class="label">Next</span>
    <span class="title">tight_layout_padding</span>
  </a>
</div>
