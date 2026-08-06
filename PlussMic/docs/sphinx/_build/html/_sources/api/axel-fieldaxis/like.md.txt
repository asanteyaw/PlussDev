<!-- pluss-hand-authored -->

# AxelFieldAxisAccessor::like

[Back to AxelFieldAxisAccessor API Reference](../../axel_fieldaxis_accessor_api_reference)

## Signature

```cpp
Axel AxelFieldAxisAccessor::like(std::string_view needle, bool case_sensitive = false) const
```

## What It Does

Returns a new axel where every registry frame is narrowed to only the
fields whose **name** contains `needle` as a substring — applies
[`RegistryFieldAxisAccessor::like`](../registry-fieldaxis/like.md) to
every frame independently via [`Axel::map`](../axel/map.md).
Matching is case-insensitive by default.

## Parameters

**needle** (`std::string_view`)
    The substring to search for within each field name.

**case_sensitive** (`bool`, optional, defaults to `false`)
    Whether the substring search is case-sensitive.

## Returns

`Axel`
    A new axel with every frame narrowed to fields whose name contains
    `needle`.

## Examples

Example 1. A case-insensitive search matches both `id` and `user_id` in
every frame.

```cpp
auto r0 = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"user_id", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})},
});
auto axel = pluss::bundle::Axel({r0, r0});

auto out = axel.fieldaxis().like("ID");
for (const auto& key : out.registry(0).fields()) std::cout << key << " ";
```

### Output

```text
id user_id
```

Example 2. A case-sensitive search against the same axel matches
nothing, since no field name literally contains `"ID"` in that case.

```cpp
auto r0 = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"user_id", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto axel = pluss::bundle::Axel({r0});

auto out = axel.fieldaxis().like("ID", true);
std::cout << "nfields=" << out.registry(0).nfields();
```

### Output

```text
nfields=0
```

## Notes

- Matches against field **names**, not field values.
- Applied independently per frame — a frame with no matching fields
  ends up with zero fields (still a valid, defined, empty registry),
  not a throw; different frames can end up with different resulting
  field sets if their schemas differ.

## See Also

- [`AxelFieldAxisAccessor::match`](match.md): the regex-based
  counterpart, for more precise patterns than a plain substring.
- [`AxelFieldAxisAccessor::prefix`](prefix.md) / [`suffix()`](suffix.md):
  anchored variants of this same substring search.
- [`RegistryFieldAxisAccessor::like`](../registry-fieldaxis/like.md):
  the single-frame operation this method broadcasts.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="kind.html">
    <span class="label">Previous</span>
    <span class="title">kind</span>
  </a>
  <a class="next-link" href="match.html">
    <span class="label">Next</span>
    <span class="title">match</span>
  </a>
</div>
