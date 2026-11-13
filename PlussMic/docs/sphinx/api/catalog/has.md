<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 63e705043173 -->

# Catalog::has

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
bool Catalog::has(std::string_view key) const
```

## What It Does

Checks whether a named stream exists in the catalog, without throwing regardless of the
answer. This is the safe way to test for a key's presence before calling
[`at()`](at.md) (which throws on a missing key) — or use [`get()`](get.md) directly if you
also want the stream itself when present.

## Parameters

**key** (`std::string_view`)
    The stream name to check.

## Returns

`bool`
    `true` if a stream with that exact name exists; `false` otherwise (including for a
    misspelled or differently-cased name — key lookup is exact and case-sensitive).

## Examples

Example 1. An existing key returns `true`.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
});

std::cout << std::boolalpha << ct.has("metric");
```

### Output

```text
true
```

Example 2. A missing key returns `false` — no throw.

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
});

std::cout << std::boolalpha << ct.has("zzz");
```

### Output

```text
false
```

Example 3. Key lookup is exact and case-sensitive — a differently-cased name is treated as
missing.

```cpp
auto ct = pluss::kindle::Catalog({
    "Metric"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})),
});

std::cout << std::boolalpha << ct.has("metric") << " " << ct.has("Metric");
```

### Output

```text
false true
```

## Notes

- Never throws — `has()` is the safe presence check, unlike [`at()`](at.md), which throws
  on a missing key.
- Key comparison is exact and case-sensitive.

## See Also

- [`Catalog::at`](at.md): fetch the stream directly, throwing if the key is missing.
- [`Catalog::get`](get.md): fetch the stream as `std::optional<Stream>`, combining the
  presence check and the fetch into one call.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="group.html">
    <span class="label">Previous</span>
    <span class="title">group</span>
  </a>
  <a class="next-link" href="head.html">
    <span class="label">Next</span>
    <span class="title">head</span>
  </a>
</div>
