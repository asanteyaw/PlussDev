<!-- pluss-hand-authored -->

# AxelCastAccessor::timeline

[Back to AxelCastAccessor API Reference](../../axel_cast_accessor_api_reference)

## Signature

```cpp
Axel AxelCastAccessor::timeline() const
```

## What It Does

Casts every bound field's values, in every registry frame, to
`Timeline`-kind and returns a new axel with those fields replaced.
Applies [`RegistryCastAccessor::timeline`](../registry-cast/timeline.md)
to every frame via [`Axel::map`](../axel/map.md).

## Parameters

`None`

## Returns

`Axel`
    A new axel with every bound field, in every frame, cast to
    `Timeline`-kind; every unbound field unchanged.

## Examples

Example 1. Cast an ISO-8601 `Note` field to `Timeline`.

```cpp
auto r0 = pluss::bundle::Registry({{"ts", pluss::bundle::Slot({pluss::make_note("2024-01-31T09:15:30Z")})}});
auto axel = pluss::bundle::Axel({r0});

auto out = axel.fields({"ts"}).cast().timeline();
std::cout << out.registry(0).fields("ts").to_json();
```

### Output

```text
{
  "_type": "Slot",
  "defined": true,
  "size": 1,
  "values": [
    "Timeline[numel=1]"
  ]
}
```

## Notes

- Bound via [`Axel::cast()`](../axel/cast.md) — remember that entry
  point binds *every* field of every frame it's called on, so scope it
  with [`fields(keys)`](../axel/fields.md) first if you only want to
  cast some fields.
- Whatever `Slot::cast()` throws for unparseable source text propagates
  the same way here.

## See Also

- [`AxelCastAccessor::timeindex`](timeindex.md): the related cast
  target requiring sorted, index-usable values.
- [`Axel::cast`](../axel/cast.md): the entry point this accessor is
  reached from.
- [`RegistryCastAccessor::timeline`](../registry-cast/timeline.md): the
  single-frame operation this method broadcasts.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="timeindex.html">
    <span class="label">Previous</span>
    <span class="title">timeindex</span>
  </a>
</div>
