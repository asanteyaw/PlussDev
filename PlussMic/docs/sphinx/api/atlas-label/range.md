<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0ce5a85b0e92 -->

# AtlasLabelAccessor::range

[Back to AtlasLabelAccessor API Reference](../../atlas_label_accessor_api_reference)

## Signature

```cpp
AtlasLabelAccessor::range() const -> Atlas
```

## What It Does

Returns a copy of the bound atlas with its depth-axis labels replaced by the default
integer sequence `0, 1, 2, ..., depth() - 1` — equivalent to calling
[`set()`](set.md) with that sequence explicitly, and functionally identical to
[`reset()`](reset.md) (both clear any existing depth labels back to the default
positions).

## Parameters

`None`

## Returns

`Atlas`
    A new atlas, identical in frame content, with `0, 1, 2, ...` as its depth labels.

## Examples

Example 1. Replace an atlas's depth labels with the default integer sequence.

```cpp
auto q1 = pluss::table::DataFrame({
    {"sales", pluss::table::Slate(pluss::make_series({100.0, 120.0}))},
});
auto q2 = pluss::table::DataFrame({
    {"sales", pluss::table::Slate(pluss::make_series({110.0, 90.0}))},
});
auto atlas = pluss::table::Atlas({q1, q2}, pluss::table::Slate(pluss::make_note({"jan", "feb"})));

auto ranged = atlas.label().range();
std::cout << ranged.label().values();
```

### Output

```text
0  0
1  1
ptype: Series
```

## Notes

- Behaves the same as [`reset()`](reset.md) — both produce the default `0, 1, 2, ...`
  integer depth labels. Unlike `reset()`,
  [`DataFrameLabelAccessor`](../dataframe-label/reset.md) has no equivalent `range()`
  name for its case-axis accessor — only `reset()` — so this naming is specific to
  `AtlasLabelAccessor`.
- The original atlas is left unchanged — `range()` returns a new `Atlas`.

## See Also

- [`AtlasLabelAccessor::reset`](reset.md): the equivalent operation, named to match
  "clearing" the labels rather than "ranging" them.
- [`AtlasLabelAccessor::set`](set.md): set the depth labels to any `Slate`, not just the
  default sequence.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pos.html">
    <span class="label">Previous</span>
    <span class="title">pos</span>
  </a>
  <a class="next-link" href="reset.html">
    <span class="label">Next</span>
    <span class="title">reset</span>
  </a>
</div>
