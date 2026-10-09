# PlussBI API Reference

[Back to PlussTils API Reference](../api_reference.md) · [PlussBI Manual](../manual/bi.md)

The PlussBI reference covers the declarative model in `pluss::bi`: query plans,
filters, controls, bindings, views, boards, studios, sessions, and serving. Its
free-function DSL is the canonical concise entry point; lower-level builders
are documented only where they expose additional control rather than duplicate
another spelling.

Types and functions in `pluss::bi::detail` are implementation machinery and do
not appear in the public reference.

## Boards, tiles, and studios

A [`Board`](bi-board.md) is one page of a dashboard: a key, a title, a grid, and an ordered list of
[`Tile`](bi-tile.md)s. Tiles are placed with a [`LayoutSpec`](bi-layout-spec.md), and each board method is on its own
page, from [`key`](bi-board/key.md) and [`title`](bi-board/title.md) through [`view`](bi-board/view.md) and
[`tiles`](bi-board/tiles.md).

A [`Studio`](bi-studio.md) owns the boards, the session, and the source tables. Its render is described by
[`StudioRender`](bi-studio-render.md), [`BoardRender`](bi-board-render.md), [`TileRender`](bi-tile-render.md), and
[`CacheStats`](bi-cache-stats.md). Its exports and serving calls are on their own pages, from
[`export_html`](bi-studio/export_html.md) to [`start_serving`](bi-studio/start_serving.md). It returns a [`ServeHandle`](bi-serve-handle.md), whose members are [`running`](bi-serve-handle/running.md), [`port`](bi-serve-handle/port.md), [`url`](bi-serve-handle/url.md), [`stop`](bi-serve-handle/stop.md), and [`wait`](bi-serve-handle/wait.md).

## Views and controls

A [`View`](bi-view.md) is a tile that shows data. Its DSL constructors are
[`kpi`](bi-kpi.md), [`line`](bi-line.md), [`bar`](bi-bar.md), [`scatter`](bi-scatter.md),
[`heatmap`](bi-heatmap.md), [`corr`](bi-corr.md), [`table`](bi-table.md), and [`pivot`](bi-pivot.md), and
each one sets a [`ViewKind`](bi-view-kind.md). The view's members are on their own pages, from
[`key`](bi-view/key.md) and [`query`](bi-view/query.md) through [`pivot_values`](bi-view/pivot_values.md),
[`dropna`](bi-view/dropna.md), [`sort`](bi-view/sort.md), [`decimals`](bi-view/decimals.md), and [`make`](bi-view/make.md).

A [`Control`](bi-control.md) is a tile that filters the other tiles on its board. Its DSL constructors are
[`category`](bi-category.md), [`range`](bi-range.md), and [`time_range`](bi-time-range.md), and each one sets a
[`ControlKind`](bi-control-kind.md). Its members are [`key`](bi-control/key.md), [`label`](bi-control/label.md),
[`feature`](bi-control/feature.md), [`kind`](bi-control/kind.md), [`scope`](bi-control/scope.md),
[`place`](bi-control/place.md), [`min_height`](bi-control/min_height.md), and [`scoped`](bi-control/scoped.md).

## Filters

The [`Filter`](bi-filter.md) value carries one narrowing choice: a column, an
operator from [`FilterOp`](bi-filter/filter_op.md), and a payload. Its factories
are [`eq`](bi-filter/eq.md), [`ne`](bi-filter/ne.md), [`gt`](bi-filter/gt.md),
[`ge`](bi-filter/ge.md), [`lt`](bi-filter/lt.md), [`le`](bi-filter/le.md),
[`between`](bi-filter/between.md), [`between_time`](bi-filter/between_time.md),
[`between_duration`](bi-filter/between_duration.md), [`in`](bi-filter/in.md), and
[`scoped`](bi-filter/scoped.md). The payload types are [`FilterScalar`](bi-filter/filter_scalar.md),
[`FilterSet`](bi-filter/filter_set.md), and [`FilterRange`](bi-filter/filter_range.md).

## Session

The [`Session`](bi-session.md) class holds the choices a reader makes, as
filters and named parameters, and it versions every change. Its methods have
their own pages: [`set_filter`](bi-session/set_filter.md),
[`set_param`](bi-session/set_param.md), and
[`clear_param`](bi-session/clear_param.md).

```{toctree}
:maxdepth: 1
:hidden:

bi-view
bi-view/key
bi-view/query
bi-view/bind
bi-view/layout
bi-view/place
bi-view/min_height
bi-view/title
bi-view/feature
bi-view/x
bi-view/y
bi-view/z
bi-view/hue
bi-view/features
bi-view/horizontal
bi-view/op
bi-view/pivot_index
bi-view/pivot_columns
bi-view/pivot_values
bi-view/dropna
bi-view/sort
bi-view/decimals
bi-view/make
bi-kpi
bi-line
bi-bar
bi-scatter
bi-heatmap
bi-corr
bi-table
bi-pivot
bi-view-kind
bi-control
bi-control/key
bi-control/label
bi-control/feature
bi-control/kind
bi-control/scope
bi-control/place
bi-control/min_height
bi-control/scoped
bi-category
bi-range
bi-time-range
bi-control-kind
bi-session
bi-session/set_filter
bi-session/clear_filter
bi-session/clear_filters
bi-session/has_filter
bi-session/filter
bi-session/filters
bi-session/set_param
bi-session/clear_param
bi-session/clear_params
bi-session/has_param
bi-session/param
bi-session/params
bi-session/empty
bi-session/version
bi-filter
bi-filter/filter_op
bi-filter/filter_scalar
bi-filter/filter_set
bi-filter/filter_range
bi-filter/eq
bi-filter/ne
bi-filter/gt
bi-filter/ge
bi-filter/lt
bi-filter/le
bi-filter/between
bi-filter/between_time
bi-filter/between_duration
bi-filter/in
bi-filter/scoped
bi-board
bi-board/key
bi-board/title
bi-board/nrows
bi-board/ncols
bi-board/version
bi-board/grid
bi-board/control
bi-board/view
bi-board/tiles
bi-tile
bi-layout-spec
bi-layout-spec/at
bi-studio
bi-studio/key
bi-studio/session
bi-studio/source
bi-studio/board
bi-studio/boards
bi-studio/set_filter
bi-studio/clear_filter
bi-studio/refresh
bi-studio/render
bi-studio/save
bi-studio/load
bi-studio/restore_session
bi-studio/export_html
bi-studio/export_svg
bi-studio/export_png
bi-studio/export_pdf
bi-studio/start_serving
bi-serve-handle
bi-serve-handle/running
bi-serve-handle/port
bi-serve-handle/url
bi-serve-handle/stop
bi-serve-handle/wait
bi-studio/serve
bi-studio/show
bi-dashboard-chrome
bi-studio-render
bi-board-render
bi-tile-render
bi-cache-stats
```
