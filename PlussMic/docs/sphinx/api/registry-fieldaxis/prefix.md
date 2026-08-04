<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9293a361d3c8 -->

# RegistryFieldAxisAccessor::prefix

[Back to RegistryFieldAxisAccessor API Reference](../../registry_fieldaxis_accessor_api_reference)

## Signature

```cpp
Registry RegistryFieldAxisAccessor::prefix(std::string_view token) const
```

## What It Does

Returns a new registry containing only the fields whose **name** starts with
`token`. Case-sensitive; there is no `case_sensitive` option on this method,
unlike [`like()`](like.md).

## Parameters

**token** (`std::string_view`)
    The prefix each field name must start with.

## Returns

`Registry`
    A new registry containing only the fields whose name starts with `token`.

## Examples

Example 1. Select every field name starting with `user`.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"user_id", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})},
});

auto out = rg.fieldaxis().prefix("user");
for (const auto& key : out.fields()) std::cout << key << " ";
```

### Output

```text
user_id
```

## Notes

- Case-sensitive — no `case_sensitive` parameter, unlike
  [`like()`](like.md).
- An empty result is a valid, defined, empty registry — not a throw.

## See Also

- [`RegistryFieldAxisAccessor::suffix`](suffix.md): the mirror-image,
  ends-with counterpart.
- [`RegistryFieldAxisAccessor::like`](like.md): unanchored substring search,
  anywhere in the name.
- [`RegistryFieldAxisAccessor::match`](match.md): regex-based matching, for
  patterns this method and `suffix()` can't express.

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
