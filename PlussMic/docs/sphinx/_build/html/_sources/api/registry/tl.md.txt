<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e5feb428e0ec -->

# Registry::tl

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
RegistryTimelineAccessor Registry::tl() const
```

## What It Does

The entry point into **datetime (`Timeline`/`TimeIndex`) operations** — `tl()`
binds every field of the current registry and hands back a
`RegistryTimelineAccessor`, documented in full on its own pages (see
[`RegistryTimelineAccessor` API Reference](../../registry_timeline_accessor_api_reference)):
calendar field extraction (`year()`, `month()`, `weekday()`, ...), formatting
(`fmt()`, `iso()`), rounding (`floor()`, `ceil()`, `round()`), timezone handling
(`tz()`, `localize()`, `utc()`), and more.

```{important} tl() always binds every field — scope it by projecting first
Same pattern as [`cast()`](cast.md) and [`nt()`](nt.md): `tl()` takes no `key`
argument and always operates on every field. To scope datetime operations to one
field, project first: `rg.fields({"created_at"}).tl().year()`.
```

## Parameters

`None`

## Returns

`RegistryTimelineAccessor`
    A binder for the current registry's fields — call one of its methods to run
    a datetime operation across every bound field.

## Examples

Example 1. Extract the calendar year from a projected `Timeline` field.

```cpp
auto rg = pluss::scribble::Registry({
    {"created_at", pluss::scribble::Slot({pluss::make_timeline("2024-03-15"), pluss::make_timeline("2023-07-04")})},
});

auto out = rg.fields({"created_at"}).tl().year();
auto years = out.fields("created_at");
std::cout << std::get<pluss::Series>(years.at(0)).host<std::int64_t>()[0] << " ";
std::cout << std::get<pluss::Series>(years.at(1)).host<std::int64_t>()[0];
```

### Output

```text
2024 2023
```

## Notes

- `tl()` alone does no work — it only binds the registry's fields; call one of
  the accessor's methods to actually operate on datetime values.
- See the important callout above for scoping to specific fields.
- The full accessor surface, including verified examples, is documented
  separately — see
  [`RegistryTimelineAccessor` API Reference](../../registry_timeline_accessor_api_reference).

## See Also

- [`Registry::fields`](fields.md): project down to specific fields before
  running datetime operations, per the important callout above.
- [`Registry::cast`](cast.md) / [`Registry::nt`](nt.md): the sibling entry
  points for type casting and text operations.
- [`Catalog::tl`](../catalog/tl.md): the equivalent entry point on the kindle
  model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="take_.html">
    <span class="label">Previous</span>
    <span class="title">take_</span>
  </a>
  <a class="next-link" href="to_json.html">
    <span class="label">Next</span>
    <span class="title">to_json</span>
  </a>
</div>
