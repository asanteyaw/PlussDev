<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 47d6b7b8967f -->

# AxelFieldAxisAccessor::prefix

[Back to AxelFieldAxisAccessor API Reference](../../axel_fieldaxis_accessor_api_reference)

## Signature

```cpp
Axel AxelFieldAxisAccessor::prefix(std::string_view token) const
```

## What It Does

Returns a new axel where every registry frame is narrowed to only the
fields whose **name starts with** `token`. Applies
[`RegistryFieldAxisAccessor::prefix`](../registry-fieldaxis/prefix.md)
to every frame via [`Axel::map`](../axel/map.md).

## Parameters

**token** (`std::string_view`)
    The prefix each surviving field name must start with.

## Returns

`Axel`
    A new axel with every frame narrowed to fields whose name starts
    with `token`.

## Examples

Example 1. Select every field name starting with `user`, in every
frame.

```cpp
auto r0 = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"user_id", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})},
});
auto axel = pluss::bundle::Axel({r0, r0});

auto out = axel.fieldaxis().prefix("user");
for (const auto& key : out.registry(0).fields()) std::cout << key << " ";
```

### Output

```text
user_id
```

## Notes

- Anchored at the start of the name — this is not a substring search
  like [`like()`](like.md); the match must begin at position `0`.
- Applied independently per frame — different frames can end up with
  different resulting field sets if their schemas differ.

## See Also

- [`AxelFieldAxisAccessor::suffix`](suffix.md): the mirror-image,
  end-anchored search.
- [`AxelFieldAxisAccessor::like`](like.md): the unanchored substring
  search.
- [`RegistryFieldAxisAccessor::prefix`](../registry-fieldaxis/prefix.md):
  the single-frame operation this method broadcasts.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nulls.html">
    <span class="label">Previous</span>
    <span class="title">nulls</span>
  </a>
  <a class="next-link" href="series.html">
    <span class="label">Next</span>
    <span class="title">series</span>
  </a>
</div>
