# Stream API Reference

This page documents the full public `Stream` surface for the Kindle model.

## Using This Reference

- Use the left sidebar to open one method page at a time.
- Each method page contains its own signatures, parameter details, notes, verified outputs, and working previous/next navigation.
- This landing page stays intentionally short so the content panel is not one giant scroll of every method.

## Method Pages

The sidebar lists methods alphabetically. You can also use the search box to jump directly to a method name.

## Accessors

`Stream` exposes nested accessors, each with its own method pages and sidebar:

- `plot()` -> [`StreamPlotAccessor`](stream_plot_accessor_api_reference.md)
- `cast()` -> [`StreamCastAccessor`](stream_cast_accessor_api_reference.md)
- `nt()` -> [`StreamNoteAccessor`](stream_note_accessor_api_reference.md)
- `tl()` -> [`StreamTimelineAccessor`](stream_timeline_accessor_api_reference.md)
- `ts()` -> [`StreamTimeSeriesAccessor`](stream_ts_accessor_api_reference.md)
- `lbl()` -> [`StreamLabelAccessor`](stream_lbl_accessor_api_reference.md)

```{toctree}
:hidden:
:maxdepth: 1

stream_plot_accessor_api_reference
stream_cast_accessor_api_reference
stream_note_accessor_api_reference
stream_timeline_accessor_api_reference
stream_ts_accessor_api_reference
stream_lbl_accessor_api_reference
api/stream/abs
api/stream/as_duration
api/stream/as_note
api/stream/as_series
api/stream/as_timeline
api/stream/astype
api/stream/bfill
api/stream/cast
api/stream/clear_label
api/stream/clear_label_
api/stream/clip
api/stream/defined
api/stream/distinct
api/stream/drop_meta
api/stream/drop_meta_
api/stream/dropnull
api/stream/dtype
api/stream/empty
api/stream/exp
api/stream/ffill
api/stream/fillnull
api/stream/filter
api/stream/freq
api/stream/from_slate
api/stream/has_label
api/stream/has_meta
api/stream/head
api/stream/invalid
api/stream/isnull
api/stream/kind
api/stream/label
api/stream/lbl
api/stream/log
api/stream/map
api/stream/max
api/stream/mean
api/stream/meta
api/stream/min
api/stream/ndistinct
api/stream/notnull
api/stream/nrecords
api/stream/nt
api/stream/plot
api/stream/ptype
api/stream/scribbelize
api/stream/select
api/stream/shape
api/stream/show
api/stream/size
api/stream/slice
api/stream/std
api/stream/sum
api/stream/tabelize
api/stream/tail
api/stream/take
api/stream/tl
api/stream/to_json
api/stream/to_slate
api/stream/to_string
api/stream/ts
api/stream/unique
api/stream/var
api/stream/with_label
api/stream/with_label_
api/stream/with_meta
api/stream/with_meta_
```
