<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 925f3e7e016e -->

# Stream::plot

[Back to Stream API Reference](../../stream_api_reference)

## Signatures

```cpp
StreamPlotAccessor Stream::plot() const
StreamPlotAccessor Stream::plot(const pluss::table::DataFramePlotOptions& options) const
```

## What It Does

The entry point into the stream-level plotting accessor — `line`/`bar`/`scatter`/`hist`/
`density`/`auto_plot` charts of the stream's own values, returning `pluss::pl::Studio`
figures. Like `Catalog::plot()`, there is no separate `StreamPlotOptions` type — the
accessor reuses `pluss::table::DataFramePlotOptions` as-is. Every method on the returned
`StreamPlotAccessor` is itself a thin forward into
[`SlatePlotAccessor`](../../slate_plot_api_reference) (via `Stream::to_slate()`), so the
Stream-level pages focus on what's different at this layer — mainly that `Stream` has no
`name()`, so every chart's axis labels use the generic fallback names (`"value"`, `"x"`,
`"hue"`) rather than any name of your own. Every method has its own page, with real
rendered charts, under
[`StreamPlotAccessor` API Reference](../../stream_plot_accessor_api_reference).

## Parameters

**options** (`const pluss::table::DataFramePlotOptions&`, optional)
    Shared defaults applied to every chart method called on the returned accessor — most
    relevantly `default_bins` (read by [`hist()`](../stream-plot/hist.md)'s ambiguity
    handling).

## Returns

`StreamPlotAccessor`
    A binder for the current stream — call one of its methods to build a specific chart.

## Examples

Example 1. `plot()` binds without rendering; the returned accessor's methods do the actual
chart-building.

```cpp
using namespace pluss::kindle;

auto revenue = Stream(pluss::make_series({100.0, 120.0, 90.0, 150.0}));

auto chart = revenue.plot().line();
```

## Notes

- `plot()` alone does no work — it only binds the stream; call one of the returned
  accessor's methods to actually build a chart.
- Reuses `pluss::table::DataFramePlotOptions` directly — there is no `StreamPlotOptions`.
- Every method on the returned accessor is a subset of, and forwards to,
  [`SlatePlotAccessor`](../../slate_plot_api_reference)'s own methods — `Stream` exposes
  `line`/`bar`/`scatter`/`hist`/`density`/`auto_plot` only, not the full
  `step`/`area`/`box`/`violin`/`pie` set `SlatePlotAccessor` has.

## See Also

- [`StreamPlotAccessor` API Reference](../../stream_plot_accessor_api_reference): every
  chart method on the accessor this call returns, each with a real rendered example.
- [`SlatePlotAccessor`](../../slate_plot_api_reference): the accessor every
  `StreamPlotAccessor` method delegates to underneath.
- [`Stream::to_slate`](to_slate.md): the conversion every plotting method runs first.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nrecords.html">
    <span class="label">Previous</span>
    <span class="title">nrecords</span>
  </a>
  <a class="next-link" href="ptype.html">
    <span class="label">Next</span>
    <span class="title">ptype</span>
  </a>
</div>
