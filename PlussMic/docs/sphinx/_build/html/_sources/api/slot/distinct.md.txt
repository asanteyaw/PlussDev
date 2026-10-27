<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b63f74da9785 -->

# Slot::distinct

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot Slot::distinct(pluss::kindle::DistinctOptions options = {}) const
```

## What It Does

Returns a new slot with duplicate values removed, preserving first-occurrence
order.

## Parameters

**options** (`pluss::kindle::DistinctOptions`, optional)
    `{bool dropnull = true}` — whether null values are dropped before
    deduplication. Defaults to dropping them.

## Returns

`Slot`
    A new slot holding each distinct value once, in first-occurrence order.

## Examples

Example 1. A repeated value collapses to one occurrence.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(1), pluss::make_series(2)});
auto out = slot.distinct();
std::cout << "size=" << out.size() << " vals="
          << std::get<pluss::Series>(out.at(0)).host<std::int64_t>()[0] << ","
          << std::get<pluss::Series>(out.at(1)).host<std::int64_t>()[0];
```

### Output

```text
size=2 vals=1,2
```

## Notes

- Bridges through [`kindelize()`](kindelize.md) internally — requires the
  slot to be stream-compatible (a single, homogeneous value kind).
- `options.dropnull` defaults to `true`; pass `{.dropnull = false}` to
  keep a single null as one of the distinct values.
- Requires the slot to be defined — an undefined `Slot` throws
  `"Slot: undefined handle"`.

## See Also

- [`Slot::freq`](freq.md): the frequency count for each distinct value,
  rather than just the values themselves.
- [`Slot::ndistinct`](ndistinct.md): the distinct-value *count*, rather
  than the values.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="defined.html">
    <span class="label">Previous</span>
    <span class="title">defined</span>
  </a>
  <a class="next-link" href="dropnull.html">
    <span class="label">Next</span>
    <span class="title">dropnull</span>
  </a>
</div>
