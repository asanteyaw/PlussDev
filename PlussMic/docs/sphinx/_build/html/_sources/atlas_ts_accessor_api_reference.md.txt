# AtlasTsAccessor API Reference

This page documents the public time-series accessor returned by `Atlas::ts()`, applied frame-by-frame across the stack.

## Using This Reference

- Use the left sidebar to open one method page at a time.
- Each method page contains its own signatures, parameter details, notes, verified outputs, and working previous/next navigation.
- This landing page stays intentionally short so the content panel is not one giant scroll of every method.

## Method Pages

The sidebar lists methods alphabetically. You can also use the search box to jump directly to a method name.

## Accessors

`AtlasTsAccessor` exposes nested accessors, each with its own method pages and sidebar:

- `ts().resample(...)` -> [`AtlasTsResampler`](atlas_ts_resampler_api_reference.md)

```{toctree}
:hidden:
:maxdepth: 1

atlas_ts_resampler_api_reference
api/atlas-ts/asfreq
api/atlas-ts/diff
api/atlas-ts/ewm
api/atlas-ts/expanding
api/atlas-ts/fill_gaps
api/atlas-ts/lag
api/atlas-ts/lead
api/atlas-ts/log_returns
api/atlas-ts/resample
api/atlas-ts/returns
api/atlas-ts/rolling
api/atlas-ts/sort
```
