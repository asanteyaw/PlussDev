# Atlas API Reference

This page documents the full public `Atlas` surface for frame-stack workflows.

## Using This Reference

- Use the left sidebar to open one method page at a time.
- Each method page contains its own signatures, parameter details, notes, verified outputs, and working previous/next navigation.
- This landing page stays intentionally short so the content panel is not one giant scroll of every method.

## Method Pages

The sidebar lists methods alphabetically. You can also use the search box to jump directly to a method name.

## Accessors

`Atlas` exposes nested accessors, each with its own method pages and sidebar:

- `label()` -> [`AtlasLabelAccessor`](atlas_label_accessor_api_reference.md)
- `tl()` -> [`AtlasTimelineAccessor`](atlas_timeline_accessor_api_reference.md)
- `ts()` -> [`AtlasTsAccessor`](atlas_ts_accessor_api_reference.md)
- `plot()` -> [`AtlasPlotAccessor`](atlas_plot_accessor_api_reference.md)

```{toctree}
:hidden:
:maxdepth: 1

api/atlas/aggregate_over_frames
api/atlas/align_depth
api/atlas/axis0_labels
api/atlas/column_names
api/atlas/defined
api/atlas/depth
api/atlas/dloc
api/atlas/empty
api/atlas/features
api/atlas/flow
api/atlas/frame
api/atlas/frames
api/atlas/head
api/atlas/kindelize
api/atlas/label
api/atlas/loc
api/atlas/map
api/atlas/map_frames
api/atlas/ncases
api/atlas/nfeatures
api/atlas/plot
api/atlas/relabel_depth
api/atlas/show
api/atlas/slice_frames
api/atlas/sort_depth
api/atlas/squeeze
api/atlas/tabelize
api/atlas/tail
api/atlas/tl
api/atlas/to_string
api/atlas/ts
api/atlas/unique
```
