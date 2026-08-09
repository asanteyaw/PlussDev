<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c7977a442951 -->

# Slot::freq

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot Slot::freq(pluss::kindle::FreqOptions options = {}) const
```

## What It Does

Returns a new slot where each value is replaced by its **frequency
count** — how many times each distinct value occurs, sorted by descending
frequency.

## Parameters

**options** (`pluss::kindle::FreqOptions`, optional)
    `{std::int64_t top = 0, bool dropnull = true}` — `top` limits the
    result to the `top` most frequent values (`0` means no limit);
    `dropnull` controls whether nulls are excluded before counting.

## Returns

`Slot`
    A new `Series[int64]` slot holding one count per distinct value, most
    frequent first.

## Examples

Example 1. A repeated value's count sorts before the singleton's.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(1), pluss::make_series(2)});
auto out = slot.freq();
std::cout << std::get<pluss::Series>(out.at(0)).to_vec<std::int64_t>()[0] << " "
          << std::get<pluss::Series>(out.at(1)).to_vec<std::int64_t>()[0];
```

### Output

```text
2 1
```

Example 2. Limit to the single most frequent value's count via `top`.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(1), pluss::make_series(2)});
pluss::kindle::FreqOptions options;
options.top = 1;
auto out = slot.freq(options);
std::cout << "size=" << out.size() << " val=" << std::get<pluss::Series>(out.at(0)).to_vec<std::int64_t>()[0];
```

### Output

```text
size=1 val=2
```

## Notes

- Returns the counts, not the distinct values themselves — pair with
  [`distinct()`](distinct.md) if both are needed, in the same
  descending-frequency order.
- Bridges through [`to_kindle()`](to_kindle.md) internally — requires the
  slot to be stream-compatible.
- Requires the slot to be defined — an undefined `Slot` throws
  `"Slot: undefined handle"`.

## See Also

- [`Slot::distinct`](distinct.md): the distinct values these counts
  correspond to.
- [`Slot::ndistinct`](ndistinct.md): the total distinct-value count.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="filter.html">
    <span class="label">Previous</span>
    <span class="title">filter</span>
  </a>
  <a class="next-link" href="from_kindle.html">
    <span class="label">Next</span>
    <span class="title">from_kindle</span>
  </a>
</div>
