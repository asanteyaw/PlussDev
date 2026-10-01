<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f5e3ebecae0f -->

# RegistryFieldAxisAccessor::suffix

[Back to RegistryFieldAxisAccessor API Reference](../../registry_fieldaxis_accessor_api_reference)

## Signature

```cpp
Registry RegistryFieldAxisAccessor::suffix(std::string_view token) const
```

## What It Does

Returns a new registry containing only the fields whose **name** ends with
`token`. Case-sensitive; there is no `case_sensitive` option on this method,
unlike [`like()`](like.md).

## Parameters

**token** (`std::string_view`)
    The suffix each field name must end with.

## Returns

`Registry`
    A new registry containing only the fields whose name ends with `token`.

## Examples

Example 1. Select every field name ending with `_id`.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"user_id", pluss::scribble::Slot({pluss::make_series(3), pluss::make_series(4)})},
    {"city", pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})},
});

auto out = rg.fieldaxis().suffix("_id");
for (const auto& key : out.fields()) std::cout << key << " ";
```

### Output

```text
user_id
```

## Notes

- Case-sensitive — no `case_sensitive` parameter, unlike [`like()`](like.md).
- An empty result is a valid, defined, empty registry — not a throw.

## See Also

- [`RegistryFieldAxisAccessor::prefix`](prefix.md): the mirror-image,
  starts-with counterpart.
- [`RegistryFieldAxisAccessor::like`](like.md): unanchored substring search,
  anywhere in the name.
- [`RegistryFieldAxisAccessor::match`](match.md): regex-based matching, for
  patterns this method and `prefix()` can't express.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="series.html">
    <span class="label">Previous</span>
    <span class="title">series</span>
  </a>
</div>
