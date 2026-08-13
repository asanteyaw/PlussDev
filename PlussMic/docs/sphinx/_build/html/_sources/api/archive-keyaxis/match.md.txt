<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8b6bd3923a46 -->

# ArchiveKeyAxisAccessor::match

[Back to ArchiveKeyAxisAccessor API Reference](../../archive_keyaxis_accessor_api_reference)

## Signature

```cpp
Archive ArchiveKeyAxisAccessor::match(std::string_view pattern) const
```

## What It Does

Returns a new archive where **every catalog** in the stack has been narrowed to only
the keys whose name matches the ECMAScript regular expression `pattern` — each catalog
filtered independently, by delegating to that catalog's own
[`CatalogKeyAxisAccessor::match`](../catalog-keyaxis/match.md).

```{important} Each catalog is filtered independently — no cross-catalog key uniformity required
Same rule as [`like()`](like.md): `match()` never validates keys across catalogs. Each
catalog runs its own independent regex match against its own key names, and a ragged
archive works without throwing — the result can be just as ragged, with different
catalogs matching different numbers (or sets) of keys.
```

## Parameters

**pattern** (`std::string_view`)
    An ECMAScript-flavored regular expression, matched against each key name using
    "search" (not "full match") semantics — anchor with `^`/`$` for whole-name matching.

## Returns

`Archive`
    A new archive of the same depth, with each catalog narrowed to its own
    regex-matching keys.

## Examples

Example 1. `^id[0-9]$` matches exactly one key out of three — anchored, whole-name
matching via explicit `^`/`$`.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "id1"_k  = pluss::kindle::Stream(pluss::make_series({1.0})),
    "id22"_k = pluss::kindle::Stream(pluss::make_series({2.0})),
    "name"_k = pluss::kindle::Stream(pluss::make_note({"x"})),
});
auto archive = pluss::kindle::Archive({c0});

auto out = archive.keyaxis().match("^id[0-9]$");
std::cout << out.catalog(0);
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [id1]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id1                      | Series       | Series[float64]  | 1       | [1]                                              
========================================================================================================================
```

`"id22"` fails the `[0-9]$` single-digit anchor and `"name"` doesn't match `id` at all —
only `"id1"` survives.

## Notes

- See the important callout above.
- Unlike [`like()`](like.md), there is no `case_sensitive` parameter — encode case
  sensitivity in the pattern itself (e.g. `(?i)` is not supported by ECMAScript regex;
  use an explicit character class like `[iI][dD]` instead).
- A catalog with zero matches becomes a valid, defined, empty catalog in the result.

## See Also

- [`ArchiveKeyAxisAccessor::like`](like.md): the plain-substring counterpart, for
  simpler searches that don't need regex.
- [`CatalogKeyAxisAccessor::match`](../catalog-keyaxis/match.md): the single-catalog
  delegate this method applies to every catalog independently.
- [`Archive::keyaxis`](../archive/keyaxis.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="like.html">
    <span class="label">Previous</span>
    <span class="title">like</span>
  </a>
  <a class="next-link" href="note.html">
    <span class="label">Next</span>
    <span class="title">note</span>
  </a>
</div>
