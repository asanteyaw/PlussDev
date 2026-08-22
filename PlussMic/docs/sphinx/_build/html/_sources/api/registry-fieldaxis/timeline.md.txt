<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2969952b3cc7 -->

# RegistryFieldAxisAccessor::timeline

[Back to RegistryFieldAxisAccessor API Reference](../../registry_fieldaxis_accessor_api_reference)

## Signature

```cpp
Registry RegistryFieldAxisAccessor::timeline() const
```

## What It Does

A named shorthand for [`kind(SlotKind::Timeline)`](kind.md) — returns a new
registry containing only the fields whose `Slot::kind()` is
`SlotKind::Timeline` (every value in the field is a `pluss::Timeline` scalar).

## Parameters

`None`

## Returns

`Registry`
    A new registry containing only the `Timeline`-kind fields.

## Examples

Example 1. Select the datetime field out of a mixed-kind registry.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"created_at", pluss::scribble::Slot({pluss::make_timeline("2024-01-01"), pluss::make_timeline("2024-01-02")})},
});

auto out = rg.fieldaxis().timeline();
for (const auto& key : out.fields()) std::cout << key << " ";
```

### Output

```text
created_at
```

## Notes

- Equivalent to `kind(SlotKind::Timeline)` — see [`kind()`](kind.md) for the
  general form.

## See Also

- [`RegistryFieldAxisAccessor::kind`](kind.md): the general form this method
  wraps.
- [`RegistryFieldAxisAccessor::timeindex`](timeindex.md): the related shorthand
  for `TimeIndex`-kind fields, a different datetime representation.
- [`RegistryFieldAxisAccessor::series`](series.md) / [`note`](note.md): the
  shorthands for the other scalar kinds.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="timeindex.html">
    <span class="label">Previous</span>
    <span class="title">timeindex</span>
  </a>
</div>
