<!-- pluss-hand-authored -->

# Trace::payload

[Back to Trace API Reference](../../trace_api_reference)

## Signatures

```cpp
const TracePayload& Trace::payload() const noexcept
```

## What It Does

Returns the trace's underlying geometry/data — a `std::variant` holding whichever
`*Data` struct matches its [`kind()`](kind.md). There is no setter: like `kind()`, the
payload is fixed at construction and immutable afterward. Inspecting it requires
`std::visit` or `std::get<T>()` against `TracePayload`'s alternative types.

```{important} Payload types map many-to-one from TraceKind
`TracePayload` has 18 alternative types but `TraceKind` has 22 values — several kinds
share one payload struct, distinguished by a flag inside that struct rather than by a
separate type. See the table below.
```

## Parameters

*(none)*

## Returns

`const TracePayload&`
    A `std::variant<TrendData, CloudData, Cloud3Data, FieldmapData, RibbonData, SpikeData,
    WedgeData, WhiskData, ColumnsData, BucketData, QuartetData, ViolinData, RadialData,
    ContourData, IsoShellData, Path3Data, Spire3Data, SurfaceData>`. The active
    alternative always corresponds to [`kind()`](kind.md):

    | `kind()`                    | Active alternative | Distinguishing field           |
    |------------------------------|---------------------|---------------------------------|
    | `Trend`                      | `TrendData`         | —                                |
    | `Cloud`                       | `CloudData`          | —                                |
    | `Cloud3`                      | `Cloud3Data`         | —                                |
    | `Fieldmap`                    | `FieldmapData`       | —                                |
    | `Ribbon`                      | `RibbonData`         | —                                |
    | `Spike`                       | `SpikeData`          | —                                |
    | `Wedge`                       | `WedgeData`          | —                                |
    | `Whisk`                       | `WhiskData`          | —                                |
    | `Columns` (from `columns()` or `columns_h()`) | `ColumnsData` | `ColumnsData::horizontal` |
    | `Bucket`                      | `BucketData`         | —                                |
    | `Quartet`                     | `QuartetData`        | —                                |
    | `Violin`                      | `ViolinData`         | —                                |
    | `Radial`                      | `RadialData`         | —                                |
    | `Contour`, `Isoband`, `Ridge3` | `ContourData`       | `ContourData::filled`/`elevated` |
    | `IsoShell`                    | `IsoShellData`       | —                                |
    | `Path3`                       | `Path3Data`          | —                                |
    | `Spire3`                      | `Spire3Data`         | —                                |
    | `Sheet`, `Lattice`            | `SurfaceData`        | (none — the `TraceKind` tag alone selects wireframe vs. filled rendering) |

## Examples

Example 1. Extracting the payload with `std::get`, guided by `kind()`.

```cpp
using namespace pluss::pl;

// A formula-generated axis -- `point_count` is the single source of truth for the
// trace's length, rather than a hand-typed literal vector repeated at each call site.
constexpr std::int64_t point_count = 4;
auto x = pluss::series::linspace(0.0, point_count - 1.0, point_count, pluss::ScalarType::Float64);
auto y = pluss::series::linspace(10.0, 25.0, point_count, pluss::ScalarType::Float64);

auto fig = studio("trace_payload_get");
auto& pane = fig.pane2();
const Trace& t = pane.trend(x, y);
if (t.kind() == TraceKind::Trend) {
  const auto& data = std::get<TrendData>(t.payload());
  std::cout << "x.numel()=" << data.x.numel() << " y.numel()=" << data.y.numel() << "\n";
}
```

Output:

```text
x.numel()=4 y.numel()=4
```

Example 2. Distinguishing `columns()` from `columns_h()` via the shared `ColumnsData`
payload's `horizontal` field, even though both report `TraceKind::Columns`. Per
[`Trace::kind`](kind.md)'s reference-invalidation callout, each trace's fields are read
immediately after that trace's own creation, before the next `add_trace_` call on the
same pane invalidates it.

```cpp
using namespace pluss::pl;

auto fig = studio("trace_payload_columns_shared");
auto& pane = fig.pane2();
const Trace& vertical = pane.columns(std::vector<double>{3.0, 5.0, 2.0});
const bool vertical_is_columns = vertical.kind() == TraceKind::Columns;
const bool vertical_horizontal = std::get<ColumnsData>(vertical.payload()).horizontal;
const Trace& horizontal = pane.columns_h(std::vector<double>{3.0, 5.0, 2.0});
const bool horizontal_is_columns = horizontal.kind() == TraceKind::Columns;
const bool horizontal_horizontal = std::get<ColumnsData>(horizontal.payload()).horizontal;
std::cout << "both TraceKind::Columns: " << (vertical_is_columns && horizontal_is_columns) << "\n";
std::cout << "vertical.horizontal: " << vertical_horizontal << "\n";
std::cout << "horizontal.horizontal: " << horizontal_horizontal << "\n";
```

Output:

```text
both TraceKind::Columns: 1
vertical.horizontal: 0
horizontal.horizontal: 1
```

## Notes

- `noexcept` — a plain field read; the `std::variant` was already fully constructed when
  the `Trace` was built.
- `std::get<T>()` with the wrong `T` throws `std::bad_variant_access` — always check
  [`kind()`](kind.md) first, or use `std::visit` with a kind-agnostic visitor instead of
  guessing the alternative.
- The payload structs themselves (`TrendData`, `CloudData`, etc.) are documented alongside
  each `Pane` trace-creation method that builds them (e.g. `WhiskData`'s field layout is
  covered on [`Pane::whisk`](../pane/whisk.md)) rather than repeated here.

## See Also

- [`Trace::kind`](kind.md): the tag selecting which payload alternative is active.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="kind.html">
    <span class="label">Previous</span>
    <span class="title">kind</span>
  </a>
  <a class="next-link" href="label.html">
    <span class="label">Next</span>
    <span class="title">label</span>
  </a>
</div>
