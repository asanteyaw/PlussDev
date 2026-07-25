<!-- pluss-hand-authored -->

# Thread::lor

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::lor(const Thread& other) const -> Thread
```

## What It Does

Elementwise logical OR, following [`land()`](land.md)'s shared mechanics: single
overload, `Bool`-only requirement, positional matching, and display labels carried
from `*this`.

## Parameters

**other** (`const Thread&`)

A `Bool`-typed `Series` thread, the same length as `*this`.

## Returns

`Thread`
    A new `Bool`-typed thread, same length as `*this`, holding the elementwise
    logical OR.

## Examples

Example 1. OR two boolean threads.

```cpp
using namespace pluss::table;

auto left = Thread(pluss::make_series({true, false, false}, pluss::ScalarType::Bool));
auto right = Thread(pluss::make_series({false, false, true}, pluss::ScalarType::Bool));

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

- [`Thread::ror`](ror.md): the reflected form.
- [`Thread::land`](land.md): the canonical reference for this family's shared
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
