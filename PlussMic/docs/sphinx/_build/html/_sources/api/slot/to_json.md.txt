<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f1d063783034 -->

# Slot::to_json

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
std::string Slot::to_json() const
```

## What It Does

Renders the slot as a JSON object: `_type`, `defined`, `size`, and a
`values` array with one type tag per value (e.g. `"Series[int64]"`,
`"Note[numel=1]"`).

```{important} Identical to to_string() and show() — unlike Registry
Unlike [`Registry::to_json()`](../registry/to_json.md), which renders
different (and more detailed) content than
[`Registry::to_string()`](../registry/to_string.md), `Slot::to_json()` is
byte-identical to [`Slot::to_string()`](to_string.md) and
[`Slot::show()`](show.md) — all three (plus `operator<<`) call the exact
same rendering path. There's no separate "compact tally" form at the
`Slot` level the way there is at the `Registry` level.
```

## Parameters

`None`

## Returns

`std::string`
    The JSON rendering described above.

## Examples

Example 1. Render a homogeneous `Series` slot.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});
std::cout << slot.to_json();
```

### Output

```text
{
  "_type": "Slot",
  "defined": true,
  "size": 3,
  "values": [
    "Series[int64]",
    "Series[int64]",
    "Series[int64]"
  ]
}
```

Example 2. An undefined slot renders without throwing.

```cpp
pluss::bundle::Slot undef;
std::cout << undef.to_json();
```

### Output

```text
{ "_type": "Slot", "defined": false }
```

## Notes

- See the important callout above.
- An undefined slot renders as `{ "_type": "Slot", "defined": false }`
  rather than throwing — see Example 2.
- Each value's tag reports its own kind — `"Note[numel=N]"` for `Note`,
  `"Timeline[numel=N]"` for `Timeline`, `"TimeIndex[size=N]"` for
  `TimeIndex`, `"Series[dtype]"` for `Series`, and `"null"` for
  `std::monostate`.

## See Also

- [`Slot::to_string`](to_string.md) / [`Slot::show`](show.md): identical
  renderings under different names.
- [`Slot::type_counts`](type_counts.md): a per-kind tally instead of a
  per-value list.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tl.html">
    <span class="label">Previous</span>
    <span class="title">tl</span>
  </a>
  <a class="next-link" href="to_string.html">
    <span class="label">Next</span>
    <span class="title">to_string</span>
  </a>
</div>
