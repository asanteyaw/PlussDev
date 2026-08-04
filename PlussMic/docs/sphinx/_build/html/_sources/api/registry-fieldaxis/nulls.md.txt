<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6498b1b88ba6 -->

# RegistryFieldAxisAccessor::nulls

[Back to RegistryFieldAxisAccessor API Reference](../../registry_fieldaxis_accessor_api_reference)

## Signature

```cpp
Registry RegistryFieldAxisAccessor::nulls() const
```

## What It Does

A named shorthand for [`kind(SlotKind::Null)`](kind.md) — returns a new
registry containing only the fields whose *every* value is null (`std::monostate`).

```{important} All-null, not merely contains-a-null
`SlotKind::Null` means every value in the field is null — a field with a mix of
null and non-null values reports whatever kind its non-null values share (or
`Mixed`, if those differ), not `Null`. Use [`Registry::isnull`](../registry/isnull.md)
to find individual null values within a field that isn't entirely null.
```

## Parameters

`None`

## Returns

`Registry`
    A new registry containing only the fields whose values are all null.

## Examples

Example 1. Isolate a field where every value is null.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"all_null", pluss::bundle::Slot({pluss::bundle::Slot::Value{std::monostate{}}, pluss::bundle::Slot::Value{std::monostate{}}})},
});

auto out = rg.fieldaxis().nulls();
for (const auto& key : out.fields()) std::cout << key << " ";
```

### Output

```text
all_null
```

## Notes

- See the important callout above — a partially-null field is not selected by
  this method.
- Equivalent to `kind(SlotKind::Null)` — see [`kind()`](kind.md) for the
  general form.

## See Also

- [`RegistryFieldAxisAccessor::kind`](kind.md): the general form this method
  wraps.
- [`Registry::isnull`](../registry/isnull.md) / [`Registry::dropnull`](../registry/dropnull.md):
  value-level null handling, for fields that are only partially null.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="note.html">
    <span class="label">Previous</span>
    <span class="title">note</span>
  </a>
  <a class="next-link" href="prefix.html">
    <span class="label">Next</span>
    <span class="title">prefix</span>
  </a>
</div>
