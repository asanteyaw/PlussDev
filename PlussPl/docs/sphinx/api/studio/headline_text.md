<!-- pluss-hand-authored -->

# Studio::headline_text

[Back to Studio API Reference](../../studio_api_reference)

## Signature

```cpp
const std::string& Studio::headline_text() const noexcept
```

## What It Does

Returns the figure's current headline — whatever [`headline()`](headline.md) most recently
set, i.e. the text actually drawn above the figure's panes.

## Parameters

*(none)*

## Returns

`const std::string&`
    The current headline text, by const reference. Empty if [`headline()`](headline.md) was
    never called.

## Examples

Example 1. Reading the headline back.

```cpp
using namespace pluss::pl;

auto fig = studio("chain");
fig.rename("chained").headline("Chained calls");

std::cout << fig.headline_text() << "\n";
```

### Output

```text
Chained calls
```

## Notes

- `noexcept`; empty string is the well-defined default for a figure with no headline set.
- Unlike [`name()`](name.md), this text is genuinely drawn inside the rendered SVG — see
  [`headline()`](headline.md)'s Example 1.

## See Also

- [`Studio::headline`](headline.md): the only way to set the headline.
- [`Studio::name`](name.md): the sibling bookkeeping-only field, never rendered.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="context_scales.html">
    <span class="label">Previous</span>
    <span class="title">context_scales</span>
  </a>
  <a class="next-link" href="name.html">
    <span class="label">Next</span>
    <span class="title">name</span>
  </a>
</div>
