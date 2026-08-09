<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1e1cd1faf5b2 -->

# AxelFieldAxisAccessor::series

[Back to AxelFieldAxisAccessor API Reference](../../axel_fieldaxis_accessor_api_reference)

## Signature

```cpp
Axel AxelFieldAxisAccessor::series() const
```

## What It Does

Named shorthand for [`kind(SlotKind::Series)`](kind.md) — returns a new
axel where every registry frame is narrowed to only its `Series`-kind
(numeric) fields.

## Parameters

`None`

## Returns

`Axel`
    A new axel with every frame narrowed to its `Series`-kind fields.

## Examples

Example 1. Keep only the numeric field in every frame.

```cpp
auto r0 = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})},
});
auto axel = pluss::bundle::Axel({r0, r0});

auto out = axel.fieldaxis().series();
for (const auto& key : out.registry(0).fields()) std::cout << key << " ";
```

### Output

```text
id
```

## Notes

- Equivalent to `kind(SlotKind::Series)` — see [`kind()`](kind.md) for
  the general form.
- Applied independently per frame — different frames can end up with
  different resulting field sets if their schemas differ.

## See Also

- [`AxelFieldAxisAccessor::kind`](kind.md): the general form this
  method is a fixed shorthand for.
- [`AxelFieldAxisAccessor::note`](note.md) /
  [`timeline`](timeline.md) / [`timeindex`](timeindex.md): the
  shorthands for the other value kinds.

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
