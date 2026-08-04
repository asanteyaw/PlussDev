<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0ec874d13a46 -->

# Registry::isnull

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry Registry::isnull() const
```

## What It Does

Returns a new registry where every field's values are replaced by a boolean flag
for whether each value is null — applies [`Slot::isnull()`](../slot/isnull.md) to
every field via [`map()`](map.md). Works uniformly across every field regardless
of its own value kind (`Series`, `Note`, `Timeline`, `TimeIndex`) — the result is
always a `Series[bool]`-valued field.

## Parameters

`None`

## Returns

`Registry`
    A new registry with every field's values replaced by boolean null-flags, one
    per original value.

## Examples

Example 1. A field with one null value among numeric values, alongside an
all-present `Note` field — both fields report boolean flags.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::bundle::Slot::Value{std::monostate{}}, pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.isnull();
std::cout << out.fields("id").to_json() << "\n";
std::cout << out.fields("city").to_json();
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

- Unlike [`fillnull()`](fillnull.md), `isnull()` never throws on a field whose
  kind doesn't match a fill payload — it works identically on every kind.
- The full per-value semantics (what counts as "null" for each domain kind) are
  documented on [`Slot::isnull`](../slot/isnull.md).
- Never mutates the receiver — always returns a new `Registry`.

## See Also

- [`Registry::notnull`](notnull.md): the logical complement.
- [`Registry::invalid`](invalid.md): a related but distinct check — invalid
  (not-a-value) rather than strictly null.
- [`Registry::dropnull`](dropnull.md): removes null values instead of flagging
  them.
- [`Catalog::isnull`](../catalog/isnull.md): the equivalent operation on the
  kindle model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="invalid.html">
    <span class="label">Previous</span>
    <span class="title">invalid</span>
  </a>
  <a class="next-link" href="keep.html">
    <span class="label">Next</span>
    <span class="title">keep</span>
  </a>
</div>
