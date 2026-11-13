<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 16af813a1847 -->

# Slate::lor

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
Slate Slate::lor(const Slate& other) const
```

## What It Does

Elementwise logical OR, following [`land()`](land.md)'s shared mechanics: single
overload, `Bool`-only requirement, positional matching, and display labels carried
from `*this`.

## Parameters

**other** (`const Slate&`)

A `Bool`-typed `Series` slate, the same length as `*this`.

## Returns

`Slate`
    A new `Bool`-typed slate, same length as `*this`, holding the elementwise
    logical OR.

## Examples

Example 1. OR two boolean slates.

```cpp
using namespace pluss::table;

auto left = Slate(pluss::make_series({true, false, false}, pluss::ScalarType::Bool));
auto right = Slate(pluss::make_series({false, false, true}, pluss::ScalarType::Bool));

auto out = left.lor(right);
std::cout << out;
```

### Output

```text
0  true
1  false
2  true

ptype: Series
```

Position `1` is the only one where OR reports `false` — the only position where
**both** operands are `false` at once.

## Notes

- See [`land()`](land.md) for the family-wide mechanics: the `Bool`-only
  requirement, positional matching, and the length-mismatch/non-`Bool` throws.
- See [`ror()`](ror.md) for how its reflected form's display-label behavior differs
  from `lor()`, the same way [`rand()`](rand.md) differs from `land()`.

## See Also

- [`Slate::ror`](ror.md): the reflected form.
- [`Slate::land`](land.md): the canonical reference for this family's shared
  mechanics.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="log.html">
    <span class="label">Previous</span>
    <span class="title">log</span>
  </a>
  <a class="next-link" href="lower.html">
    <span class="label">Next</span>
    <span class="title">lower</span>
  </a>
</div>
