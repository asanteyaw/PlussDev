<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: add01b15668a -->

# RegistryFieldAxisAccessor::series

[Back to RegistryFieldAxisAccessor API Reference](../../registry_fieldaxis_accessor_api_reference)

## Signature

```cpp
Registry RegistryFieldAxisAccessor::series() const
```

## What It Does

A named shorthand for [`kind(SlotKind::Series)`](kind.md) — returns a new
registry containing only the fields whose `Slot::kind()` is `SlotKind::Series`
(every value in the field is a `pluss::Series` scalar; no nulls, no other
kinds).

## Parameters

`None`

## Returns

`Registry`
    A new registry containing only the `Series`-kind fields.

## Examples

Example 1. Select the numeric field(s) out of a mixed-kind registry.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"city", pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})},
});

auto out = rg.fieldaxis().series();
for (const auto& key : out.fields()) std::cout << key << " ";
```

### Output

```text
id
```

## Notes

- Equivalent to `kind(SlotKind::Series)` — see [`kind()`](kind.md) for the
  general form.
- A field with even one non-`Series` or null value is excluded — see
  [`RegistryFieldAxisAccessor::mixed`](mixed.md) and
  [`RegistryFieldAxisAccessor::nulls`](nulls.md) for those cases.

## See Also

- [`RegistryFieldAxisAccessor::kind`](kind.md): the general form this method
  wraps.
- [`RegistryFieldAxisAccessor::note`](note.md) / [`timeline`](timeline.md) /
  : the shorthands for the other scalar kinds.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="prefix.html">
    <span class="label">Previous</span>
    <span class="title">prefix</span>
  </a>
  <a class="next-link" href="suffix.html">
    <span class="label">Next</span>
    <span class="title">suffix</span>
  </a>
</div>
