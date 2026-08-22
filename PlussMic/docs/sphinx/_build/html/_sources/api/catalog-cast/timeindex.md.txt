<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 11ef84e808cf -->

# CatalogCastAccessor::timeindex

[Back to CatalogCastAccessor API Reference](../../catalog_cast_accessor_api_reference)

## Signature

```cpp
Catalog CatalogCastAccessor::timeindex() const
```

## What It Does

Casts every bound key's values to `TimeIndex`-kind and returns a new catalog
with those keys replaced. Behaves the same as [`timeline()`](timeline.md),
including throwing (rather than nulling) on unparseable text.

## Parameters

`None`

## Returns

`Catalog`
    A new catalog with every bound key cast to `TimeIndex`-kind.

## Examples

Example 1. Cast a `Note` key of date-looking text to `TimeIndex`.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "created_at"_k = pluss::kindle::Stream(pluss::make_note({"2024-01-01", "2024-01-02"})),
});
auto out = ct.cast().timeindex();
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [created_at]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
created_at               | TimeIndex    | TimeIndex        | 2       | [2024-01-01T00:00:00.000~, 2024-01-02T00:00:00.0~
========================================================================================================================
```

Example 2 (edge case). Text that doesn't parse as a datetime throws.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema"})),
});
try {
  auto out = ct.cast().timeindex();
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
moment: failed to parse datetime
```

## Notes

- Bound via [`Catalog::cast()`](../catalog/cast.md) — remember that entry
  point applies to *every* key of the catalog it's called on, so scope it with
  [`Catalog::keys(names)`](../catalog/keys.md) first if you only want to cast
  some keys.
- `TimeIndex` and `Timeline` are both rendered with the same
  `"YYYY-MM-DDTHH:MM:SS.fff~"`-style preview in the table dump — check the
  `kind`/`dtype` columns to tell them apart.

## See Also

- [`CatalogCastAccessor::timeline`](timeline.md): the related cast target,
  with the same parse-failure throwing behavior.
- [`CatalogCastAccessor::series`](series.md): the numeric cast target, with a
  different (silent-null) failure mode for unparseable input.
- [`Catalog::cast`](../catalog/cast.md): the entry point this accessor is
  reached from.
- [`RegistryCastAccessor::timeindex`](../registry-cast/timeindex.md): the
  scribble-model equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="series.html">
    <span class="label">Previous</span>
    <span class="title">series</span>
  </a>
  <a class="next-link" href="timeline.html">
    <span class="label">Next</span>
    <span class="title">timeline</span>
  </a>
</div>
