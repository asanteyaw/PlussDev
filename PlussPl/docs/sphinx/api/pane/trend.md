<!-- pluss-hand-authored -->

# Pane::trend

[Back to Pane API Reference](../../pane_api_reference)

## Signature

```cpp
template <class X, class Y>
Trace& Pane::trend(X&& x, Y&& y)
```

## What It Does

Adds a connected line trace to the pane — the lowest-level primitive behind
`flash::lineplot` and every other line-based `flash::` function. `x` and `y` accept
anything convertible to a `Series` (a `pluss::Series` directly, or any series-convertible
range like a `std::vector<double>`), converted internally via `detail::to_series()`.
Returns the newly created [`Trace&`](../trace/label.md), which every trace-creation
method shares — chain [`.label(...)`](../trace/label.md) or any other `Trace` styling
method directly off the return value.

```{important} Requires a Planar pane
`trend()` is 2D-only. Calling it on a `Volumetric` pane (created via
[`pane3()`](../studio/pane3.md)) throws `std::logic_error("trend requires a planar pane")` — a
different exception *type* (`std::logic_error`, not `std::invalid_argument`) than most of
the validation elsewhere in `Pane`. Every other 2D-only trace method documented in this
group throws the identical shape of message, substituting its own name — see Example 2.
```

## Parameters

**x**, **y** (series-convertible)
    The trace's coordinates. Must have matching lengths (enforced by the underlying
    series/rendering machinery, not by `trend()` itself).

## Returns

`Trace&`
    A reference to the newly created trace, for chaining trace-level styling calls (like
    [`label()`](../trace/label.md)).

## Examples

Example 1. A basic line trace.

```cpp
using namespace pluss::pl;

auto fig = studio("trend_basic");
auto& pane = fig.pane2();
auto& trace = pane.trend(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0}, std::vector<double>{2.0, 4.0, 3.0, 5.0, 6.0});
trace.label("trend");
fig.show();

std::cout << (trace.kind() == TraceKind::Trend) << "\n";
```

### Output

```text
1
```

![Output of Example 1](img/trend-1-basic.svg)

Example 2. Calling `trend()` on a 3D pane throws.

```cpp
using namespace pluss::pl;

auto fig = studio("trend_wrong_kind");
auto& pane = fig.pane3();

try {
  pane.trend(std::vector<double>{1.0, 2.0}, std::vector<double>{1.0, 2.0});
} catch (const std::logic_error& e) {
  std::cout << e.what() << "\n";
}
```

### Output

```text
trend requires a planar pane
```

## Notes

- Not `noexcept` — throws on a kind mismatch, and the underlying series conversion may
  throw for malformed input.
- `flash::lineplot` and its relatives are convenience wrappers around this exact method —
  reach for `trend()` directly only when working at the low-level `Pane` API rather than
  through `flash::`.
- Every trace method returns `Trace&`, letting per-trace styling
  ([`label()`](../trace/label.md), stroke color, etc.) chain immediately off the call.

## See Also

- [`Pane::cloud`](cloud.md): the scatter-point (unconnected) counterpart.
- [`Trace::label`](../trace/label.md): the most common thing chained off a trace-creation
  call.
- [`Pane::kind`](kind.md): what determines whether `trend()` succeeds or throws.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="axis_label_font_scale.html">
    <span class="label">Previous</span>
    <span class="title">axis_label_font_scale</span>
  </a>
  <a class="next-link" href="cloud.html">
    <span class="label">Next</span>
    <span class="title">cloud</span>
  </a>
</div>
