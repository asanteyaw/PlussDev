<!-- pluss-hand-authored -->

# Axel::dropnull

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
Axel Axel::dropnull() const
```

## What It Does

Returns a new axel with null values removed from **every** field of
**every** frame independently, applying
[`Registry::dropnull()`](../registry/dropnull.md) to every frame via
[`map()`](map.md).

```{important} Can produce a ragged axel
Because each field's nulls are dropped independently — within each
frame, and across frames — fields that started with different null
counts end up with different lengths, and frames can end up internally
misaligned the same way a single [`Registry::dropnull()`](../registry/dropnull.md)
call can produce a ragged registry. Follow up per-frame with
`Registry::nstate()` or `Registry::shape()` if you need to check
alignment afterward.
```

## Parameters

`None`

## Returns

`Axel`
    A new axel with each field's null values removed, in every frame.

## Examples

Example 1. A field with one null value drops to a shorter length.

```cpp
auto r0 = pluss::bundle::Registry({{"id", pluss::bundle::Slot({pluss::make_series(1), pluss::bundle::Slot::Value{std::monostate{}}, pluss::make_series(3)})}});
auto axel = pluss::bundle::Axel({r0});

auto out = axel.dropnull();
std::cout << out.registry(0).fields("id").size();
```

### Output

```text
2
```

## Notes

- See the important callout above — a previously-aligned axel can
  become ragged after this call.
- Never throws on a mismatched field kind — unlike
  [`fillnull()`](fillnull.md), which does.
- Never mutates the receiver — always returns a new `Axel`.

## See Also

- [`Axel::isnull`](isnull.md): identify null values without removing
  them.
- [`Axel::fillnull`](fillnull.md): replace null values instead of
  dropping them, keeping every field's length intact.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="drop.html">
    <span class="label">Previous</span>
    <span class="title">drop</span>
  </a>
  <a class="next-link" href="empty.html">
    <span class="label">Next</span>
    <span class="title">empty</span>
  </a>
</div>
