<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: fde1e2db1e7f -->

# Slot::max

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot Slot::max() const
```

## What It Does

Returns a new, single-value slot holding the maximum of the slot's
values.

## Parameters

`None`

## Returns

`Slot`
    A new `Series` slot with exactly one value: the maximum.

## Examples

Example 1. Find the maximum of three values.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});
auto out = slot.max();
std::cout << std::get<pluss::Series>(out.at(0)).to_vec<std::int64_t>()[0];
```

### Output

```text
3
```

## Notes

- Always collapses to exactly one value.
- Bridges through [`kindelize()`](kindelize.md) internally — requires the
  slot to be stream-compatible (a single, homogeneous numeric kind).
- Requires the slot to be defined — an undefined `Slot` throws
  `"Slot: undefined handle"`.

## See Also

- [`Slot::min`](min.md): the mirror reduction.
- [`Slot::sum`](sum.md), [`Slot::mean`](mean.md): the other reductions.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="kindelize.html">
    <span class="label">Previous</span>
    <span class="title">kindelize</span>
  </a>
  <a class="next-link" href="mean.html">
    <span class="label">Next</span>
    <span class="title">mean</span>
  </a>
</div>
