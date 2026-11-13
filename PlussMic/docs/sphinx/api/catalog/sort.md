<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5b1056818d63 -->

# Catalog::sort

[Back to Catalog API Reference](../../catalog_api_reference)

## Signatures

```cpp
Catalog Catalog::sort(std::string_view by_field, bool ascending = true) const
Catalog Catalog::sort(std::span<const std::string_view> fields, const std::vector<bool>& ascending = {}) const
Catalog Catalog::sort(std::initializer_list<std::string_view> fields, const std::vector<bool>& ascending = {}) const
Catalog Catalog::sort(const std::vector<std::string>& fields, const std::vector<bool>& ascending = {}) const
```

## What It Does

Reorders every stream's rows by one or more named sort-key streams. Every stream in the
catalog is reordered together, keeping row alignment intact across the whole catalog, not
just the sort key itself.

`sort(...)` stays on the root catalog surface because the keys are intrinsic to the
operation itself: this is a whole-catalog row-order plan, not a payload accessor.

## Parameters

**by_field** (`std::string_view`)
    The single stream name to sort by. Must exist.

**ascending** (`bool`, default `true`)
    For the single-key overload, `true` sorts smallest-to-largest (or alphabetically for
    text); `false` reverses the order.

**fields** (`std::span<const std::string_view>` / `std::initializer_list<std::string_view>` /
`const std::vector<std::string>&`)
    The ordered list of sort-key streams for the multi-key overloads. Must be non-empty and
    every name must exist.

**ascending** (`const std::vector<bool>&`, default `{}`)
    Optional per-key sort directions for the multi-key overloads. Leave empty to sort every
    key ascending, or provide one flag per key.

## Returns

`Catalog`
    A new catalog with every stream's rows reordered by the requested key stream(s).

## Examples

Example 1. Sort ascending (the default) by a numeric stream.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({3.0, 1.0, 2.0})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 10.0, 20.0})),
});

auto out = ct.sort("id");
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [id, metric]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 3       | [1, 2, 3]                                        
metric                   | Series       | Series[float64]  | 3       | [10, 20, 30]                                     
========================================================================================================================
```

`metric`'s rows moved along with `id`'s — the whole catalog stays aligned to the new row
order, not just the sort key.

Example 2. Sort descending with `ascending = false`.

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({3.0, 1.0, 2.0})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 10.0, 20.0})),
});

auto out = ct.sort("id", false);
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [id, metric]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 3       | [3, 2, 1]                                        
metric                   | Series       | Series[float64]  | 3       | [30, 20, 10]                                     
========================================================================================================================
```

Example 3. Sort by multiple keys, with per-key direction flags.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "region"_k = pluss::kindle::Stream(pluss::make_note({"West", "East", "West", "East"})),
    "id"_k     = pluss::kindle::Stream(pluss::make_series({2.0, 2.0, 1.0, 1.0})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({20.0, 40.0, 10.0, 30.0})),
});

auto out = ct.sort({"region", "id"}, {true, false});
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=3
keys: [region, id, metric]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
region                   | Note         | Note             | 4       | [East, East, West, West]                         
id                       | Series       | Series[float64]  | 4       | [2, 1, 2, 1]                                     
metric                   | Series       | Series[float64]  | 4       | [40, 30, 20, 10]                                 
========================================================================================================================
```

## Notes

- Reorders *every* stream together, preserving cross-stream row alignment.
- The single-key overload is just the short form of the multi-key surface.
- The `NameRange` convenience overload forwards into the same multi-key logic as the
  listed span/vector overloads.
- `fields` must be non-empty for the multi-key overloads.
- If `ascending` is supplied for the multi-key overloads, it must either be empty or match
  `fields.size()`.

## See Also

- [`Catalog::slice`](slice.md): inspect a portion of the reordered catalog.
- [`Catalog::take`](take.md): reorder explicitly by positional picks instead of value-based
  sorting.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="slice.html">
    <span class="label">Previous</span>
    <span class="title">slice</span>
  </a>
  <a class="next-link" href="stats.html">
    <span class="label">Next</span>
    <span class="title">stats</span>
  </a>
</div>
