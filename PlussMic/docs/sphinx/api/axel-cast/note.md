<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 81f63cfd89ab -->

# AxelCastAccessor::note

[Back to AxelCastAccessor API Reference](../../axel_cast_accessor_api_reference)

## Signature

```cpp
Axel AxelCastAccessor::note() const
```

## What It Does

Casts every bound field's values, in every registry frame, to
`Note`-kind (text) and returns a new axel with those fields replaced.
Applies [`RegistryCastAccessor::note`](../registry-cast/note.md) to
every frame via [`Axel::map`](../axel/map.md).

## Parameters

`None`

## Returns

`Axel`
    A new axel with every bound field, in every frame, cast to
    `Note`-kind; every unbound field unchanged.

## Examples

Example 1. Cast a `Series` field of integers to text.

```cpp
auto r0 = pluss::scribble::Registry({{"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2)})}});
auto axel = pluss::scribble::Axel({r0});

auto out = axel.fields({"id"}).cast().note();
auto ids = out.registry(0).fields("id");
std::cout << std::get<pluss::Note>(ids.at(0)).at(0) << " " << std::get<pluss::Note>(ids.at(1)).at(0);
```

### Output

```text
1 2
```

## Notes

- Bound via [`Axel::cast()`](../axel/cast.md) — remember that entry
  point binds *every* field of every frame it's called on, so scope it
  with [`fields(keys)`](../axel/fields.md) first if you only want to
  cast some fields.

## See Also

- [`AxelCastAccessor::series`](series.md): the inverse direction,
  casting text fields to numeric.
- [`Axel::cast`](../axel/cast.md): the entry point this accessor is
  reached from.
- [`RegistryCastAccessor::note`](../registry-cast/note.md): the
  single-frame operation this method broadcasts.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="series.html">
    <span class="label">Next</span>
    <span class="title">series</span>
  </a>
</div>
