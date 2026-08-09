<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 59f09b8135ee -->

# AxelFieldAxisAccessor::nulls

[Back to AxelFieldAxisAccessor API Reference](../../axel_fieldaxis_accessor_api_reference)

## Signature

```cpp
Axel AxelFieldAxisAccessor::nulls() const
```

## What It Does

Named shorthand for [`kind(SlotKind::Null)`](kind.md) — returns a new
axel where every registry frame is narrowed to only its `Null`-kind
fields — fields whose every value is the null sentinel.

## Parameters

`None`

## Returns

`Axel`
    A new axel with every frame narrowed to its `Null`-kind fields.

## Examples

Example 1. Keep only the all-null field in every frame.

```cpp
auto r0 = pluss::bundle::Registry({
    {"nullfield", pluss::bundle::Slot({pluss::bundle::Slot::Value{std::monostate{}}})},
    {"id", pluss::bundle::Slot({pluss::make_series(1)})},
});
auto axel = pluss::bundle::Axel({r0});

auto out = axel.fieldaxis().nulls();
for (const auto& key : out.registry(0).fields()) std::cout << key << " ";
```

### Output

```text
nullfield
```

## Notes

- Equivalent to `kind(SlotKind::Null)` — see [`kind()`](kind.md) for
  the general form.
- Applied independently per frame — different frames can end up with
  different resulting field sets if their schemas differ.

## See Also

- [`AxelFieldAxisAccessor::kind`](kind.md): the general form this
  method is a fixed shorthand for.
- [`AxelFieldAxisAccessor::empty`](empty.md): the shorthand for
  zero-value fields, a related but distinct category.

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
