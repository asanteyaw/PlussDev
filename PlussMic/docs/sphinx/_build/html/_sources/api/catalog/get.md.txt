<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 58495b6b10cb -->

# Catalog::get

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::get(std::string_view key) const -> std::optional<Stream>
```

## What It Does

Fetches the named stream, returning `std::nullopt` instead of throwing if the key is
missing — the non-throwing counterpart to [`at()`](at.md). Also tolerant of an undefined
catalog: calling `get()` on one returns `std::nullopt` rather than throwing
`"Catalog: undefined handle"`, unlike almost every other `Catalog` method.

```{important} get() is one of the few methods that tolerates an undefined catalog
Most `Catalog` methods throw `"Catalog: undefined handle"` when called on a
default-constructed catalog (see [`defined()`](defined.md)). `get()` is an exception: on an
undefined catalog it simply returns `std::nullopt`, the same as a missing key would. See
Example 3.
```

## Parameters

**key** (`std::string_view`)
    The stream name to fetch.

## Returns

`std::optional<Stream>`
    The stream stored under `key`, or `std::nullopt` if no such key exists (or the catalog
    itself is undefined).

## Examples

Example 1. Fetching an existing key returns an engaged `optional`.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
});

auto out = ct.get("metric");
std::cout << std::boolalpha << out.has_value();
```

### Output

```text
true
```

Example 2. A missing key returns `std::nullopt` — no throw, unlike [`at()`](at.md).

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
});

auto out = ct.get("zzz");
std::cout << std::boolalpha << out.has_value();
```

### Output

```text
false
```

Example 3. Calling `get()` on an undefined catalog also returns `std::nullopt`, rather
than throwing the way most other methods would.

```cpp
pluss::kindle::Catalog ct;
auto out = ct.get("anything");
std::cout << std::boolalpha << out.has_value();
```

### Output

```text
false
```

## Notes

- Returns `std::nullopt` both for a missing key and for an undefined catalog — see the
  important callout above.
- Unwrap the result with `*out`, `out.value()`, or `out.value_or(...)` once you've checked
  `has_value()`.

## See Also

- [`Catalog::at`](at.md): the throwing counterpart — use it when a missing key should be
  treated as an error.
- [`Catalog::has`](has.md): check presence without fetching the stream at all.
- [`Catalog::defined`](defined.md): the general undefined-handle check that most other
  methods enforce but `get()` deliberately doesn't.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="from_json.html">
    <span class="label">Previous</span>
    <span class="title">from_json</span>
  </a>
  <a class="next-link" href="group.html">
    <span class="label">Next</span>
    <span class="title">group</span>
  </a>
</div>
