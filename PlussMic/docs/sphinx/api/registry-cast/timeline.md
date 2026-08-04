<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2969952b3cc7 -->

# RegistryCastAccessor::timeline

[Back to RegistryCastAccessor API Reference](../../registry_cast_accessor_api_reference)

## Signature

```cpp
Registry RegistryCastAccessor::timeline() const
```

## What It Does

Casts every bound field's values to `Timeline`-kind and returns a new registry
with those fields replaced.

```{important} Unparseable text throws — unlike casting toward Series
Casting toward `Timeline` behaves differently from
[`series()`](series.md)'s silent-null failure mode: text that doesn't parse as
a datetime throws `"moment: failed to parse datetime"` immediately, rather
than producing a null value.
```

## Parameters

`None`

## Returns

`Registry`
    A new registry with every bound field cast to `Timeline`-kind; every
    unbound field unchanged.

## Examples

Example 1. Cast a `Note` field of date-looking text to `Timeline`.

```cpp
auto rg = pluss::bundle::Registry({
    {"created_at", pluss::bundle::Slot({pluss::make_note("2024-01-01"), pluss::make_note("2024-01-02")})},
});

auto out = rg.fields({"created_at"}).cast().timeline();
std::cout << out.fields("created_at").to_json();
```

### Output

```text
{
  "_type": "Slot",
  "defined": true,
  "size": 2,
  "values": [
    "Timeline[numel=1]",
    "Timeline[numel=1]"
  ]
}
```

Example 2. Text that doesn't parse as a datetime throws — see the important
callout above.

```cpp
auto rg = pluss::bundle::Registry({
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})},
});

try {
  auto out = rg.fields({"city"}).cast().timeline();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
moment: failed to parse datetime
```

## Notes

- Bound via [`Registry::cast()`](../registry/cast.md) — remember that entry
  point binds *every* field of the registry it's called on, so scope it with
  [`fields(keys)`](../registry/fields.md) first if you only want to cast some
  fields.
- Throws on parse failure — see the important callout above, and compare with
  [`series()`](series.md)'s silent-null behavior for the same kind of input.

## See Also

- [`RegistryCastAccessor::timeindex`](timeindex.md): the related cast target,
  with the same parse-failure throwing behavior.
- [`RegistryCastAccessor::series`](series.md): the numeric cast target, with a
  different (silent-null) failure mode for unparseable input.
- [`Registry::cast`](../registry/cast.md): the entry point this accessor is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="timeindex.html">
    <span class="label">Previous</span>
    <span class="title">timeindex</span>
  </a>
</div>
