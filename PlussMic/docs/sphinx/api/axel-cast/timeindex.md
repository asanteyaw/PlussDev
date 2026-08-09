<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 744b375e0c39 -->

# AxelCastAccessor::timeindex

[Back to AxelCastAccessor API Reference](../../axel_cast_accessor_api_reference)

## Signature

```cpp
Axel AxelCastAccessor::timeindex() const
```

## What It Does

Casts every bound field's values, in every registry frame, to
`TimeIndex`-kind and returns a new axel with those fields replaced.
Applies [`RegistryCastAccessor::timeindex`](../registry-cast/timeindex.md)
to every frame via [`Axel::map`](../axel/map.md).

## Parameters

`None`

## Returns

`Axel`
    A new axel with every bound field, in every frame, cast to
    `TimeIndex`-kind; every unbound field unchanged.

## Examples

Example 1. Cast a `Timeline` field to `TimeIndex`.

```cpp
auto r0 = pluss::bundle::Registry({{"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-01T00:00:00Z")})}});
auto axel = pluss::bundle::Axel({r0});

auto out = axel.fields({"ts"}).cast().timeindex();
std::cout << out.registry(0).fields("ts").to_json();
```

### Output

```text
{
  "_type": "Slot",
  "defined": true,
  "size": 1,
  "values": [
    "TimeIndex[size=1]"
  ]
}
```

Each row's `Timeline` value converts to its own single-element
`TimeIndex` — a `Slot` stores one independent value per row rather than
one shared column, mirroring
[`Slot::tl().index()`](../slot-timeline/index.md)'s per-row behavior.

## Notes

- Bound via [`Axel::cast()`](../axel/cast.md) — remember that entry
  point binds *every* field of every frame it's called on, so scope it
  with [`fields(keys)`](../axel/fields.md) first if you only want to
  cast some fields.
- Whatever `Slot::cast()` throws for a non-`Timeline`-compatible source
  value propagates the same way here.

## See Also

- [`AxelCastAccessor::timeline`](timeline.md): the related cast target
  for calendar timestamps without the sortedness requirement.
- [`Axel::cast`](../axel/cast.md): the entry point this accessor is
  reached from.
- [`RegistryCastAccessor::timeindex`](../registry-cast/timeindex.md):
  the single-frame operation this method broadcasts.

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
