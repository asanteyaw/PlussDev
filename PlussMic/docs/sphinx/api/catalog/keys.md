<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 376561c1a32a -->

# Catalog::keys

[Back to Catalog API Reference](../../catalog_api_reference)

## Signatures

```cpp
Catalog::keys() const -> const std::vector<std::string>&
Catalog::keys(std::string_view key) const -> CatalogKeyAccessor
Catalog::keys(std::span<const std::string_view> field_names) const -> CatalogKeyAccessor
Catalog::keys(std::initializer_list<std::string_view> field_names) const -> CatalogKeyAccessor
```

## What It Does

Two genuinely different operations share this one overloaded name:

- **`keys()`** (no arguments) returns the catalog's stream names as a plain
  `const std::vector<std::string>&` — a read-only view, in the catalog's own key order.
- **`keys(key)`** / **`keys(field_names)`** (one or more names) selects a subset of streams
  and returns a `CatalogKeyAccessor` — an entry point into a small family of operations
  (`fillnull`, `dropnull`, `ffill`, `bfill`, `isnull`, `notnull`, `invalid`, `distinct`,
  `freq`, `ndistinct`, `astype`, plus `cast()`/`nt()`/`tl()`/`ts()` sub-accessors) applied
  to just the selected streams, returning a new `Catalog` scoped to that selection.

```{important} keys(names) is a selection, not a rename or a projection to a smaller catalog
`keys("score").fillnull(0.0)` operates on the `score` stream *within the full catalog
context* and returns a `Catalog` containing only the selected streams, transformed. If you
want to keep every stream but only transform some, use the transform on the full catalog
directly, or [`set()`](set.md) the transformed stream back in afterward.
```

## Parameters

**key** (`std::string_view`)
    A single stream name to select.

**field_names** (`std::span<const std::string_view>` or
`std::initializer_list<std::string_view>`)
    Several stream names to select at once.

## Returns

`const std::vector<std::string>&` (no-argument overload)
    A read-only reference to the catalog's stream names, in key order.

`CatalogKeyAccessor` (name-argument overloads)
    An accessor scoped to the selected stream(s) — call one of its methods to actually
    transform them.

## Examples

Example 1. The no-argument overload returns every stream name, in order.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k   = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

for (const auto& name : ct.keys()) {
  std::cout << name << " ";
}
```

### Output

```text
id metric city 
```

Example 2. `keys(key)` selects one stream and returns an accessor — chaining
[`fillnull()`](fillnull.md) through it fills `NULL`s only in that stream, and the result
is a `Catalog` (still containing exactly the streams that were selected).

```cpp
auto ct = pluss::kindle::Catalog({
    "score"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}),
        pluss::make_series(std::vector<bool>{true, false, true})),
});

auto out = ct.keys("score").fillnull(0.0);
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [score]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
score                    | Series       | Series[float64]  | 3       | [1, 0, 3]                                        
========================================================================================================================
```

Example 3. `keys({...})` with an `initializer_list` selects several streams at once —
here, `distinct()` is applied to the selection.

```cpp
auto ct = pluss::kindle::Catalog({
    "score"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}),
        pluss::make_series(std::vector<bool>{true, false, true})),
});

auto out = ct.keys({"score"}).distinct();
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [score]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
score                    | Series       | Series[float64]  | 2       | [1, 3]                                           
========================================================================================================================
```

## Notes

- The no-argument overload returns a reference into the catalog's own storage — no copy.
- The name-argument overloads return a `CatalogKeyAccessor`, an entry point to a small
  per-stream transformation family (`fillnull`, `dropnull`, `ffill`, `bfill`, `isnull`,
  `notnull`, `invalid`, `distinct`, `freq`, `ndistinct`, `astype`, plus `cast`/`nt`/`tl`/`ts`
  sub-accessors) — not a rename or an arbitrary projection.
- Selecting an unknown key name throws — see [`has()`](has.md) to check first.

## See Also

- [`Catalog::has`](has.md): check whether a key exists before selecting it.
- [`Catalog::at`](at.md) / [`Catalog::get`](get.md): fetch a single stream directly as a
  `Stream`, rather than entering the `CatalogKeyAccessor` transformation family.
- [`Catalog::select`](select.md): choose a subset of streams to keep in a new `Catalog`,
  without any accessor transformation attached.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="keyaxis.html">
    <span class="label">Previous</span>
    <span class="title">keyaxis</span>
  </a>
  <a class="next-link" href="kindelize.html">
    <span class="label">Next</span>
    <span class="title">kindelize</span>
  </a>
</div>
