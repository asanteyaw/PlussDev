<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 67568b96d67b -->

# Catalog::freq

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::freq(FreqOptions options = {}) const -> Catalog
```

### `FreqOptions` fields

| Field | Type | Default |
|---|---|---|
| `top` | `std::int64_t` | `0` |
| `dropnull` | `bool` | `true` |

## What It Does

Computes an occurrence count per distinct value, independently for **every** stream — each
stream's own `freq()`, in count order. Like [`distinct()`](distinct.md), this operates on
each stream separately, not as a joint frequency table across the whole catalog; the
resulting catalog is very likely ragged, for the same reason.

## Parameters

**options** (`FreqOptions`, optional)
    `top` (default `0`, meaning unlimited) caps each stream's result to its `top` most
    frequent values. `dropnull` (default `true`) excludes `NULL` from the count.

## Returns

`Catalog`
    A new catalog with every stream replaced by its own occurrence counts, ordered most-
    to least-frequent — streams are very likely to end up different lengths (ragged).

## Examples

Example 1. Occurrence counts per stream, independently.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 2.0})),
    "region"_k = pluss::kindle::Stream(pluss::make_note({"east", "west", "east", "east"})),
});

auto out = ct.freq();
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [id, region]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[int64]    | 3       | [2, 1, 1]                                        
region                   | Series       | Series[int64]    | 2       | [3, 1]                                           
========================================================================================================================
```

`id`'s value `2.0` occurred twice (the most of any `id` value), so its count `2` leads;
`region`'s `"east"` occurred three times, leading its own count list. Note both results are
plain `Series[int64]` counts — the *values themselves* aren't shown, only how many times
each occurred, in descending order.

Example 2. Cap each stream's result to its top 2 most frequent values.

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 2.0})),
    "region"_k = pluss::kindle::Stream(pluss::make_note({"east", "west", "east", "east"})),
});

FreqOptions options;
options.top = 2;
auto out = ct.freq(options);
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [id, region]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[int64]    | 2       | [2, 1]                                           
region                   | Series       | Series[int64]    | 2       | [3, 1]                                           
========================================================================================================================
```

## Notes

- Operates per-stream, independently — like `distinct()`, the resulting catalog is very
  likely ragged.
- Results are counts (`Series[int64]`), not the original values paired with their counts —
  cross-reference against [`distinct()`](distinct.md) on the same stream if you need both.
- `top` (default `0`, unlimited) caps each stream's result length; `dropnull` (default
  `true`) excludes `NULL` from the count.

## See Also

- [`Catalog::distinct`](distinct.md): the corresponding distinct *values*, without counts.
- [`Catalog::ndistinct`](ndistinct.md): just the count of distinct values per stream.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="grep.html">
    <span class="label">Previous</span>
    <span class="title">grep</span>
  </a>
  <a class="next-link" href="from_json.html">
    <span class="label">Next</span>
    <span class="title">from_json</span>
  </a>
</div>
