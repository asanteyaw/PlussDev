<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 16cb0bc0a5d8 -->

# RegistryCastAccessor::timeindex

[Back to RegistryCastAccessor API Reference](../../registry_cast_accessor_api_reference)

## Signature

```cpp
Registry RegistryCastAccessor::timeindex() const
```

## What It Does

Casts every bound field's values to `TimeIndex`-kind and returns a new
registry with those fields replaced. Behaves the same as
[`timeline()`](timeline.md), including throwing (rather than nulling) on
unparseable text.

## Parameters

`None`

## Returns

`Registry`
    A new registry with every bound field cast to `TimeIndex`-kind; every
    unbound field unchanged.

## Examples

Example 1. Cast a `Note` field of date-looking text to `TimeIndex`.

```cpp
auto rg = pluss::scribble::Registry({
    {"created_at", pluss::scribble::Slot({pluss::make_note("2024-01-01"), pluss::make_note("2024-01-02")})},
});

auto out = rg.fields({"created_at"}).cast().timeindex();
std::cout << out.fields("created_at").to_json();
```

### Output

```text
{
  "_type": "Slot",
  "defined": true,
  "size": 2,
  "values": [
    "TimeIndex[size=1]",
    "TimeIndex[size=1]"
  ]
}
```

Example 2. Text that doesn't parse as a datetime throws.

```cpp
auto rg = pluss::scribble::Registry({
    {"city", pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})},
});

try {
  auto out = rg.fields({"city"}).cast().timeindex();
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
- `TimeIndex`'s `to_json()` type tag reports `"TimeIndex[size=N]"`, distinct
  from `Timeline`'s `"Timeline[numel=N]"` — a small naming difference between
  the two datetime kinds.

## See Also

- [`RegistryCastAccessor::timeline`](timeline.md): the related cast target,
  with the same parse-failure throwing behavior.
- [`RegistryCastAccessor::series`](series.md): the numeric cast target, with a
  different (silent-null) failure mode for unparseable input.
- [`Registry::cast`](../registry/cast.md): the entry point this accessor is
  reached from.

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
