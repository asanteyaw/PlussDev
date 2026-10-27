<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7cad1b711f5e -->

# Slot::min

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot Slot::min() const
```

## What It Does

Returns a new, single-value slot holding the minimum of the slot's
values.

## Parameters

`None`

## Returns

`Slot`
    A new `Series` slot with exactly one value: the minimum.

## Examples

Example 1. Find the minimum of three values.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});
auto out = slot.min();
std::cout << std::get<pluss::Series>(out.at(0)).host<std::int64_t>()[0];
```

### Output

```text
1
```

## Notes

- Always collapses to exactly one value.
- Bridges through [`kindelize()`](kindelize.md) internally — requires the
  slot to be stream-compatible (a single, homogeneous numeric kind).
- Requires the slot to be defined — an undefined `Slot` throws
  `"Slot: undefined handle"`.

## See Also

- [`Slot::max`](max.md): the mirror reduction.
- [`Slot::sum`](sum.md), [`Slot::mean`](mean.md): the other reductions.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mean.html">
    <span class="label">Previous</span>
    <span class="title">mean</span>
  </a>
  <a class="next-link" href="ndistinct.html">
    <span class="label">Next</span>
    <span class="title">ndistinct</span>
  </a>
</div>
