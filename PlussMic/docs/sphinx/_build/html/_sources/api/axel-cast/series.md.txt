<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1e1cd1faf5b2 -->

# AxelCastAccessor::series

[Back to AxelCastAccessor API Reference](../../axel_cast_accessor_api_reference)

## Signature

```cpp
Axel AxelCastAccessor::series() const
```

## What It Does

Casts every bound field's values, in every registry frame, to
`Series`-kind and returns a new axel with those fields replaced.
Applies [`RegistryCastAccessor::series`](../registry-cast/series.md) to
every frame via [`Axel::map`](../axel/map.md).

```{important} Unparseable text becomes null, not a throw
Casting a `Note` value that can't be parsed as numeric (e.g. `"Accra"`)
doesn't throw — it silently becomes a null value (`std::monostate`) in
the resulting field. Check [`Axel::isnull`](../axel/isnull.md) after
casting if you need to detect values that failed to convert.
```

## Parameters

`None`

## Returns

`Axel`
    A new axel with every bound field, in every frame, cast to
    `Series`-kind; every unbound field unchanged.

## Examples

Example 1. Cast a `Note` field of numeric-looking text to `Series`.

```cpp
auto r0 = pluss::bundle::Registry({{"id", pluss::bundle::Slot({pluss::make_note("1"), pluss::make_note("2")})}});
auto axel = pluss::bundle::Axel({r0});

auto out = axel.fields({"id"}).cast().series();
auto ids = out.registry(0).fields("id");
std::cout << std::get<pluss::Series>(ids.at(0)).to_vec<std::int64_t>()[0] << " "
          << std::get<pluss::Series>(ids.at(1)).to_vec<std::int64_t>()[0];
```

### Output

```text
1 2
```

Example 2. Unparseable text becomes null rather than throwing — see the
important callout above.

```cpp
auto r0 = pluss::bundle::Registry({{"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})}});
auto axel = pluss::bundle::Axel({r0});

auto out = axel.fields({"city"}).cast().series();
std::cout << out.registry(0).fields("city").to_json();
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

- Bound via [`Axel::cast()`](../axel/cast.md) — remember that entry
  point binds *every* field of every frame it's called on, so scope it
  with [`fields(keys)`](../axel/fields.md) first if you only want to
  cast some fields.
- See the important callout above for the silent-null behavior on
  unparseable text.

## See Also

- [`AxelCastAccessor::note`](note.md): the inverse direction, casting
  numeric fields to text.
- [`Axel::cast`](../axel/cast.md): the entry point this accessor is
  reached from.
- [`RegistryCastAccessor::series`](../registry-cast/series.md): the
  single-frame operation this method broadcasts.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="note.html">
    <span class="label">Previous</span>
    <span class="title">note</span>
  </a>
  <a class="next-link" href="timeindex.html">
    <span class="label">Next</span>
    <span class="title">timeindex</span>
  </a>
</div>
