<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 81f63cfd89ab -->

# AxelFieldAxisAccessor::note

[Back to AxelFieldAxisAccessor API Reference](../../axel_fieldaxis_accessor_api_reference)

## Signature

```cpp
Axel AxelFieldAxisAccessor::note() const
```

## What It Does

Named shorthand for [`kind(SlotKind::Note)`](kind.md) — returns a new
axel where every registry frame is narrowed to only its `Note`-kind
(text) fields.

## Parameters

`None`

## Returns

`Axel`
    A new axel with every frame narrowed to its `Note`-kind fields.

## Examples

Example 1. Keep only the text field in every frame.

```cpp
auto r0 = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"city", pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})},
});
auto axel = pluss::scribble::Axel({r0, r0});

auto out = axel.fieldaxis().note();
for (const auto& key : out.registry(0).fields()) std::cout << key << " ";
```

### Output

```text
city
```

## Notes

- Equivalent to `kind(SlotKind::Note)` — see [`kind()`](kind.md) for
  the general form.
- Applied independently per frame — different frames can end up with
  different resulting field sets if their schemas differ.

## See Also

- [`AxelFieldAxisAccessor::kind`](kind.md): the general form this
  method is a fixed shorthand for.
- [`AxelFieldAxisAccessor::series`](series.md) /
  [`timeline`](timeline.md) / : the
  shorthands for the other value kinds.

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
