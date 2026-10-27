<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: fc974589619f -->

# Slot::ndistinct

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot Slot::ndistinct(pluss::kindle::DistinctOptions options = {}) const
```

## What It Does

Returns a new, single-value slot holding the count of distinct values in
the source slot.

## Parameters

**options** (`pluss::kindle::DistinctOptions`, optional)
    `{bool dropnull = true}` — whether null values are dropped before
    counting distinct values. Defaults to dropping them.

## Returns

`Slot`
    A new `Series[int64]` slot with exactly one value: the distinct count.

## Examples

Example 1. A repeated value among three leaves two distinct values.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(1), pluss::make_series(2)});
auto out = slot.ndistinct();
std::cout << std::get<pluss::Series>(out.at(0)).host<std::int64_t>()[0];
```

### Output

```text
2
```

## Notes

- Always collapses to exactly one value, regardless of the source slot's
  size.
- Bridges through [`kindelize()`](kindelize.md) internally — requires the
  slot to be stream-compatible.
- Requires the slot to be defined — an undefined `Slot` throws
  `"Slot: undefined handle"`.

## See Also

- [`Slot::distinct`](distinct.md): the actual distinct values, rather
  than just their count.
- [`Slot::freq`](freq.md): per-value frequency counts.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="min.html">
    <span class="label">Previous</span>
    <span class="title">min</span>
  </a>
  <a class="next-link" href="notnull.html">
    <span class="label">Next</span>
    <span class="title">notnull</span>
  </a>
</div>
