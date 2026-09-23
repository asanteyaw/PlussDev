<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1f6967423f7f -->

# AxelFieldAxisAccessor::empty

[Back to AxelFieldAxisAccessor API Reference](../../axel_fieldaxis_accessor_api_reference)

## Signature

```cpp
Axel AxelFieldAxisAccessor::empty() const
```

## What It Does

Named shorthand for [`kind(SlotKind::Empty)`](kind.md) — returns a new
axel where every registry frame is narrowed to only its `Empty`-kind
fields — fields whose `Slot` holds zero values.

## Parameters

`None`

## Returns

`Axel`
    A new axel with every frame narrowed to its `Empty`-kind fields.

## Examples

Example 1. Keep only the zero-value field in every frame.

```cpp
auto r0 = pluss::scribble::Registry({
    {"emptyfield", pluss::scribble::Slot(std::vector<pluss::scribble::Slot::Value>{})},
    {"id", pluss::scribble::Slot({pluss::make_series(1)})},
});
auto axel = pluss::scribble::Axel({r0});

auto out = axel.fieldaxis().empty();
for (const auto& key : out.registry(0).fields()) std::cout << key << " ";
```

### Output

```text
emptyfield
```

## Notes

- Equivalent to `kind(SlotKind::Empty)` — see [`kind()`](kind.md) for
  the general form.
- Distinct from [`nulls()`](nulls.md): `empty()` selects fields with
  zero values, while `nulls()` selects fields whose values are all
  present but all the null sentinel.
- Applied independently per frame — different frames can end up with
  different resulting field sets if their schemas differ.

## See Also

- [`AxelFieldAxisAccessor::kind`](kind.md): the general form this
  method is a fixed shorthand for.
- [`AxelFieldAxisAccessor::nulls`](nulls.md): the related, but distinct,
  shorthand for all-null (not zero-value) fields.
- [`Axel::empty`](../axel/empty.md): the depth-axis emptiness check —
  a different question from this field-level check.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="kind.html">
    <span class="label">Next</span>
    <span class="title">kind</span>
  </a>
</div>
