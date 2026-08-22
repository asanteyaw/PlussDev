<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0fd60351db75 -->

# Axel::isnull

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
Axel Axel::isnull() const
```

## What It Does

Returns a new axel where every field's values, in every registry frame,
are replaced by a boolean flag for whether each value is null — applies
[`Registry::isnull()`](../registry/isnull.md) to every frame via
[`map()`](map.md), which in turn applies
[`Slot::isnull()`](../slot/isnull.md) to every field. Works uniformly
across every field regardless of its own value kind — the result is
always `Series[bool]`-valued.

## Parameters

`None`

## Returns

`Axel`
    A new axel with every field's values replaced by boolean null-flags.

## Examples

Example 1. A field with one null value among numeric values reports
`Series[bool]` per value.

```cpp
auto r0 = pluss::scribble::Registry({{"id", pluss::scribble::Slot({pluss::make_series(1), pluss::scribble::Slot::Value{std::monostate{}}, pluss::make_series(3)})}});
auto axel = pluss::scribble::Axel({r0});

auto out = axel.isnull();
std::cout << out.registry(0).fields("id").to_json();
```

### Output

```text
{
  "_type": "Slot",
  "defined": true,
  "size": 3,
  "values": [
    "Series[bool]",
    "Series[bool]",
    "Series[bool]"
  ]
}
```

## Notes

- Unlike [`fillnull()`](fillnull.md), `isnull()` never throws on a field
  whose kind doesn't match a fill payload — it works identically on
  every kind.
- Never mutates the receiver — always returns a new `Axel`.

## See Also

- [`Axel::notnull`](notnull.md): the logical complement.
- [`Axel::invalid`](invalid.md): a related but distinct check —
  invalid (not-a-value) rather than strictly null.
- [`Axel::dropnull`](dropnull.md): removes null values instead of
  flagging them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="invalid.html">
    <span class="label">Previous</span>
    <span class="title">invalid</span>
  </a>
  <a class="next-link" href="kindelize.html">
    <span class="label">Next</span>
    <span class="title">kindelize</span>
  </a>
</div>
