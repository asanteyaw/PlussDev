<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 56b55cda85f1 -->

# Registry::freq

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry Registry::freq(pluss::kindle::FreqOptions options = {}) const
```

## What It Does

Returns a new registry where each field's values are replaced by their
**frequency counts** — how many times each distinct value occurs, sorted by
descending frequency — applying [`Slot::freq(options)`](../slot/freq.md) to
every field independently via [`map()`](map.md).

## Parameters

**options** (`pluss::kindle::FreqOptions`, optional)
    `{std::int64_t top = 0, bool dropnull = true}` — `top` limits the result to
    the `top` most frequent values (`0` means no limit); `dropnull` controls
    whether nulls are excluded before counting.

## Returns

`Registry`
    A new registry where each field's `Slot` holds one count per distinct value,
    most frequent first.

## Examples

Example 1. A field with one repeated value: the repeated value's count (`2`)
sorts before the singleton's count (`1`).

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(1), pluss::make_series(2)})},
});

auto out = rg.freq();
auto counts = out.fields("id");
std::cout << "[";
for (std::int64_t i = 0; i < counts.size(); ++i) {
  if (i) std::cout << ", ";
  std::cout << std::get<pluss::Series>(counts.at(i)).to_vec<std::int64_t>()[0];
}
std::cout << "]";
```

### Output

```text
[2, 1]
```

Example 2. Limit to the single most frequent value's count via `top`.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(1), pluss::make_series(2)})},
});

auto out = rg.freq(pluss::kindle::FreqOptions{1, true});
auto counts = out.fields("id");
std::cout << "[";
for (std::int64_t i = 0; i < counts.size(); ++i) {
  if (i) std::cout << ", ";
  std::cout << std::get<pluss::Series>(counts.at(i)).to_vec<std::int64_t>()[0];
}
std::cout << "]";
```

### Output

```text
[2]
```

## Notes

- Returns the frequency **counts**, not the distinct values themselves — pair
  with [`distinct()`](distinct.md) if you need both, in the same
  descending-frequency order.
- Per-field, independent — a previously-aligned registry can come out ragged,
  same as [`distinct()`](distinct.md).
- Full semantics documented on [`Slot::freq`](../slot/freq.md).
- Never mutates the receiver — always returns a new `Registry`.

## See Also

- [`Registry::distinct`](distinct.md): the distinct values these counts
  correspond to.
- [`Registry::ndistinct`](ndistinct.md): the total distinct-value count.
- [`Catalog::freq`](../catalog/freq.md): the equivalent operation on the kindle
  model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fillnull.html">
    <span class="label">Previous</span>
    <span class="title">fillnull</span>
  </a>
  <a class="next-link" href="get.html">
    <span class="label">Next</span>
    <span class="title">get</span>
  </a>
</div>
