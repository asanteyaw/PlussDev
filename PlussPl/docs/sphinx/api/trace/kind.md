<!-- pluss-hand-authored -->

# Trace::kind

[Back to Trace API Reference](../../trace_api_reference)

## Signatures

```cpp
TraceKind Trace::kind() const noexcept
```

## What It Does

Returns which of the 22 `TraceKind` values this trace was constructed with — the tag that
tells the renderer which geometry/payload interpretation to use for
[`payload()`](payload.md). There is no setter: a `Trace`'s kind is fixed for its lifetime,
set once at construction by whichever `Pane` trace-creation method built it (e.g.
[`Pane::trend`](../pane/trend.md) always produces `TraceKind::Trend`,
[`Pane::sheet`](../pane/sheet.md) always produces `TraceKind::Sheet`).

## Parameters

*(none)*

## Returns

`TraceKind`
    One of: `Trend`, `Cloud`, `Cloud3`, `Fieldmap`, `Ribbon`, `Spike`, `Wedge`, `Whisk`,
    `Band`, `Columns`, `Bucket`, `Quartet`, `Violin`, `Radial`, `Contour`, `Isoband`,
    `Ridge3`, `IsoShell`, `Path3`, `Spire3`, `Sheet`, `Lattice`.

## Examples

Example 1. Reading `kind()` back from traces created by different `Pane` methods.

```{important} Pane::traces_ is also a std::vector -- the same reference-invalidation rule applies
Pane stores its traces in a single std::vector<Trace> internally, exactly like
Studio stores its panes. A Trace& returned by any trace-creation method
(trend(), wedge(), etc.) is invalidated the moment ANOTHER trace-creation call
is made on the SAME Pane -- not just by pane-creation calls on the enclosing
Studio. Read whatever you need from a trace immediately after creating it,
before creating the next one on that pane. This bit the first draft of the
example below directly: reading a captured Trace& AFTER a second trend()/
wedge() call on the same pane produced a wrong kind() and, in a related example
on Trace::payload, a std::bad_variant_access from an invalidated reference.
```

```cpp
using namespace pluss::pl;

// A formula-generated axis, not a hand-typed literal -- `point_count` is the single
// source of truth for how many points the trace has.
constexpr std::int64_t point_count = 4;
auto x = pluss::series::linspace(0.0, point_count - 1.0, point_count, pluss::ScalarType::Float64);
auto y = pluss::series::linspace(2.0, 5.0, point_count, pluss::ScalarType::Float64);

auto fig = studio("trace_kind");
auto& pane2 = fig.pane2();
const Trace& trend_trace = pane2.trend(x, y);
const bool trend_is_trend = trend_trace.kind() == TraceKind::Trend;  // read back BEFORE the next trace-creation call
const Trace& wedge_trace = pane2.wedge(std::vector<double>{30.0, 70.0});
const bool wedge_is_wedge = wedge_trace.kind() == TraceKind::Wedge;
std::cout << "trend kind == TraceKind::Trend: " << trend_is_trend << "\n";
std::cout << "wedge kind == TraceKind::Wedge: " << wedge_is_wedge << "\n";
```

Output:

```text
trend kind == TraceKind::Trend: 1
wedge kind == TraceKind::Wedge: 1
```

## Notes

- `noexcept` — a plain field read.
- `columns()` and `columns_h()` both produce `TraceKind::Columns` — the horizontal/vertical
  distinction lives in `ColumnsData::horizontal`, not in a separate `TraceKind` value. The
  same sharing pattern applies to `isoband()`/`ridge3()` (both build a `ContourData`
  payload, distinguished by `ContourData::filled`/`elevated`) and to `sheet()`/`lattice()`
  (both build a `SurfaceData` payload, distinguished purely by which `TraceKind` tag is
  used — `SurfaceData` itself carries no wireframe/filled flag). See
  [`payload()`](payload.md) for the full payload-type-per-kind mapping.
- `TraceKind::Band` was observed in the enum definition but no `Pane` trace-creation
  method producing it was found among the public trace-creation methods documented in this
  reference — the `band_x`/`band_y`/`band_z` family instead produces `BandCue` objects
  stored separately from `Pane::traces()`, not `Trace` objects. Treat `TraceKind::Band` as
  present in the type but not reachable through the public trace-creation surface covered
  here.

## See Also

- [`Trace::payload`](payload.md): the actual geometry/data driving this trace, tagged by
  this same `kind()`.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="payload.html">
    <span class="label">Next</span>
    <span class="title">payload</span>
  </a>
</div>
