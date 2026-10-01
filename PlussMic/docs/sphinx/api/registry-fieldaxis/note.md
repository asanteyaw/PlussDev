<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f24df731c89b -->

# RegistryFieldAxisAccessor::note

[Back to RegistryFieldAxisAccessor API Reference](../../registry_fieldaxis_accessor_api_reference)

## Signature

```cpp
Registry RegistryFieldAxisAccessor::note() const
```

## What It Does

A named shorthand for [`kind(SlotKind::Note)`](kind.md) — returns a new
registry containing only the fields whose `Slot::kind()` is `SlotKind::Note`
(every value in the field is a `pluss::Note` scalar; no nulls, no other kinds).

## Parameters

`None`

## Returns

`Registry`
    A new registry containing only the `Note`-kind fields.

## Examples

Example 1. Select the text field(s) out of a mixed-kind registry.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"city", pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})},
});

auto out = rg.fieldaxis().note();
for (const auto& key : out.fields()) std::cout << key << " ";
```

### Output

```text
city
```

## Notes

- Equivalent to `kind(SlotKind::Note)` — see [`kind()`](kind.md) for the
  general form.

## See Also

- [`RegistryFieldAxisAccessor::kind`](kind.md): the general form this method
  wraps.
- [`RegistryFieldAxisAccessor::series`](series.md) / [`timeline`](timeline.md) /
  : the shorthands for the other scalar kinds.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mixed.html">
    <span class="label">Previous</span>
    <span class="title">mixed</span>
  </a>
  <a class="next-link" href="nulls.html">
    <span class="label">Next</span>
    <span class="title">nulls</span>
  </a>
</div>
