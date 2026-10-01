<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0ae330b7d3cb -->

# CatalogCastAccessor::timeline

[Back to CatalogCastAccessor API Reference](../../catalog_cast_accessor_api_reference)

## Signature

```cpp
Catalog CatalogCastAccessor::timeline() const
```

## What It Does

Casts every bound key's values to `Timeline`-kind and returns a new catalog
with those keys replaced.

```{important} Unparseable text throws — unlike casting toward Series
Casting toward `Timeline` behaves differently from [`series()`](series.md)'s
silent-null failure mode: text that doesn't parse as a datetime throws
`"moment: failed to parse datetime"` immediately, rather than producing a
null value.
```

## Parameters

`None`

## Returns

`Catalog`
    A new catalog with every bound key cast to `Timeline`-kind.

## Examples

Example 1. Cast a `Note` key of date-looking text to `Timeline`.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "created_at"_k = pluss::kindle::Stream(pluss::make_note({"2024-01-01", "2024-01-02"})),
});
auto out = ct.cast().timeline();
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
created_at               | Timeline     | Timeline         | 2       | [2024-01-01T00:00:00.000~, 2024-01-02T00:00:00.0~
========================================================================================================================
```

Example 2 (edge case). Text that doesn't parse as a datetime throws — see the
important callout above.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema"})),
});
try {
  auto out = ct.cast().timeline();
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
- Throws on parse failure — see the important callout above, and compare with
  [`series()`](series.md)'s silent-null behavior for the same kind of input.

## See Also

- [`CatalogCastAccessor::series`](series.md): the numeric cast target, with a
  different (silent-null) failure mode for unparseable input.
- [`Catalog::cast`](../catalog/cast.md): the entry point this accessor is
  reached from.
- [`RegistryCastAccessor::timeline`](../registry-cast/timeline.md): the
  scribble-model equivalent.

<div class="pluss-inline-prev-next">
</div>
