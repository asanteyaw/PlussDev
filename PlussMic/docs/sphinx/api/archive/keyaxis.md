<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8f932e53f84b -->

# Archive::keyaxis

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive::keyaxis() const -> ArchiveKeyAxisAccessor
```

## What It Does

The entry point into operations on the **key axis** — the stream names within each
catalog, applied uniformly across every catalog in the archive. `keyaxis()` binds the
current archive and hands back an `ArchiveKeyAxisAccessor`:

### `ArchiveKeyAxisAccessor` members

| Member | Kind or type | Signature or default |
|---|---|---|
| `like` | Method | `Archive like(std::string_view needle, bool case_sensitive = true) const` |
| `match` | Method | `Archive match(std::string_view pattern) const` |
| `series` | Method | `Archive series() const` |
| `note` | Method | `Archive note() const` |
| `timeline` | Method | `Archive timeline() const` |
| `timeindex` | Method | `Archive timeindex() const` |
| `prefix` | Method | `Archive prefix(std::string_view prefix) const` |
| `suffix` | Method | `Archive suffix(std::string_view suffix) const` |

All eight methods are **filters** on the key axis, identical in behavior to
[`Catalog::keyaxis`](../catalog/keyaxis.md) — just applied to every catalog in the archive
at once instead of a single catalog:

- **`like()`/`match()`** filter by key text.
- **`series()`/`note()`/`timeline()`/`timeindex()`** filter by stream payload kind.
- **`prefix()`/`suffix()`** filter by whether a key starts/ends with the requested token.

```{important} prefix()/suffix() are key filters, not renames
Same as on `Catalog`: `prefix("user_")` means "keep only keys starting with `user_`" in
every catalog — it does **not** rename keys.
```

## Parameters

`None` (parameters belong to the individual `ArchiveKeyAxisAccessor` methods, not to
`keyaxis()` itself).

## Returns

`ArchiveKeyAxisAccessor`
    A binder for the current archive — call one of its eight methods to filter every
    catalog's keys by text or stream kind.

## Examples

Example 1. `like()` filters keys by substring, applied to every catalog.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "user_id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})),
    "user_name"_k   = pluss::kindle::Stream(pluss::make_note({"Ada", "Bola"})),
    "signup_date"_k = pluss::kindle::Stream(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-02-01T00:00:00Z"})),
});
auto c1 = pluss::kindle::Catalog({
    "user_id"_k     = pluss::kindle::Stream(pluss::make_series({3.0, 4.0})),
    "user_name"_k   = pluss::kindle::Stream(pluss::make_note({"Chen", "Dee"})),
    "signup_date"_k = pluss::kindle::Stream(pluss::make_timeline({
        "2024-03-01T00:00:00Z", "2024-04-01T00:00:00Z"})),
});
auto archive = pluss::kindle::Archive({c0, c1});

auto out = archive.keyaxis().like("user");
std::cout << out.catalog(0);
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [user_id, user_name]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
user_id                  | Series       | Series[float64]  | 2       | [1, 2]                                           
user_name                | Note         | Note             | 2       | [Ada, Bola]                                      
========================================================================================================================
```

Example 2. Filter to only `Note`-kind streams in every catalog with `note()`.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "user_id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})),
    "user_name"_k   = pluss::kindle::Stream(pluss::make_note({"Ada", "Bola"})),
    "signup_date"_k = pluss::kindle::Stream(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-02-01T00:00:00Z"})),
});
auto archive = pluss::kindle::Archive({c0});

auto out = archive.keyaxis().note();
std::cout << out.catalog(0);
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [user_name]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
user_name                | Note         | Note             | 2       | [Ada, Bola]                                      
========================================================================================================================
```

## Notes

- All eight methods filter down to a subset of streams *within each catalog*; none of them
  rename keys.
- Applied uniformly across every catalog in the archive — if catalogs have different keys,
  each catalog's own filtered result reflects only the keys it actually has.
- `like()` defaults to case-sensitive matching; pass `false` for a case-insensitive
  substring search. `match()` takes a full regular expression.

## See Also

- [`Archive::keys`](keys.md): select streams by explicit name list, rather than by
  substring/regex/kind match.
- [`Catalog::keyaxis`](../catalog/keyaxis.md): the single-catalog equivalent this method
  applies uniformly across every catalog in the archive.

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
