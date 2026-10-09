<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3685d9f2e68b -->

# Catalog::defined

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::defined() const noexcept -> bool
```

## What It Does

Reports whether the catalog owns a concrete backing handle at all — the same
undefined/defined distinction `Slate::defined()` and `DataFrame::defined()` make. A
default-constructed `Catalog` is **undefined**; almost every other method throws
`"Catalog: undefined handle"` if called on one. A `Catalog` built from an explicit (even
empty) stream list is defined — `defined()` is about handle existence, not about whether
the catalog has any streams. See the important callout below.

```{important} An empty-but-defined catalog is different from an undefined one
`Catalog{}` (default-constructed) is **undefined** — `defined()` is `false`, and calling
almost anything else on it throws. `Catalog(std::vector<std::pair<std::string, Stream>>{})`
(an explicit, zero-length stream list) is **defined but empty** — `defined()` is `true`,
[`empty()`](empty.md) is also `true`, and every introspection method works normally,
reporting zero streams. See Example 3.
```

## Parameters

`None`

## Returns

`bool`
    `true` if the catalog owns a backing handle; `false` for a default-constructed
    (moved-from, or otherwise handle-less) catalog.

## Examples

Example 1. A catalog built from named streams is defined.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
});

std::cout << std::boolalpha << ct.defined();
```

### Output

```text
true
```

Example 2. A default-constructed catalog is undefined, and most other methods throw when
called on it.

```cpp
pluss::kindle::Catalog ct;
std::cout << std::boolalpha << ct.defined() << "\n";

try {
  auto n = ct.nkeys();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
false
Catalog: undefined handle
```

Example 3. An explicitly empty stream list produces a *defined* catalog with zero
streams — not an undefined one. Compare with Example 2.

```cpp
pluss::kindle::Catalog ct(std::vector<std::pair<std::string, pluss::kindle::Stream>>{});
std::cout << std::boolalpha << ct.defined() << "\n";
std::cout << std::boolalpha << ct.empty() << "\n";
std::cout << ct.nkeys();
```

### Output

```text
true
true
0
```

## Notes

- `noexcept`: safe to call on any catalog, including an undefined one.
- Undefined is not the same as empty — see the important callout above and Example 3.
- Almost every other `Catalog` method throws `"Catalog: undefined handle"` when called on
  an undefined catalog; check `defined()` first if you're unsure of a catalog's origin.

## See Also

- [`Catalog::empty`](empty.md): whether a *defined* catalog has zero streams — a different
  question from whether it's defined at all.
- [`Catalog::unique`](unique.md): whether this catalog is the sole owner of its backing
  storage, a separate question from whether that storage exists.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="similarity.html">
    <span class="label">Previous</span>
    <span class="title">similarity</span>
  </a>
  <a class="next-link" href="display.html">
    <span class="label">Next</span>
    <span class="title">display</span>
  </a>
</div>
