<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7c1b99bbe680 -->

# AxelFieldAxisAccessor::kind

[Back to AxelFieldAxisAccessor API Reference](../../axel_fieldaxis_accessor_api_reference)

## Signature

```cpp
Axel AxelFieldAxisAccessor::kind(SlotKind kind) const
```

## What It Does

Returns a new axel where every registry frame is narrowed to only the
fields whose `Slot::kind()` equals `kind`. This is the general form
behind the seven named shorthand methods
([`series()`](series.md), [`note()`](note.md), [`timeline()`](timeline.md),
[`timeindex()`](timeindex.md), [`mixed()`](mixed.md), [`nulls()`](nulls.md),
[`empty()`](empty.md)) — each is a one-line wrapper calling `kind()`
with a fixed `SlotKind` value. Applies
[`RegistryFieldAxisAccessor::kind`](../registry-fieldaxis/kind.md) to
every frame via [`Axel::map`](../axel/map.md).

## Parameters

**kind** (`SlotKind`)
    The kind to filter fields by — one of `SlotKind::Empty`,
    `SlotKind::Null`, `SlotKind::Series`, `SlotKind::Note`,
    `SlotKind::Timeline`, `SlotKind::TimeIndex`, `SlotKind::Mixed`.

## Returns

`Axel`
    A new axel with every frame narrowed to fields whose kind matches.

## Examples

Example 1. Select every `Series`-kind field directly via `kind()`,
equivalent to calling [`series()`](series.md).

```cpp
auto r0 = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"user_id", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})},
});
auto axel = pluss::bundle::Axel({r0, r0});

auto out = axel.fieldaxis().kind(pluss::bundle::SlotKind::Series);
for (const auto& key : out.registry(0).fields()) std::cout << key << " ";
```

### Output

```text
id user_id
```

## Notes

- Prefer the named shorthand ([`series()`](series.md), [`note()`](note.md),
  etc.) at call sites — they read more clearly than `kind(SlotKind::...)`.
- A field's kind reflects its `Slot`'s own `kind()` — see
  [`Slot::kind`](../slot/kind.md) for exactly how a slot's kind is
  determined from its values.
- Applied independently per frame — different frames can end up with
  different resulting field sets if their schemas differ.

## See Also

- [`AxelFieldAxisAccessor::series`](series.md) / [`note`](note.md) /
  [`timeline`](timeline.md) / [`timeindex`](timeindex.md) /
  [`mixed`](mixed.md) / [`nulls`](nulls.md) / [`empty`](empty.md): the
  seven named shorthands for this method.
- [`RegistryFieldAxisAccessor::kind`](../registry-fieldaxis/kind.md):
  the single-frame operation this method broadcasts.

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
