<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9f1772b1ecaa -->

# Shelf::ts

[Back to Shelf API Reference](../../shelf_api_reference)

## Signature

```cpp
TimeSeriesAccessor Shelf::ts() const
```

## What It Does

Enters the dedicated time-series-*numerics* accessor for this shelf — period-over-
period returns, differencing, lag/lead, rolling and expanding windows, exponentially-
weighted statistics, gap filling, resampling, and axis-aligned arithmetic, one method
per operation. Unlike [`tl()`](tl.md) (which requires `Timeline`/`TimeIndex` payloads),
`ts()` requires the shelf to already be a numeric `Series` — cast it first with
[`astype()`](astype.md)/[`cast()`](cast.md) if it isn't.

Unlike [`DataFrame::ts()`](../dataframe/ts.md), this overload takes no options — there's
no `group`/`anchor` concept for a single shelf outside a frame, and duration-based
methods that need an explicit time axis (like the duration overload of `rolling()`)
simply take that axis as a direct argument instead.

This page previews the accessor's most common entry points. Every one of its methods
has its own dedicated, example-rich reference page — start at the
[TimeSeriesAccessor API Reference](../../shelf_ts_accessor_api_reference) and use its
left sidebar to jump straight to any method (`lag`, `diff`, `rolling`, `expanding`,
`ewm`, and more).

## Parameters

`None`

## Returns

`TimeSeriesAccessor`
    A lightweight accessor bound to this (numeric) shelf. Every method on it returns a
    new `Shelf` (or a `TimeSeriesResample`/`DataFrame` for the block/resample methods);
    the source shelf is never mutated.

## Examples

Example 1. Compute a lag and a period-over-period difference on a numeric shelf.

```cpp
using namespace pluss::table;

auto load = Shelf(pluss::make_series({10.0, 20.0, 15.0, 30.0}));

auto lagged = load.ts().lag(1);
std::cout << lagged;
```

### Output

```text
0  NULL
1  10
2  20
3  15

ptype: Series
```

`.lag(1)` is equivalent to [`shift(1)`](shift.md): each position now shows the previous
position's value, with position `0` becoming `NULL` since it has no predecessor.

```cpp
auto changes = load.ts().diff(1);
std::cout << changes;
```

### Output

```text
0  NULL
1  10
2  -5
3  15

ptype: Series
```

`.diff(1)` is `load[i] - load[i - 1]`: position `1` is `20 - 10 = 10`, position `2` is
`15 - 20 = -5`, position `3` is `30 - 15 = 15`; position `0` has no prior position to
subtract, so it's `NULL`.

Example 2. `ts()` on a non-numeric shelf throws before the accessor's methods can run.

```cpp
auto words = Shelf(pluss::make_note({"a", "b"}));
try {
  auto bad = words.ts().lag(1);
} catch (const pluss::Error& e) {
  std::cout << e.message();
}
```

### Output

```text
TimeSeriesAccessor::lag: source must be Series
```

The check happens lazily, on the first method call against the accessor — `ts()`
itself never throws, since it just binds the shelf without inspecting it.

## Notes

- The source shelf must already be a numeric `Series`; every accessor method throws
  otherwise (`ts()` itself doesn't validate eagerly).
- `.lag(periods)` on a numeric shelf behaves identically to [`shift(periods)`](shift.md)
  — both introduce `NULL` at the vacated end.
- No `options`/`TsOptions` parameter here, unlike [`DataFrame::ts()`](../dataframe/ts.md)
  — duration-aware methods take their time axis directly as an argument instead.

## See Also

- [`Shelf::shift`](shift.md): the lower-level positional shift `.ts().lag()` is built
  on, without the time-series-accessor framing.
- [`Shelf::tl`](tl.md): the equivalent dedicated accessor for calendar/timestamp
  components on `Timeline`/`TimeIndex` payloads, instead of numeric time-series values.
- [`Shelf::weigh`](weigh.md), [`Shelf::bounded`](bounded.md): lower-level windowing
  primitives that some of this accessor's fixed-width methods are built on top of.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="truediv.html">
    <span class="label">Previous</span>
    <span class="title">truediv</span>
  </a>
  <a class="next-link" href="unname.html">
    <span class="label">Next</span>
    <span class="title">unname</span>
  </a>
</div>
