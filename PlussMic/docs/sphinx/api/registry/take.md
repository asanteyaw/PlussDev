<!-- pluss-hand-authored -->

# Registry::take

[Back to Registry API Reference](../../registry_api_reference)

## Signatures

```cpp
Registry Registry::take(std::span<const std::int64_t> indices) const
Registry Registry::take(const std::vector<std::int64_t>& indices) const
```

## What It Does

Returns a new registry keeping only the values at the given positions, in the
given order, applying [`Slot::take(indices)`](../slot/take.md) independently to
each field. Unlike [`slice()`](slice.md)/[`head()`](head.md)/[`tail()`](tail.md),
`take()` is not restricted to a contiguous range — indices may repeat, skip
around, or reorder rows arbitrarily.

```{important} Out-of-range indices throw — no clamping, no negative-index wraparound
Unlike `slice()`, `take()` performs no clamping and no Python-style negative-index
support. Each index is looked up via `Slot::at()`, which throws
`"Slot::at: index out of bounds"` for any index `< 0` or `>= size()` — for that
field. On a ragged registry, an index valid for one field can throw for a shorter
one.
```

## Parameters

**indices** (`std::span<const std::int64_t>` / `const std::vector<std::int64_t>&`)
    The positions to select, applied to every field independently. May repeat or
    appear out of order.

## Returns

`Registry`
    A new registry with each field's `Slot` rebuilt from the values at `indices`,
    in the order given.

## Examples

Example 1. Reorder and repeat positions.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3), pluss::make_series(4)})},
});

std::vector<std::int64_t> picks{3, 0, 0};
auto out = rg.take(picks);
auto ids = out.select("id");
std::cout << "[";
for (std::int64_t i = 0; i < ids.size(); ++i) {
  if (i) std::cout << ", ";
  std::cout << std::get<pluss::Series>(ids.at(i)).to_vec<std::int64_t>()[0];
}
std::cout << "]";
```

### Output

```text
[4, 1, 1]
```

Example 2. An out-of-range index throws — no clamping.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3), pluss::make_series(4)})},
});

std::vector<std::int64_t> picks{99};
try {
  auto out = rg.take(picks);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slot::at: index out of bounds
```

## Notes

- Applies [`Slot::take()`](../slot/take.md) independently per field — on a ragged
  registry, the same index list can succeed on a longer field and throw on a
  shorter one.
- No clamping and no negative-index support — contrast with
  [`slice()`](slice.md)'s important callout, which clamps and supports negative
  indices.
- [`sample()`](sample.md) is built on top of `take()`, with an internally generated
  shuffled index list.

## See Also

- [`Registry::slice`](slice.md): the clamping, negative-index-aware, contiguous-range
  alternative to this method.
- [`Registry::sample`](sample.md): random sampling, implemented via `take()`.
- [`Catalog::take`](../catalog/take.md): the equivalent operation on the kindle
  model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tail.html">
    <span class="label">Previous</span>
    <span class="title">tail</span>
  </a>
  <a class="next-link" href="take_values.html">
    <span class="label">Next</span>
    <span class="title">take_values</span>
  </a>
</div>
