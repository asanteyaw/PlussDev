<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 91864af7ac25 -->

# RegistryFieldAxisAccessor::empty

[Back to RegistryFieldAxisAccessor API Reference](../../registry_fieldaxis_accessor_api_reference)

## Signature

```cpp
Registry RegistryFieldAxisAccessor::empty() const
```

## What It Does

A named shorthand for [`kind(SlotKind::Empty)`](kind.md) — returns a new
registry containing only the fields whose `Slot` has **zero values**
(`Slot::size() == 0`). This is the field-selection counterpart to
[`Registry::prune()`](../registry/prune.md), which removes these same fields
instead of selecting them.

## Parameters

`None`

## Returns

`Registry`
    A new registry containing only the zero-length fields.

## Examples

Example 1. Isolate the zero-length field in a registry that also has a
non-empty field.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"empty_field", pluss::bundle::Slot(std::vector<pluss::bundle::Slot::Value>{})},
});

auto out = rg.fieldaxis().empty();
for (const auto& key : out.fields()) std::cout << key << " ";
```

### Output

```text
empty_field
```

## Notes

- Equivalent to `kind(SlotKind::Empty)` — see [`kind()`](kind.md) for the
  general form.
- The mirror image of [`Registry::prune()`](../registry/prune.md), which
  removes these fields rather than selecting them.

## See Also

- [`RegistryFieldAxisAccessor::kind`](kind.md): the general form this method
  wraps.
- [`Registry::prune`](../registry/prune.md): removes zero-length fields,
  rather than selecting them.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="kind.html">
    <span class="label">Next</span>
    <span class="title">kind</span>
  </a>
</div>
