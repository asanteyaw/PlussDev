<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b01832046037 -->

# Catalog::at

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::at(std::string_view key) const -> Stream
```

## What It Does

Fetches the named stream directly, by exact key match — the throwing counterpart to
[`get()`](get.md), for when a missing key genuinely represents a programming error you want
to fail loudly on, rather than a case to handle gracefully.

## Parameters

**key** (`std::string_view`)
    The stream name to fetch. Must exist exactly (case-sensitive) — see [`has()`](has.md)
    to check first without risking a throw.

## Returns

`Stream`
    The stream stored under `key`.

## Examples

Example 1. Fetch an existing stream by name.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
});

auto out = ct.at("metric");
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=3
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 10                                                                                                       
1            | 20                                                                                                       
2            | 30                                                                                                       
========================================================================================================================
```

Example 2. Fetching a missing key throws.

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
});

try {
  auto out = ct.at("zzz");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Catalog: key 'zzz' not found
```

## Notes

- Throws on a missing key — use [`get()`](get.md) instead if a missing key is an
  expected, handleable case rather than an error.
- Key comparison is exact and case-sensitive, same as [`has()`](has.md).
- Returns a `Stream` by value — `Stream` itself is a cheap, copy-on-write handle, so this
  is not an expensive copy of the underlying data.

## See Also

- [`Catalog::get`](get.md): the non-throwing counterpart, returning
  `std::optional<Stream>`.
- [`Catalog::has`](has.md): check for a key's existence without fetching anything.
- [`Catalog::keys`](keys.md): the name-argument overloads select one or more streams for
  further transformation, rather than fetching a single `Stream` directly.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="astype.html">
    <span class="label">Previous</span>
    <span class="title">astype</span>
  </a>
  <a class="next-link" href="begin.html">
    <span class="label">Next</span>
    <span class="title">begin</span>
  </a>
</div>
