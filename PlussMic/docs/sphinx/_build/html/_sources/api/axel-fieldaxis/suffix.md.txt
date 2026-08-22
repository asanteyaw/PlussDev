<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 91da021c2346 -->

# AxelFieldAxisAccessor::suffix

[Back to AxelFieldAxisAccessor API Reference](../../axel_fieldaxis_accessor_api_reference)

## Signature

```cpp
Axel AxelFieldAxisAccessor::suffix(std::string_view token) const
```

## What It Does

Returns a new axel where every registry frame is narrowed to only the
fields whose **name ends with** `token`. Applies
[`RegistryFieldAxisAccessor::suffix`](../registry-fieldaxis/suffix.md)
to every frame via [`Axel::map`](../axel/map.md).

## Parameters

**token** (`std::string_view`)
    The suffix each surviving field name must end with.

## Returns

`Axel`
    A new axel with every frame narrowed to fields whose name ends with
    `token`.

## Examples

Example 1. Select every field name ending with `id`, in every frame.

```cpp
auto r0 = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"user_id", pluss::scribble::Slot({pluss::make_series(3), pluss::make_series(4)})},
    {"city", pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})},
});
auto axel = pluss::scribble::Axel({r0, r0});

auto out = axel.fieldaxis().suffix("id");
for (const auto& key : out.registry(0).fields()) std::cout << key << " ";
```

### Output

```text
id user_id
```

`"id"` matches both `"id"` itself and `"user_id"`, since both end with
`"id"`.

## Notes

- Anchored at the end of the name — this is not a substring search like
  [`like()`](like.md); the match must reach exactly to the name's end.
- Applied independently per frame — different frames can end up with
  different resulting field sets if their schemas differ.

## See Also

- [`AxelFieldAxisAccessor::prefix`](prefix.md): the mirror-image,
  start-anchored search.
- [`AxelFieldAxisAccessor::like`](like.md): the unanchored substring
  search.
- [`RegistryFieldAxisAccessor::suffix`](../registry-fieldaxis/suffix.md):
  the single-frame operation this method broadcasts.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="series.html">
    <span class="label">Previous</span>
    <span class="title">series</span>
  </a>
  <a class="next-link" href="timeindex.html">
    <span class="label">Next</span>
    <span class="title">timeindex</span>
  </a>
</div>
