<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 34c2313bd94d -->

# Atlas::aggregate_over_frames

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
DataFrame Atlas::aggregate_over_frames(const std::function<DataFrame(const std::vector<DataFrame>&)>& fn) const
```

## What It Does

Reduces the **entire** atlas down to a single `DataFrame` by handing `fn` every frame at
once (as a `const std::vector<DataFrame>&`) and returning whatever `fn` builds — distinct
from [`map_frames()`](map_frames.md), which transforms each frame independently and keeps
the atlas's own depth. The `Atlas` analog of
[`Archive::aggregate_over_catalogs`](../archive/aggregate_over_catalogs.md).

## Parameters

**fn** (`const std::function<DataFrame(const std::vector<DataFrame>&)>&`)
    A callback receiving every frame in the atlas at once, returning a single `DataFrame`.
    Must not be empty.

## Returns

`DataFrame`
    Whatever `fn` returns — the atlas collapses to this one frame.

## Examples

Example 1. Sum a feature across every frame into a single-row summary frame.

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Slate(pluss::make_series({100.0, 200.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Slate(pluss::make_series({300.0, 400.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1}, pluss::table::Slate(pluss::make_note({"jan", "feb"})));

auto total = atlas.aggregate_over_frames([](const std::vector<pluss::table::DataFrame>& dfs) {
  double sum = 0.0;
  for (const auto& df : dfs) {
    auto s = df.features("sales").sum().as_series().host<double>();
    sum += s[0];
  }
  return pluss::table::DataFrame({
      {"total_sales", pluss::table::Slate(pluss::make_series({sum}))},
  });
});
std::cout << total;
```

### Output

```text
==================
     | total_sales
------------------
0    | 1000       
------------------
[1 cs x 1 feat]
```

## Notes

- Collapses the whole atlas to a single `DataFrame` — not depth-preserving, unlike
  [`map_frames()`](map_frames.md).
- `fn` must not be empty; throws `"Atlas::aggregate_over_frames: reducer function is
  empty"` otherwise.
- `fn` receives every frame; it's responsible for handling any per-frame differences
  itself (though `Atlas`'s strict-cube constraint means every frame it receives already
  has the same row count).

## See Also

- [`Atlas::map_frames`](map_frames.md): transform each frame independently, preserving
  depth, instead of reducing to one.
- [`Archive::aggregate_over_catalogs`](../archive/aggregate_over_catalogs.md): the
  depth-stacked-catalog analog of this method.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="align_depth.html">
    <span class="label">Next</span>
    <span class="title">align_depth</span>
  </a>
</div>
