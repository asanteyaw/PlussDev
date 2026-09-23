<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: cde8e34238f4 -->

# RegistryFieldAxisAccessor::kind

[Back to RegistryFieldAxisAccessor API Reference](../../registry_fieldaxis_accessor_api_reference)

## Signature

```cpp
Registry RegistryFieldAxisAccessor::kind(SlotKind kind) const
```

## What It Does

Returns a new registry containing only the fields whose `Slot::kind()` equals
`kind`. This is the general form behind the seven named shorthand methods
([`series()`](series.md), [`note()`](note.md), [`timeline()`](timeline.md),
[`timeindex()`](timeindex.md), [`mixed()`](mixed.md), [`nulls()`](nulls.md),
[`empty()`](empty.md)) — each is a one-line wrapper calling `kind()` with a
fixed `SlotKind` value.

## Parameters

**kind** (`SlotKind`)
    The kind to filter fields by — one of `SlotKind::Empty`, `SlotKind::Null`,
    `SlotKind::Series`, `SlotKind::Note`, `SlotKind::Timeline`,
    `SlotKind::TimeIndex`, `SlotKind::Mixed`.

## Returns

`Registry`
    A new registry containing only the fields whose kind matches.

## Examples

Example 1. Select every `Series`-kind field directly via `kind()`, equivalent
to calling [`series()`](series.md).

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"user_id", pluss::scribble::Slot({pluss::make_series(3), pluss::make_series(4)})},
    {"city", pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})},
});

auto out = rg.fieldaxis().kind(pluss::scribble::SlotKind::Series);
for (const auto& key : out.fields()) std::cout << key << " ";
```

### Output

```text
id user_id
```

## Notes

- Prefer the named shorthand ([`series()`](series.md), [`note()`](note.md),
  etc.) at call sites — they read more clearly than `kind(SlotKind::...)`.
- A field's kind reflects its `Slot`'s own `kind()` — see
  [`Slot::kind`](../slot/kind.md) for exactly how a slot's kind is determined
  from its values.

## See Also

- [`RegistryFieldAxisAccessor::series`](series.md) / [`note`](note.md) /
  [`timeline`](timeline.md) / [`timeindex`](timeindex.md) /
  [`mixed`](mixed.md) / [`nulls`](nulls.md) / [`empty`](empty.md): the seven
  named shorthands for this method.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="empty.html">
    <span class="label">Previous</span>
    <span class="title">empty</span>
  </a>
  <a class="next-link" href="like.html">
    <span class="label">Next</span>
    <span class="title">like</span>
  </a>
</div>
