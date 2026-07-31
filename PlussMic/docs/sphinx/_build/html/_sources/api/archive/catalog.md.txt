<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3fc8b8551733 -->

# Archive::catalog

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive::catalog(std::int64_t i) const -> Catalog
```

## What It Does

Returns the catalog at depth-index `i`. Supports Python-style negative indexing (`-1` is
the last catalog). An exact synonym for [`dloc()`](dloc.md).

## Parameters

**i** (`std::int64_t`)
    The depth-index to fetch. Negative values count from the end.

## Returns

`Catalog`
    The catalog at depth-index `i`.

## Examples

Example 1. Fetch the first catalog by index.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})) });
auto c1 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({3.0, 4.0})) });
auto archive = pluss::kindle::Archive({c0, c1});

std::cout << archive.catalog(0);
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [id]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 2       | [1, 2]                                           
========================================================================================================================
```

Example 2. Negative indices count from the end — `-1` is the last catalog.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({1.0})) });
auto c1 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({2.0})) });
auto archive = pluss::kindle::Archive({c0, c1});

std::cout << archive.catalog(-1);
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [id]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 1       | [2]                                              
========================================================================================================================
```

Example 3. An index outside `[-depth(), depth())` throws.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({1.0})) });
auto archive = pluss::kindle::Archive({c0});

try {
  archive.catalog(5);
} catch (const std::exception& e) {
  std::cout << std::string_view(e.what()).substr(0, 28);
}
```

### Output

```text
Archive: index out of bounds
```

## Notes

- Supports negative indexing (`-1`, `-2`, ...), counting from the end — see Example 2.
- Out-of-range (after negative-index normalization) throws `"Archive: index out of
  bounds"`.
- Requires a defined archive; throws `"Archive: undefined handle"` on a default-constructed
  `Archive()`.
- The thrown exception's `.what()` appends a native stack trace after the message text
  shown above; examples here show only the leading message for readability.

## See Also

- [`Archive::dloc`](dloc.md): the exact synonym for this method.
- [`Archive::loc`](loc.md): fetch a catalog by depth *label* instead of positional index.
- [`Archive::catalogs`](catalogs.md): every catalog at once, as a `const&` to the whole
  vector.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cast.html">
    <span class="label">Previous</span>
    <span class="title">cast</span>
  </a>
  <a class="next-link" href="catalogs.html">
    <span class="label">Next</span>
    <span class="title">catalogs</span>
  </a>
</div>
