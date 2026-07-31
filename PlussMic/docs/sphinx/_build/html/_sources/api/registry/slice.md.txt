<!-- pluss-hand-authored -->

# Registry::slice

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry Registry::slice(std::int64_t start, std::int64_t stop) const
```

## What It Does

Returns a new registry keeping the positional range `[start, stop)` of every
field, applying [`Slot::slice(start, stop)`](../slot/slice.md) independently to
each field. Supports Python-style negative indices (counting from the end) and
never throws for an out-of-range `start`/`stop` — both are clamped to each field's
own valid range.

```{important} Each field's range is normalized against its own length, independently
Because negative indices and clamping are resolved per field against that field's
own `size()`, the same `slice(start, stop)` call can select a different-looking
positional window on differently-sized fields in a ragged registry.
```

## Parameters

**start** (`std::int64_t`)
    The inclusive start position. Negative values count from the end of each
    field.

**stop** (`std::int64_t`)
    The exclusive stop position. Negative values count from the end of each field.

## Returns

`Registry`
    A new registry with each field's `Slot` restricted to `[start, stop)`.

## Examples

Example 1. A plain forward slice.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3), pluss::make_series(4)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi"), pluss::make_note("Ho")})},
});

auto out = rg.slice(1, 3);
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
[2, 3]
```

Example 2. Negative indices count from the end, Python-style.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3), pluss::make_series(4)})},
});

auto out = rg.slice(-2, -1);
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
[3]
```

## Notes

- Applies [`Slot::slice()`](../slot/slice.md) independently per field — see the
  important callout above for the ragged-registry consequence.
- `start`/`stop` are each clamped to `[0, size]` per field after resolving negative
  indices; an inverted or fully out-of-range request yields an empty result for
  that field rather than throwing.
- [`head()`](head.md), [`tail()`](tail.md), and [`cloc()`](cloc.md) are all
  expressible in terms of `slice()`.

## See Also

- [`Registry::head`](head.md) / [`Registry::tail`](tail.md): fixed-endpoint
  specializations of this method.
- [`Registry::cloc`](cloc.md): a thin wrapper — `cloc(row)` is `slice(row, row+1)`,
  `cloc(start, stop)` is `slice(start, stop)` directly.
- [`Catalog::slice`](../catalog/slice.md): the equivalent operation on the kindle
  model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sizes.html">
    <span class="label">Previous</span>
    <span class="title">sizes</span>
  </a>
  <a class="next-link" href="sum.html">
    <span class="label">Next</span>
    <span class="title">sum</span>
  </a>
</div>
