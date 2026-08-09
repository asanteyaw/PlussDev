<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9bc27f7d7ff8 -->

# AxelFieldAxisAccessor::mixed

[Back to AxelFieldAxisAccessor API Reference](../../axel_fieldaxis_accessor_api_reference)

## Signature

```cpp
Axel AxelFieldAxisAccessor::mixed() const
```

## What It Does

Named shorthand for [`kind(SlotKind::Mixed)`](kind.md) — returns a new
axel where every registry frame is narrowed to only its heterogeneous
(`Mixed`-kind) fields — fields whose values span more than one value
kind.

## Parameters

`None`

## Returns

`Axel`
    A new axel with every frame narrowed to its `Mixed`-kind fields.

## Examples

Example 1. Keep only the field mixing `Series` and `Note` values in
every frame.

```cpp
auto r0 = pluss::bundle::Registry({
    {"mixed", pluss::bundle::Slot({pluss::make_series(1), pluss::make_note("x")})},
    {"id", pluss::bundle::Slot({pluss::make_series(1)})},
});
auto axel = pluss::bundle::Axel({r0});

auto out = axel.fieldaxis().mixed();
for (const auto& key : out.registry(0).fields()) std::cout << key << " ";
```

### Output

```text
mixed
```

## Notes

- Equivalent to `kind(SlotKind::Mixed)` — see [`kind()`](kind.md) for
  the general form and [`Slot::kind`](../slot/kind.md) for exactly when
  a slot is classified `Mixed`.
- Applied independently per frame — different frames can end up with
  different resulting field sets if their schemas differ.

## See Also

- [`AxelFieldAxisAccessor::kind`](kind.md): the general form this
  method is a fixed shorthand for.
- [`AxelFieldAxisAccessor::nulls`](nulls.md) / [`empty`](empty.md): the
  shorthands for the remaining kind categories.

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
