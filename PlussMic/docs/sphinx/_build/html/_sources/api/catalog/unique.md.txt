<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 405866cfe223 -->

# Catalog::unique

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::unique() const noexcept -> bool
```

## What It Does

Reports whether the current catalog is the **sole owner** of its backing storage — a
reference-count check on the internal handle (`defined() && refcount == 1`), not a check
about whether the catalog's *data* is free of duplicate values. `Catalog` shares storage
cheaply on copy; `unique()` tells you whether that sharing is currently happening.

```{important} This is not about duplicate rows or values
Despite the name, `unique()` has nothing to do with [`distinct()`](distinct.md) or
duplicate detection on the catalog's contents. It answers a single question: "is any other
`Catalog` handle currently pointing at the same backing storage as this one?" Two
independently built catalogs with identical stream contents are each `unique()` on their
own — copying is what breaks uniqueness, not data collisions. See Example 2.
```

## Parameters

`None`

## Returns

`bool`
    `true` if the catalog is defined and no other `Catalog` handle currently shares its
    storage; `false` if it's undefined, or if a copy is currently sharing storage with it.

## Examples

Example 1. A freshly built catalog with no copies is unique.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
});

std::cout << std::boolalpha << ct.unique();
```

### Output

```text
true
```

Example 2. Copying a catalog makes **both** the original and the copy report
`unique() == false`, since they now share the same backing storage.

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
});

auto copy = ct;
std::cout << std::boolalpha << ct.unique() << "\n";
std::cout << std::boolalpha << copy.unique();
```

### Output

```text
false
false
```

Example 3. An undefined catalog is never unique.

```cpp
pluss::kindle::Catalog ct;
std::cout << std::boolalpha << ct.unique();
```

### Output

```text
false
```

## Notes

- `noexcept`: safe to call on any catalog, including an undefined one — an undefined
  catalog always returns `false`.
- Purely a reference-count check on the internal handle — unrelated to whether the
  catalog's stream contents contain duplicate values. See the important callout above.
- Copying a `Catalog` (copy constructor or copy-assignment) is what breaks uniqueness;
  every purely functional transform method returns freshly built storage instead.

## See Also

- [`Catalog::defined`](defined.md): whether the catalog has *any* backing storage at all,
  independent of sharing.
- [`Catalog::distinct`](distinct.md): the actual duplicate-value-detection operation, not
  to be confused with this method's storage-sharing check.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="transform.html">
    <span class="label">Previous</span>
    <span class="title">transform</span>
  </a>
  <a class="next-link" href="where.html">
    <span class="label">Next</span>
    <span class="title">where</span>
  </a>
</div>
