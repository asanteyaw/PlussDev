<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: edb2adc5ba27 -->

# Catalog::keyaxis

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::keyaxis() const -> CatalogKeyAxisAccessor
```

## What It Does

The entry point into operations on the **key axis** — the stream *names* themselves,
rather than their contents. `keyaxis()` binds the current catalog and hands back a
`CatalogKeyAxisAccessor`:

```cpp
class CatalogKeyAxisAccessor {
public:
  Catalog like(std::string_view needle, bool case_sensitive = true) const;
  Catalog match(std::string_view pattern) const;
  Catalog series() const;
  Catalog note() const;
  Catalog timeline() const;
  Catalog timeindex() const;
  Catalog prefix(std::string_view prefix) const;
  Catalog suffix(std::string_view suffix) const;
};
```

All eight methods are **filters** on the key axis:

- **`like()`/`match()`** filter by key text.
- **`series()`/`note()`/`timeline()`/`timeindex()`** filter by stream payload kind.
- **`prefix()`/`suffix()`** filter by whether a key starts/ends with the requested token.

```{important} prefix()/suffix() are key filters
`prefix("user_")` means "keep only keys starting with `user_`", and `suffix("_raw")`
means "keep only keys ending with `_raw`". They do **not** rename keys.
```

## Parameters

`None` (parameters belong to the individual `CatalogKeyAxisAccessor` methods, not to
`keyaxis()` itself).

## Returns

`CatalogKeyAxisAccessor`
    A binder for the current catalog — call one of its eight methods to filter by
    key text or stream kind.

## Examples

Example 1. `like()` filters keys by substring — case-sensitive by default.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "user_id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "user_name"_k   = pluss::kindle::Stream(pluss::make_note({"Ada", "Bola", "Chen"})),
    "signup_date"_k = pluss::kindle::Stream(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-02-01T00:00:00Z", "2024-03-01T00:00:00Z"})),
});

auto out = ct.keyaxis().like("user");
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [user_id, user_name]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
user_id                  | Series       | Series[float64]  | 3       | [1, 2, 3]                                        
user_name                | Note         | Note             | 3       | [Ada, Bola, Chen]                                
========================================================================================================================
```

Example 2. `prefix()` filters to keys that *start with* the requested token.

```cpp
auto ct = pluss::kindle::Catalog({
    "user_id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "user_name"_k   = pluss::kindle::Stream(pluss::make_note({"Ada", "Bola", "Chen"})),
    "signup_date"_k = pluss::kindle::Stream(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-02-01T00:00:00Z", "2024-03-01T00:00:00Z"})),
});

auto out = ct.keyaxis().prefix("user_");
std::cout << out.keys().at(0) << " " << out.keys().at(1);
```

### Output

```text
user_id user_name
```

Example 3. `suffix()` filters to keys that *end with* the requested token.

```cpp
auto ct = pluss::kindle::Catalog({
    "id_raw"_k       = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "name_raw"_k     = pluss::kindle::Stream(pluss::make_note({"Ada", "Bola", "Chen"})),
    "signup_date"_k  = pluss::kindle::Stream(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-02-01T00:00:00Z", "2024-03-01T00:00:00Z"})),
});

auto out = ct.keyaxis().suffix("_raw");
std::cout << out.keys().at(0) << " " << out.keys().at(1);
```

### Output

```text
id_raw name_raw
```

Example 4. Filter to only streams of a given payload kind with `series()`/`note()`/
`timeline()`/`timeindex()`, instead of by name.

```cpp
auto ct = pluss::kindle::Catalog({
    "user_id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "user_name"_k   = pluss::kindle::Stream(pluss::make_note({"Ada", "Bola", "Chen"})),
    "signup_date"_k = pluss::kindle::Stream(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-02-01T00:00:00Z", "2024-03-01T00:00:00Z"})),
});

auto out = ct.keyaxis().note();
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [user_name]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
user_name                | Note         | Note             | 3       | [Ada, Bola, Chen]                                
========================================================================================================================
```

Example 5. A `like()`/`match()`/`prefix()`/`suffix()` filter that matches nothing returns
an empty catalog
(zero keys) rather than throwing.

```cpp
auto ct = pluss::kindle::Catalog({
    "user_id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "user_name"_k = pluss::kindle::Stream(pluss::make_note({"Ada", "Bola", "Chen"})),
});

auto out = ct.keyaxis().like("zzz");
std::cout << out.nkeys();
```

### Output

```text
0
```

## Notes

- All eight methods filter down to a subset of streams; none of them rename keys.
- `like()` defaults to case-sensitive matching; pass `false` as the second argument for a
  case-insensitive substring search.
- `match()` takes a full regular expression rather than a plain substring.
- A filter that matches nothing returns an empty (zero-key) catalog rather than throwing.

## See Also

- [`Catalog::keys`](keys.md): select streams by explicit name list, rather than by
  substring/regex/kind match.
- [`Catalog::rename`](rename.md): rename specific streams explicitly when you actually want
  key transformation rather than selection.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isnull.html">
    <span class="label">Previous</span>
    <span class="title">isnull</span>
  </a>
  <a class="next-link" href="keys.html">
    <span class="label">Next</span>
    <span class="title">keys</span>
  </a>
</div>
