<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a0ecad6ed6cc -->

# RegistryFieldAxisAccessor::mixed

[Back to RegistryFieldAxisAccessor API Reference](../../registry_fieldaxis_accessor_api_reference)

## Signature

```cpp
Registry RegistryFieldAxisAccessor::mixed() const
```

## What It Does

A named shorthand for [`kind(SlotKind::Mixed)`](kind.md) — returns a new
registry containing only the fields whose `Slot::kind()` is
`SlotKind::Mixed` (the field holds more than one distinct value kind, or a
`Series` field mixes numeric dtypes). These are exactly the fields that make a
registry fail [`to_kindle()`](../registry/to_kindle.md)'s stream-compatibility
check.

## Parameters

`None`

## Returns

`Registry`
    A new registry containing only the genuinely mixed-kind fields.

## Examples

Example 1. Isolate the mixed field in a registry that also has a uniform field.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"mixed", pluss::bundle::Slot({pluss::make_series(1), pluss::make_note("x")})},
});

auto out = rg.fieldaxis().mixed();
for (const auto& key : out.fields()) std::cout << key << " ";
```

### Output

```text
mixed
```

## Notes

- Equivalent to `kind(SlotKind::Mixed)` — see [`kind()`](kind.md) for the
  general form.
- Directly identifies which fields would make [`to_kindle()`](../registry/to_kindle.md)
  throw.

## See Also

- [`RegistryFieldAxisAccessor::kind`](kind.md): the general form this method
  wraps.
- [`Registry::to_kindle`](../registry/to_kindle.md): the conversion these
  fields block.
- [`Registry::profiles`](../registry/profiles.md): the per-field type tally
  that shows exactly *why* a field is mixed.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="match.html">
    <span class="label">Previous</span>
    <span class="title">match</span>
  </a>
  <a class="next-link" href="note.html">
    <span class="label">Next</span>
    <span class="title">note</span>
  </a>
</div>
