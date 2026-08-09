<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 744b375e0c39 -->

# AxelFieldAxisAccessor::timeindex

[Back to AxelFieldAxisAccessor API Reference](../../axel_fieldaxis_accessor_api_reference)

## Signature

```cpp
Axel AxelFieldAxisAccessor::timeindex() const
```

## What It Does

Named shorthand for [`kind(SlotKind::TimeIndex)`](kind.md) — returns a
new axel where every registry frame is narrowed to only its
`TimeIndex`-kind fields.

## Parameters

`None`

## Returns

`Axel`
    A new axel with every frame narrowed to its `TimeIndex`-kind
    fields.

## Examples

Example 1. Keep only the `TimeIndex`-kind field in every frame.

```cpp
auto r0 = pluss::bundle::Registry({
    {"idx", pluss::bundle::Slot({pluss::make_timeindex(pluss::make_timeline({"2024-01-01T00:00:00Z"}))})},
    {"id", pluss::bundle::Slot({pluss::make_series(1)})},
});
auto axel = pluss::bundle::Axel({r0});

auto out = axel.fieldaxis().timeindex();
for (const auto& key : out.registry(0).fields()) std::cout << key << " ";
```

### Output

```text
idx
```

## Notes

- Equivalent to `kind(SlotKind::TimeIndex)` — see [`kind()`](kind.md)
  for the general form.
- Applied independently per frame — different frames can end up with
  different resulting field sets if their schemas differ.

## See Also

- [`AxelFieldAxisAccessor::kind`](kind.md): the general form this
  method is a fixed shorthand for.
- [`AxelFieldAxisAccessor::timeline`](timeline.md): the shorthand for
  the related `Timeline`-kind fields.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="suffix.html">
    <span class="label">Previous</span>
    <span class="title">suffix</span>
  </a>
  <a class="next-link" href="timeline.html">
    <span class="label">Next</span>
    <span class="title">timeline</span>
  </a>
</div>
