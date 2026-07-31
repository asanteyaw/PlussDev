<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 404e8434f90f -->

# AtlasLabelAccessor::reset

[Back to AtlasLabelAccessor API Reference](../../atlas_label_accessor_api_reference)

## Signature

```cpp
AtlasLabelAccessor::reset() const -> Atlas
```

## What It Does

Returns a copy of the bound atlas with its depth-axis labels cleared back to the default
integer sequence `0, 1, 2, ..., depth() - 1` — the depth-axis counterpart to
[`DataFrame::label().reset()`](../dataframe-label/reset.md).

## Parameters

`None`

## Returns

`Atlas`
    A new atlas, identical in frame content, with its depth labels cleared to
    `0, 1, 2, ...`.

## Examples

Example 1. Reset a labeled atlas's depth labels back to the default sequence.

```cpp
auto q1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0}))},
});
auto q2 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({110.0, 90.0}))},
});
auto atlas = pluss::table::Atlas({q1, q2}, pluss::table::Thread(pluss::make_note({"jan", "feb"})));

auto reset_atlas = atlas.label().reset();
std::cout << "has after reset: " << reset_atlas.label().has() << "\n";
std::cout << reset_atlas.label().values();
```

### Output

```text
has after reset: 0
0  0
1  1
ptype: Series
```

## Notes

- After `reset()`, [`has()`](has.md) reports `false` again — `reset()` clears the
  explicit-labels flag, not just the values, even though
  [`values()`](values.md) always returns something regardless.
- The original atlas is left unchanged — `reset()` returns a new `Atlas`.
- Functionally identical to [`range()`](range.md); prefer whichever name reads more
  clearly at the call site.

## See Also

- [`AtlasLabelAccessor::has`](has.md): check whether depth labels are set — flips to
  `false` after `reset()`.
- [`AtlasLabelAccessor::range`](range.md): the same operation, named after its result
  rather than the act of clearing.
- [`AtlasLabelAccessor::set`](set.md): set the depth labels to something other than the
  default sequence.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="range.html">
    <span class="label">Previous</span>
    <span class="title">range</span>
  </a>
  <a class="next-link" href="set.html">
    <span class="label">Next</span>
    <span class="title">set</span>
  </a>
</div>
