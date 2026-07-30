<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0aee4dafc582 -->

# Catalog::cast

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::cast(pluss::table::ThreadCastTarget target, const pluss::table::ThreadCastOptions& options = {}) const -> Catalog
```

## What It Does

Casts **every** stream in the catalog to the same target payload kind, in one call — each
stream's own `Stream::cast(target, options)`, applied uniformly regardless of that
stream's current kind. This is the whole-catalog counterpart to
[`Thread::cast()`](../thread/cast.md); see that page for the full explanation of what each
`ThreadCastTarget` means and how `ThreadCastOptions` (dtype selection, parse-failure
handling) affects the conversion.

```{important} Applies to every stream uniformly — not just the ones that need it
`cast()` doesn't skip streams that are already the target kind (that conversion is a cheap
identity) or streams for which the target doesn't make obvious sense — it calls `cast()`
on all of them. If you only want to cast specific streams, use
[`Catalog::keys(names)`](keys.md)`.cast(options).series()` (or `.note()`/`.timeline()`/
`.timeindex()`) instead — that path returns a `CatalogCastAccessor` scoped to your
selection, with the target picked by which method you call rather than a `target`
argument.
```

## Parameters

**target** (`pluss::table::ThreadCastTarget`)
    The payload kind to cast every stream to — `Series`, `Note`, `Timeline`, or
    `TimeIndex`.

**options** (`const pluss::table::ThreadCastOptions&`, optional)
    Conversion policy — `series_dtype` picks the numeric dtype for a `Series` target;
    `invalid` chooses parse-failure behavior. See [`Thread::cast`](../thread/cast.md) for
    the full breakdown.

## Returns

`Catalog`
    A new catalog with every stream cast to `target`.

## Examples

Example 1. Cast every `Note`-kind stream of numeric-looking text to `Series`.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k    = pluss::kindle::Stream(pluss::make_note({"1", "2", "3"})),
    "score"_k = pluss::kindle::Stream(pluss::make_note({"1.5", "2.5", "3.5"})),
});

auto out = ct.cast(pluss::table::ThreadCastTarget::Series);
std::cout << out;
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
score                    | Series       | Series[float64]  | 3       | [1.5, 2.5, 3.5]                                  
========================================================================================================================
```

Example 2. Parse ISO-formatted text into `Timeline`.

```cpp
auto ct = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_note({
        "2024-01-01T00:00:00Z", "2024-02-01T00:00:00Z"})),
});

auto out = ct.cast(pluss::table::ThreadCastTarget::Timeline);
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [ts]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
ts                       | Timeline     | Timeline         | 2       | [2024-01-01T00:00:00.000~, 2024-02-01T00:00:00.0~
========================================================================================================================
```

Example 3. Request a specific numeric dtype via `ThreadCastOptions`.

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_note({"1", "2", "3"})),
});

pluss::table::ThreadCastOptions options;
options.series_dtype = pluss::codem::ScalarType::Int64;

auto out = ct.cast(pluss::table::ThreadCastTarget::Series, options);
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [id]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[int64]    | 3       | [1, 2, 3]                                        
========================================================================================================================
```

## Notes

- Applies to every stream, unconditionally — see the important callout above.
- Requires a defined catalog; throws `"Catalog: undefined handle"` otherwise.
- Follows the same per-target conversion rules as [`Thread::cast`](../thread/cast.md),
  including its documented quirk that `ThreadCastOptions::invalid` only actually takes
  effect for the `Timeline` target.

## See Also

- [`Catalog::astype`](astype.md): the shorthand for `cast(Series, {.series_dtype = dtype})`.
- [`Thread::cast`](../thread/cast.md): the underlying single-stream conversion this method
  applies uniformly.
- [`Catalog::keys`](keys.md): scope a cast to specific streams instead of the whole
  catalog, via its own `cast()` accessor.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bfill.html">
    <span class="label">Previous</span>
    <span class="title">bfill</span>
  </a>
  <a class="next-link" href="cloc.html">
    <span class="label">Next</span>
    <span class="title">cloc</span>
  </a>
</div>
