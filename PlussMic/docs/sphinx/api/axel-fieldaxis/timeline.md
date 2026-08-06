<!-- pluss-hand-authored -->

# AxelFieldAxisAccessor::timeline

[Back to AxelFieldAxisAccessor API Reference](../../axel_fieldaxis_accessor_api_reference)

## Signature

```cpp
Axel AxelFieldAxisAccessor::timeline() const
```

## What It Does

Named shorthand for [`kind(SlotKind::Timeline)`](kind.md) — returns a
new axel where every registry frame is narrowed to only its
`Timeline`-kind (datetime) fields.

## Parameters

`None`

## Returns

`Axel`
    A new axel with every frame narrowed to its `Timeline`-kind fields.

## Examples

Example 1. Keep only the `Timeline`-kind field in every frame.

```cpp
auto r0 = pluss::bundle::Registry({
    {"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-01T00:00:00Z")})},
    {"id", pluss::bundle::Slot({pluss::make_series(1)})},
});
auto axel = pluss::bundle::Axel({r0});

auto out = axel.fieldaxis().timeline();
for (const auto& key : out.registry(0).fields()) std::cout << key << " ";
```

### Output

```text
ts
```

## Notes

- Equivalent to `kind(SlotKind::Timeline)` — see [`kind()`](kind.md)
  for the general form.
- Applied independently per frame — different frames can end up with
  different resulting field sets if their schemas differ.

## See Also

- [`AxelFieldAxisAccessor::kind`](kind.md): the general form this
  method is a fixed shorthand for.
- [`AxelFieldAxisAccessor::timeindex`](timeindex.md): the shorthand for
  the related `TimeIndex`-kind fields.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="timeindex.html">
    <span class="label">Previous</span>
    <span class="title">timeindex</span>
  </a>
</div>
