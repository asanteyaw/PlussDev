<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7964b5dc15d1 -->

# Catalog::sort_by

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::sort_by(const std::vector<std::string>& fields, const std::vector<bool>& ascending = {}) const -> Catalog
```

## What It Does

Reorders every stream's rows by one or more named streams, applied as successive tie-break
levels — the first field is the primary sort key, the second breaks ties within it, and so
on. [`sort()`](sort.md) is the single-key convenience wrapper over this method. As with
`sort()`, every stream in the catalog moves together, keeping cross-stream row alignment.

## Parameters

**fields** (`const std::vector<std::string>&`)
    One or more stream names, in priority order (primary key first). Must be non-empty,
    and every name must exist.

**ascending** (`const std::vector<bool>&`, default `{}`)
    Per-field sort direction, matched positionally to `fields`. An empty vector (the
    default) sorts every field ascending. If non-empty, its size must exactly match
    `fields`' size.

## Returns

`Catalog`
    A new catalog with every stream's rows reordered by the given fields.

## Examples

Example 1. Sort by a primary key, with a secondary key breaking ties.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "region"_k = pluss::kindle::Stream(pluss::make_note({"west", "east", "east", "west"})),
    "id"_k     = pluss::kindle::Stream(pluss::make_series({4.0, 2.0, 1.0, 3.0})),
});

auto out = ct.sort_by({"region", "id"}, {true, true});
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [region, id]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
region                   | Note         | Note             | 4       | [east, east, west, west]                         
id                       | Series       | Series[float64]  | 4       | [1, 2, 3, 4]                                     
========================================================================================================================
```

Rows group by `region` first (`east` before `west`), then by `id` ascending within each
group.

Example 2. Omitting `ascending` (leaving it at its default empty vector) sorts every field
ascending.

```cpp
auto ct = pluss::kindle::Catalog({
    "region"_k = pluss::kindle::Stream(pluss::make_note({"west", "east", "east", "west"})),
    "id"_k     = pluss::kindle::Stream(pluss::make_series({4.0, 2.0, 1.0, 3.0})),
});

auto out = ct.sort_by({"id"});
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [region, id]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
region                   | Note         | Note             | 4       | [east, east, west, west]                         
id                       | Series       | Series[float64]  | 4       | [1, 2, 3, 4]                                     
========================================================================================================================
```

Example 3. An `ascending` size mismatch throws — it must be either empty or exactly match
`fields`' size.

```cpp
auto ct = pluss::kindle::Catalog({
    "region"_k = pluss::kindle::Stream(pluss::make_note({"west", "east"})),
    "id"_k     = pluss::kindle::Stream(pluss::make_series({4.0, 2.0})),
});

try {
  auto out = ct.sort_by({"region", "id"}, {true});
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Catalog::sort_by: ascending size must be 0 or equal to fields size
```

## Notes

- `fields` must be non-empty; throws `"Catalog::sort_by: at least one field is required"`
  otherwise.
- Every field name must exist; throws `"Catalog::sort_by: unknown key '...'"` otherwise.
- `ascending` must be empty (all-ascending default) or exactly match `fields`' length —
  see Example 3.
- Reorders every stream together, preserving cross-stream row alignment.

## See Also

- [`Catalog::sort`](sort.md): the single-key convenience wrapper over this method.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sort.html">
    <span class="label">Previous</span>
    <span class="title">sort</span>
  </a>
  <a class="next-link" href="sum.html">
    <span class="label">Next</span>
    <span class="title">sum</span>
  </a>
</div>
