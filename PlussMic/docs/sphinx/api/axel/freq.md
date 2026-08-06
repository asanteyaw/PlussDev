<!-- pluss-hand-authored -->

# Axel::freq

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
Axel Axel::freq(pluss::kindle::FreqOptions options = {}) const
```

## What It Does

Returns a new axel where each field's values, in every frame, are
replaced by their **frequency counts** — how many times each distinct
value occurs, sorted by descending frequency — applying
[`Registry::freq(options)`](../registry/freq.md) to every frame via
[`map()`](map.md).

## Parameters

**options** (`pluss::kindle::FreqOptions`, optional)
    `{std::int64_t top = 0, bool dropnull = true}` — `top` limits the
    result to the `top` most frequent values (`0` means no limit);
    `dropnull` controls whether nulls are excluded before counting.

## Returns

`Axel`
    A new axel where each field's `Slot`, in every frame, holds one
    count per distinct value, most frequent first.

## Examples

Example 1. A field with one repeated value: the repeated value's count
(`2`) sorts before the singleton's count (`1`).

```cpp
auto r0 = pluss::bundle::Registry({{"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(1), pluss::make_series(2)})}});
auto axel = pluss::bundle::Axel({r0});

auto out = axel.freq();
auto counts = out.registry(0).fields("id");
std::cout << "[" << std::get<pluss::Series>(counts.at(0)).to_vec<std::int64_t>()[0] << ", "
          << std::get<pluss::Series>(counts.at(1)).to_vec<std::int64_t>()[0] << "]";
```

### Output

```text
[2, 1]
```

## Notes

- Returns the frequency **counts**, not the distinct values themselves —
  pair with [`distinct()`](distinct.md) if you need both, in the same
  descending-frequency order.
- Per-field, independent — a previously-aligned axel can come out
  ragged, same as [`distinct()`](distinct.md).
- Never mutates the receiver — always returns a new `Axel`.

## See Also

- [`Axel::distinct`](distinct.md): the distinct values these counts
  correspond to.
- [`Axel::ndistinct`](ndistinct.md): the total distinct-value count.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fillnull.html">
    <span class="label">Previous</span>
    <span class="title">fillnull</span>
  </a>
  <a class="next-link" href="from_kindle.html">
    <span class="label">Next</span>
    <span class="title">from_kindle</span>
  </a>
</div>
