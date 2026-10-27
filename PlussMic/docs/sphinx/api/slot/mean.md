<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ee7684612221 -->

# Slot::mean

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot Slot::mean() const
```

## What It Does

Returns a new, single-value slot holding the arithmetic mean of the
slot's values.

## Parameters

`None`

## Returns

`Slot`
    A new `Series[float64]` slot with exactly one value: the mean.

## Examples

Example 1. Average three values.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});
auto out = slot.mean();
std::cout << std::get<pluss::Series>(out.at(0)).host<double>()[0];
```

### Output

```text
2
```

## Notes

- Always collapses to exactly one value, and the result is always
  `float64` regardless of the source values' dtype.
- Bridges through [`kindelize()`](kindelize.md) internally — requires the
  slot to be stream-compatible (a single, homogeneous numeric kind).
- Requires the slot to be defined — an undefined `Slot` throws
  `"Slot: undefined handle"`.

## See Also

- [`Slot::sum`](sum.md), [`Slot::min`](min.md), [`Slot::max`](max.md): the
  sibling reduction methods.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="max.html">
    <span class="label">Previous</span>
    <span class="title">max</span>
  </a>
  <a class="next-link" href="min.html">
    <span class="label">Next</span>
    <span class="title">min</span>
  </a>
</div>
