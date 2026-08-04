<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2f059cc8103d -->

# RegistryFieldAxisAccessor::like

[Back to RegistryFieldAxisAccessor API Reference](../../registry_fieldaxis_accessor_api_reference)

## Signature

```cpp
Registry RegistryFieldAxisAccessor::like(std::string_view needle, bool case_sensitive = false) const
```

## What It Does

Returns a new registry containing only the fields whose **name** contains
`needle` as a substring. Matching is case-insensitive by default.

## Parameters

**needle** (`std::string_view`)
    The substring to search for within each field name.

**case_sensitive** (`bool`, optional, defaults to `false`)
    Whether the substring search is case-sensitive.

## Returns

`Registry`
    A new registry containing only the fields whose name contains `needle`.

## Examples

Example 1. A case-insensitive search matches both `id` and `user_id`.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"user_id", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})},
});

auto out = rg.fieldaxis().like("ID");
for (const auto& key : out.fields()) std::cout << key << " ";
```

### Output

```text
id user_id
```

Example 2. A case-sensitive search against the same registry matches nothing,
since no field name literally contains `"ID"` in that case.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"user_id", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});

auto out = rg.fieldaxis().like("ID", true);
std::cout << "nfields=" << out.nfields();
```

### Output

```text
nfields=0
```

## Notes

- Matches against field **names**, not field values.
- An empty result is a valid, defined, empty registry — not a throw.

## See Also

- [`RegistryFieldAxisAccessor::match`](match.md): the regex-based counterpart,
  for more precise patterns than a plain substring.
- [`RegistryFieldAxisAccessor::prefix`](prefix.md) / [`RegistryFieldAxisAccessor::suffix`](suffix.md):
  anchored variants of this same substring search.
- [`Registry::fields`](../registry/fields.md): select fields you already
  know the exact names of, rather than by pattern.

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
