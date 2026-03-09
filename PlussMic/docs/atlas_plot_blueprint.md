# Atlas Plot Blueprint

## Goal
Make `Atlas` plotting depth-native, not just a frame-selection shim over `DataFrame::plot()`.

## Current State
- `Atlas::plot().frames(frame_index)` picks one frame and delegates to `DataFrame::plot()`.
- This is useful for compatibility, but it does not visualize cross-frame structure (`depth x cases x features`).

## Target Reach
`Atlas` should support two plotting layers:

1. Frame view (existing, keep)
- `atlas.plot().frames(i).line(...)`
- Exact `DataFrame` behavior on a chosen frame.

2. Depth view (new, Atlas-native)
- `atlas.plot().surface("feature")`
- `atlas.plot().wireframe("feature")`
- `atlas.plot().cloud3("x", "y", "z")`
- Future: depth cloud/ridge/tri-combination views, depth-wise faceting, and frame-label encoding.

## Axis Contract
- `x`: case axis (within frame)
- `y`: depth axis (frame index or `axis0_labels`)
- `z`: numeric feature value

Depth labels from `axis0_labels` should be used when available.

## Doctrine Alignment
- Keep reshaping at Atlas layer; keep rendering delegated to `flash`/`PlussPl`.
- Use pluss types (`Series`, `Thread`, `DataFrame`) end-to-end.
- Avoid alias/API bloat.
- Use linear passes where possible.

## Rollout Plan
### Tranche 1 (implemented now)
- Add `Atlas::plot()` Atlas-native accessor with `frames(i)` for per-frame DataFrame plotting.
- Add `surface(feature)`, `wireframe(feature)`, and `cloud3(x, y, z)` over stacked frames.
- Add demo output for quick visual validation.

### Tranche 2
- [x] Add depth aggregation view:
  - `atlas.plot().depth("feature", ...)`
  - supports center statistic + optional lower/upper envelopes across frames.
- [ ] Add depth-axis configuration options (ordering, label formatting, subsampling).

### Tranche 3
- [x] Add atlas-native feature triplet panels:
  - `atlas.plot().triplot(...)` over stacked frame points.
- [x] Add atlas-native comparison plots:
  - `atlas.plot().depth_compare(...)` for multi-feature depth overlays.
  - `atlas.plot().depth_facet(...)` for per-feature depth facets.
- [ ] Add rolling depth summaries (windowed depth statistics).
- Add stricter API docs and tests for shape/label contracts.

## Keep / Remove Rule
- Keep existing frame shim path (`Atlas::plot().frames(i)`) because it is low-friction and useful.
- Keep Atlas-native features only if they produce clearer insight than per-frame plotting.
- Remove weak or redundant ops after demo validation.
