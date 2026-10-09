<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2e173f8feae0 -->

# Archive::cast

[Back to Archive API Reference](../../archive_api_reference)

## Signatures

```cpp
Archive::cast() const -> ArchiveCastAccessor
Archive::cast(const pluss::table::SlateCastOptions& options) const -> ArchiveCastAccessor
Archive::cast(pluss::table::SlateCastTarget target, const pluss::table::SlateCastOptions& options = {}) const -> Archive
```

## What It Does

Archive casting follows the same two-surface pattern as catalog casting:

- accessor-first: `archive.cast().series()` / `.note()` / `.timeline()` /
  `.timeindex()`
- direct root overload: `archive.cast(target, options)`

Both cast **every** stream in **every** catalog to the same target payload kind by
delegating to each catalog's own [`Catalog::cast()`](../catalog/cast.md), applied
uniformly regardless of each stream's current kind.

```{important} Preferred surface
Prefer the accessor form in new code: `archive.cast().timeline()` reads better than
passing an explicit target enum at the root. The direct root overload still exists and
returns the same result.
```

```{important} Applies to every stream in every catalog uniformly
Same behavior as [`Catalog::cast()`](../catalog/cast.md), just archive-wide: `cast()`
doesn't skip streams that are already the target kind, or streams for which the target
doesn't make obvious sense — it calls `cast()` on all of them, in every catalog. If you
only want to cast specific streams, use [`Archive::keys(names)`](keys.md) followed by
`.cast()` on the selection instead.
```

## Parameters

**options** (`const pluss::table::SlateCastOptions&`, optional)
    For the accessor overloads, stores conversion policy on the accessor before you pick
    the terminal target.

**target** (`pluss::table::SlateCastTarget`)
    For the direct root overload, the payload kind to cast every stream to — `Series`,
    `Note`, `Timeline`, or `TimeIndex`.

**options** (`const pluss::table::SlateCastOptions&`, optional)
    For the direct root overload, conversion policy — `series_dtype` picks the numeric
    dtype for a `Series` target; `invalid` chooses parse-failure behavior. See
    [`Slate::cast`](../slate/cast.md) for the full breakdown.

## Returns

`ArchiveCastAccessor` / `Archive`
    The no-target overloads return an accessor; the target overload returns the fully
    cast archive directly.

## Examples

Example 1. Cast every `Note`-kind stream of numeric-looking text to `Series`, across every
catalog.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "id"_k    = pluss::kindle::Stream(pluss::make_note({"1", "2", "3"})),
    "score"_k = pluss::kindle::Stream(pluss::make_note({"1.5", "2.5"})),
});
auto archive = pluss::kindle::Archive({c0});

auto out = archive.cast().series();
std::cout << out.catalog(0);
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [id, score]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 3       | [1, 2, 3]                                        
score                    | Series       | Series[float64]  | 2       | [1.5, 2.5]                                       
========================================================================================================================
```

## Notes

- Applies to every stream in every catalog, unconditionally — see the important callout
  above.
- `Archive::cast(target, options)` and `Archive::cast(options).series()` are equivalent
  in result; they differ only in surface style.
- Requires a defined archive; throws `"Archive: undefined handle"` otherwise.
- Follows the same per-target conversion rules as
  [`Catalog::cast`](../catalog/cast.md)/[`Slate::cast`](../slate/cast.md).

## See Also

- [`Archive::astype`](astype.md): the shorthand for `cast(Series, {.series_dtype = dtype})`.
- [`Catalog::cast`](../catalog/cast.md): the single-catalog delegate this method applies to
  every catalog.
- [`Archive::keys`](keys.md): scope a cast to specific streams instead of the whole archive.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bfill.html">
    <span class="label">Previous</span>
    <span class="title">bfill</span>
  </a>
  <a class="next-link" href="catalog.html">
    <span class="label">Next</span>
    <span class="title">catalog</span>
  </a>
</div>
