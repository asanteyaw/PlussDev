<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 525f03cffa04 -->

# SlatePlotAccessor::line

[Back to SlatePlotAccessor API Reference](../../slate_plot_api_reference)

## Signatures

```cpp
pluss::pl::Studio SlatePlotAccessor::line() const
pluss::pl::Studio SlatePlotAccessor::line(const Slate& x) const
pluss::pl::Studio SlatePlotAccessor::line(const LinePlotOptions& options) const
pluss::pl::Studio SlatePlotAccessor::line(const Slate& x, const LinePlotOptions& options) const
```

## What It Does

Draws a single line trace of the bound slate's own values, against either a plain case
index (no `x` given) or an explicit companion `Slate` used as the x-axis. This is the
single-series specialization of
[`DataFramePlotAccessor::line`](../../api/dataframe-plot/line.md): internally, `line()`
wraps the bound slate (and, if given, `x`) into a one- or two-column `DataFrame` and calls
straight into that method — every field of `LinePlotOptions` behaves identically to its 2D
counterpart, and this page's callouts on naming and row-label handling apply to every other
method on this accessor too, not just `line()`.

```{important} How a wrapped Slate becomes a named column and inherits row labels
Every method on this accessor that takes one or more `Slate` arguments builds a small
internal `DataFrame` before delegating. Each slate becomes one column, named after that
slate's own `name()` if it has one (set via `name(...)`, checked via `named()`); otherwise a
generic fallback name is used — `"value"` for the bound slate itself, `"x"` for a
companion x-axis slate (other methods on this accessor use `"hue"`, `"label"`, or
`"group"` for their own second-slate arguments, documented on their own pages). Row
(case) labels for the resulting frame come from whichever slate involved actually has
them: the bound value slate is checked first, then the companion slate — the first one
found wins, and if neither has row labels the frame falls back to a plain numeric case
index, exactly like a `DataFrame` built without explicit row labels.
```

## Parameters

**x** (`const Slate&`, optional)
    A companion slate used as the x-axis. Must be the same size as the bound slate — a
    mismatched size throws `"Slate::plot: slate size mismatch"`. Omitted means the
    x-axis falls back to the resulting frame's own row labels (or a plain case index — see
    the important callout above), exactly like
    [`DataFramePlotAccessor::line`](../../api/dataframe-plot/line.md) with no `x_feature`
    and no `default_x_feature` set.

**options** (`LinePlotOptions`)
    The identical struct documented on
    [`DataFramePlotAccessor::line`](../../api/dataframe-plot/line.md)
    (`alpha`, `line_width`, `y_limit_method`, `align`) — see that page for each field's
    meaning. `align` is doubly inert at this layer: `x`'s own size check above
    (`"Slate::plot: slate size mismatch"`) throws unconditionally on a mismatch, before
    any `align`-aware reconciliation could ever run.

## Returns

`pluss::pl::Studio`
    A figure with one pane containing a single, unlabeled (no-legend) line trace of the
    bound slate's values.

## Examples

Example 1. No companion slate: the x-axis falls back to a plain case index.

```cpp
using namespace pluss::table;

auto revenue = Slate(pluss::make_series({100.0, 120.0, 90.0, 150.0}));
revenue = revenue.name("revenue");

auto chart = revenue.plot().line();
chart.show();
```

`revenue` has no row labels of its own, so the wrapped one-column frame falls back to a
plain `0..3` case index; the y-axis is labeled `"revenue"`, taken from the slate's own
`name()`.

![Output of Example 1](img/line-1.svg)

Example 2. An explicit companion slate as the x-axis.

```cpp
auto month = Slate(pluss::make_note({"Jan", "Feb", "Mar", "Apr"}));
month = month.name("month");

auto chart = revenue.plot().line(month);
chart.show();
```

The wrapped frame now has two columns, `"month"` and `"revenue"` (each named from its own
slate's `name()`), and the line is drawn against `month`'s four categorical
positions instead of a numeric case index.

![Output of Example 2](img/line-2.svg)

Example 3. An explicit `LinePlotOptions`, tuning opacity and the y-axis fit.

```cpp
LinePlotOptions options;
options.alpha = 0.85;
options.y_limit_method = pluss::pl::LimitMethod::Tight;

auto chart = revenue.plot().line(month, options);
chart.show();
```

![Output of Example 3](img/line-3.svg)

Example 4. Row labels are inherited from whichever operand actually has them — here, from
`month` since the bound `revenue` slate itself has none.

```cpp
auto labeled_revenue = Slate(pluss::make_series({100.0, 120.0, 90.0, 150.0}))
                            .label().set(Slate(pluss::make_note({"Q1", "Q2", "Q3", "Q4"})));

auto chart = labeled_revenue.plot().line();
chart.show();
```

`labeled_revenue` carries its own row labels (`"Q1"`..`"Q4"`), so the wrapped frame's case
axis uses those directly, even with no companion `x` slate supplied.

![Output of Example 4](img/line-4.svg)

## Notes

- Wraps the bound slate (and any companion slate) into a small internal `DataFrame` and
  delegates to [`DataFramePlotAccessor::line`](../../api/dataframe-plot/line.md) — see the
  important callout above for exactly how naming and row-label inheritance work.
- A companion `x` slate must match the bound slate's size, or the call throws.
- `LinePlotOptions` behaves identically to its 2D counterpart.
- Returns a `pluss::pl::Studio`, never printed as text — see
  [`DataFramePlotAccessor::line`](../../api/dataframe-plot/line.md)'s important callout for
  how to view or save the result.

## See Also

- [`DataFramePlotAccessor::line`](../../api/dataframe-plot/line.md): the method this
  delegates to, and the canonical explanation of `Studio`'s view/save mechanics.
- [`SlatePlotAccessor::step`](step.md) / [`area`](area.md): the same shape, drawn as a
  step function or a filled area instead of a straight line.
- [`SlatePlotAccessor::scatter`](scatter.md): unconnected points against a required
  companion slate, instead of a connected line against an optional one.
- [`Slate::plot`](../slate/plot.md): how to enter this accessor in the first place.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lag.html">
    <span class="label">Previous</span>
    <span class="title">lag</span>
  </a>
  <a class="next-link" href="pie.html">
    <span class="label">Next</span>
    <span class="title">pie</span>
  </a>
</div>
