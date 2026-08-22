<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9a16e1eb6278 -->

# Registry::invalid

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry Registry::invalid() const
```

## What It Does

Returns a new registry where every field's values are replaced by a boolean flag
for whether each value is invalid, applying [`Slot::invalid()`](../slot/invalid.md)
to every field via [`map()`](map.md). For numeric (`Series`) values this includes
`NaN`, alongside genuinely missing (null) values — see the important callout
below for how this compares to [`isnull()`](isnull.md).

```{important} invalid() and isnull() overlap for NaN, but ask different questions
For a `Series` field, a `NaN` value is flagged by **both** `invalid()` and
`isnull()` — in the tested case below they produce identical flags. `invalid()`
is the broader, domain-aware check (what counts as "not a usable value" for each
kind); `isnull()` is specifically about missing/null values. Don't assume they
always agree for every value kind — see [`Slot::invalid`](../slot/invalid.md) for
the authoritative per-kind rules.
```

## Parameters

`None`

## Returns

`Registry`
    A new registry with every field's values replaced by boolean invalidity
    flags, one per original value.

## Examples

Example 1. A numeric field containing a `NaN` value.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1.0), pluss::make_series(std::numeric_limits<double>::quiet_NaN()), pluss::make_series(3.0)})},
});

auto out = rg.invalid();
auto flags = out.fields("id");
std::cout << "[";
for (std::int64_t i = 0; i < flags.size(); ++i) {
  if (i) std::cout << ", ";
  std::cout << std::boolalpha << std::get<pluss::Series>(flags.at(i)).to_vec<bool>()[0];
}
std::cout << "]";
```

### Output

```text
[false, true, false]
```

## Notes

- Never throws on a mismatched field kind — unlike [`fillnull()`](fillnull.md).
- Full per-value semantics (what counts as invalid for each domain kind) are
  documented on [`Slot::invalid`](../slot/invalid.md).
- Never mutates the receiver — always returns a new `Registry`.

## See Also

- [`Registry::isnull`](isnull.md): the narrower, missing-value-specific check —
  see the important callout above for how the two compare.
- [`Registry::dropnull`](dropnull.md): removes null values rather than flagging
  invalid ones.
- [`Catalog::invalid`](../catalog/invalid.md): the equivalent operation on the
  kindle model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="head.html">
    <span class="label">Previous</span>
    <span class="title">head</span>
  </a>
  <a class="next-link" href="isnull.html">
    <span class="label">Next</span>
    <span class="title">isnull</span>
  </a>
</div>
