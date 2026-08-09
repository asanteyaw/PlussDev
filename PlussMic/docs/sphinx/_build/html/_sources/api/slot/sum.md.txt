<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4c7d0d457c15 -->

# Slot::sum

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot Slot::sum() const
```

## What It Does

Returns a new, single-value slot holding the sum of the slot's values.

## Parameters

`None`

## Returns

`Slot`
    A new `Series` slot with exactly one value: the sum.

## Examples

Example 1. Sum three values.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});
auto out = slot.sum();
std::cout << std::get<pluss::Series>(out.at(0)).to_vec<std::int64_t>()[0];
```

### Output

```text
6
```

## Notes

- Always collapses to exactly one value.
- Bridges through [`to_kindle()`](to_kindle.md) internally — requires the
  slot to be stream-compatible (a single, homogeneous numeric kind).
- Requires the slot to be defined — an undefined `Slot` throws
  `"Slot: undefined handle"`.

## See Also

- [`Slot::mean`](mean.md), [`Slot::min`](min.md), [`Slot::max`](max.md):
  the sibling reduction methods.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="slice.html">
    <span class="label">Previous</span>
    <span class="title">slice</span>
  </a>
  <a class="next-link" href="tail.html">
    <span class="label">Next</span>
    <span class="title">tail</span>
  </a>
</div>
