# Catalog API Reference

This page documents the full public `Catalog` surface for named stream collections.

## Using This Reference

- Use the left sidebar to open one method page at a time.
- Each method page contains its own signatures, parameter details, notes, verified outputs, and working previous/next navigation.
- This landing page stays intentionally short so the content panel is not one giant scroll of every method.

## Method Pages

The sidebar lists methods alphabetically. You can also use the search box to jump directly to a method name.

## Accessors

`Catalog` exposes nested accessors, each with its own method pages and sidebar:

- `plot()` -> [`CatalogPlotAccessor`](catalog_plot_accessor_api_reference.md)
- `plot3d()` -> [`CatalogPlot3DAccessor`](catalog_plot3d_accessor_api_reference.md)
- `nt()` -> [`CatalogNoteAccessor`](catalog_note_accessor_api_reference.md)
- `tl()` -> [`CatalogTimelineAccessor`](catalog_timeline_accessor_api_reference.md)
- `ts()` -> [`CatalogTimeSeriesAccessor`](catalog_ts_accessor_api_reference.md)
- `cast()` -> [`CatalogCastAccessor`](catalog_cast_accessor_api_reference.md)
- `keyaxis()` -> [`CatalogKeyAxisAccessor`](catalog_keyaxis_accessor_api_reference.md)

```{toctree}
:hidden:
:maxdepth: 1

catalog_plot_accessor_api_reference
catalog_plot3d_accessor_api_reference
catalog_note_accessor_api_reference
catalog_timeline_accessor_api_reference
catalog_ts_accessor_api_reference
catalog_cast_accessor_api_reference
catalog_keyaxis_accessor_api_reference
api/catalog/astype
api/catalog/begin
api/catalog/bfill
api/catalog/bundelize
api/catalog/cast
api/catalog/clean
api/catalog/cloc
api/catalog/defined
api/catalog/display
api/catalog/distinct
api/catalog/drop
api/catalog/dropnull
api/catalog/empty
api/catalog/end
api/catalog/featurize
api/catalog/ffill
api/catalog/fillnull
api/catalog/filter
api/catalog/freq
api/catalog/from_json
api/catalog/get
api/catalog/grep
api/catalog/group
api/catalog/has
api/catalog/head
api/catalog/info
api/catalog/invalid
api/catalog/isnull
api/catalog/keyaxis
api/catalog/keys
api/catalog/keywords
api/catalog/load
api/catalog/load_csv
api/catalog/load_excel
api/catalog/load_json
api/catalog/load_labeled
api/catalog/load_parquet
api/catalog/load_txt
api/catalog/map
api/catalog/map_
api/catalog/max
api/catalog/maxrows
api/catalog/mean
api/catalog/merge
api/catalog/min
api/catalog/minrows
api/catalog/ndistinct
api/catalog/nkeys
api/catalog/notnull
api/catalog/nrows
api/catalog/nt
api/catalog/plot
api/catalog/plot3d
api/catalog/query
api/catalog/rename
api/catalog/rename_
api/catalog/sample
api/catalog/save_as
api/catalog/save_csv
api/catalog/save_excel
api/catalog/save_json
api/catalog/save_parquet
api/catalog/save_txt
api/catalog/select
api/catalog/set
api/catalog/set_
api/catalog/shape
api/catalog/show
api/catalog/similarity
api/catalog/size
api/catalog/slice
api/catalog/sort
api/catalog/stats
api/catalog/std
api/catalog/sum
api/catalog/tabelize
api/catalog/tail
api/catalog/take
api/catalog/tfidf
api/catalog/tl
api/catalog/to_json
api/catalog/to_string
api/catalog/transform
api/catalog/ts
api/catalog/unique
api/catalog/var
api/catalog/where
```
