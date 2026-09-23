<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1c72b53d56e0 -->

# Thread::lxor

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::lxor(const Thread& other) const -> Thread
```

## What It Does

Elementwise logical XOR, following [`land()`](land.md)'s shared mechanics: single
overload, `Bool`-only requirement, positional matching, and display labels carried
from `*this`. Unlike AND/OR, XOR reports `true` exactly where the two operands
**disagree**.

## Parameters

**other** (`const Thread&`)

A `Bool`-typed `Series` thread, the same length as `*this`.

## Returns

`Thread`
    A new `Bool`-typed thread, same length as `*this`, holding the elementwise
    logical XOR.

## Examples

Example 1. XOR two boolean threads.

```cpp
using namespace pluss::table;

auto left = Thread(pluss::make_series({true, false, true}, pluss::ScalarType::Bool));
auto right = Thread(pluss::make_series({true, true, false}, pluss::ScalarType::Bool));

auto out = left.lxor(right);
std::cout << out;
```

### Output

```text
0  false
1  true
2  true

ptype: Series
```

Position `0` (`true` vs `true`) agrees, so XOR reports `false`; positions `1` and `2`
disagree, so XOR reports `true`.

## Notes

- See [`land()`](land.md) for the family-wide mechanics.
- See [`rxor()`](rxor.md) for how its reflected form's display-label behavior
  differs, the same way [`rand()`](rand.md) differs from `land()`.

## See Also

- [`Thread::rxor`](rxor.md): the reflected form.
- [`Thread::land`](land.md): the canonical reference for this family's shared
  mechanics.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lt.html">
    <span class="label">Previous</span>
    <span class="title">lt</span>
  </a>
  <a class="next-link" href="map.html">
    <span class="label">Next</span>
    <span class="title">map</span>
  </a>
</div>
