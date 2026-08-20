# DataFrameTsAccessor API Reference

This page documents the public time-series accessor returned by `DataFrame::ts()`.

## Using This Reference

- Use the left sidebar to open one method page at a time.
- Each method page contains its own signatures, parameter details, notes, verified outputs, and working previous/next navigation.
- This landing page stays intentionally short so the content panel is not one giant scroll of every method.

## Method Pages

The sidebar lists methods alphabetically. You can also use the search box to jump directly to a method name.

## Accessors

`DataFrameTsAccessor` exposes nested accessors, each with its own method pages and sidebar:

- `ts().window(...)` -> [`DataFrameTsWindowAccessor`](dataframe_ts_window_accessor_api_reference.md)
- `ts().resample(...)` -> [`DataFrameTsResampler`](dataframe_ts_resampler_api_reference.md)

```{toctree}
:hidden:
:maxdepth: 1

dataframe_ts_window_accessor_api_reference
dataframe_ts_resampler_api_reference
api/dataframe-ts/add
api/dataframe-ts/align_to
api/dataframe-ts/asfreq
api/dataframe-ts/bounded
api/dataframe-ts/calendar
api/dataframe-ts/diff
api/dataframe-ts/div
api/dataframe-ts/eq
api/dataframe-ts/ewm
api/dataframe-ts/expanding
api/dataframe-ts/fill_gaps
api/dataframe-ts/ge
api/dataframe-ts/grouped
api/dataframe-ts/gt
api/dataframe-ts/insert_gaps
api/dataframe-ts/lag
api/dataframe-ts/lag_block
api/dataframe-ts/le
api/dataframe-ts/lead
api/dataframe-ts/log_returns
api/dataframe-ts/lt
api/dataframe-ts/mul
api/dataframe-ts/ne
api/dataframe-ts/resample
api/dataframe-ts/returns
api/dataframe-ts/rolling
api/dataframe-ts/rolling_block
api/dataframe-ts/sort
api/dataframe-ts/sub
api/dataframe-ts/timed
api/dataframe-ts/weigh
api/dataframe-ts/window
api/dataframe-ts/zscore_rolling
```
