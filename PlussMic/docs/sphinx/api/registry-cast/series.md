<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: add01b15668a -->

# RegistryCastAccessor::series

[Back to RegistryCastAccessor API Reference](../../registry_cast_accessor_api_reference)

## Signature

```cpp
Registry RegistryCastAccessor::series() const
```

## What It Does

Casts every bound field's values to `Series`-kind and returns a new registry
with those fields replaced.

```{important} Unparseable text becomes null, not a throw
Casting a `Note` value that can't be parsed as numeric (e.g. `"Accra"`) doesn't
throw — it silently becomes a null value (`std::monostate`) in the resulting
field. Check [`Registry::isnull`](../registry/isnull.md) after casting if you
need to detect values that failed to convert.
```

## Parameters

`None`

## Returns

`Registry`
    A new registry with every bound field cast to `Series`-kind; every
    unbound field unchanged.

## Examples

Example 1. Cast a `Note` field of numeric-looking text to `Series`.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_note("1"), pluss::make_note("2")})},
});

auto out = rg.fields({"id"}).cast().series();
auto ids = out.fields("id");
std::cout << std::get<pluss::Series>(ids.at(0)).host<std::int64_t>()[0] << " ";
std::cout << std::get<pluss::Series>(ids.at(1)).host<std::int64_t>()[0];
```

### Output

```text
1 2
```

Example 2. Unparseable text becomes null rather than throwing — see the
important callout above.

```cpp
auto rg = pluss::scribble::Registry({
    {"city", pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})},
});

auto out = rg.fields({"city"}).cast().series();
std::cout << out.fields("city").to_json();
```

### Output

```text
{
  "_type": "Slot",
  "defined": true,
  "size": 2,
  "values": [
    null,
    null
  ]
}
```

## Notes

- Bound via [`Registry::cast()`](../registry/cast.md) — remember that entry
  point binds *every* field of the registry it's called on, so scope it with
  [`fields(keys)`](../registry/fields.md) first if you only want to cast some
  fields.
- See the important callout above for the silent-null behavior on unparseable
  text.

## See Also

- [`RegistryCastAccessor::note`](note.md): the inverse direction, casting
  numeric fields to text.
- [`Registry::cast`](../registry/cast.md): the entry point this accessor is
  reached from.
- [`Slot::cast`](../slot/cast.md): the per-field casting this method applies
  across every bound field.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="note.html">
    <span class="label">Previous</span>
    <span class="title">note</span>
  </a>
</div>
