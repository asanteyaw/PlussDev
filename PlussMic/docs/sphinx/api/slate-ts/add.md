<!-- pluss-hand-authored -->

# TimeSeriesAccessor::add

[Back to TimeSeriesAccessor API Reference](../../slate_ts_accessor_api_reference)

## Signature

```cpp
Slate TimeSeriesAccessor::add(const Slate& other, const Slate& left_axis, const Slate& right_axis, TsAlignMode mode = TsAlignMode::Exact) const
```

## What It Does

Adds `other` to the bound slate **after first reindexing `other` onto the
bound slate's own positions**, using `left_axis` (the bound slate's time
axis) and `right_axis` (`other`'s own time axis) to figure out the
correspondence — exactly the same alignment [`align_to()`](align_to.md)
performs, run automatically before an ordinary [`Slate::add`](../slate/add.md).
Use this when two slates live on *different* time axes and you want them
added position-by-position after lining up by time, rather than by raw
position.

## Parameters

**other** (`const Slate&`)
    The slate to add, currently living at `right_axis` ticks.

**left_axis** (`const Slate&`)
    The bound slate's own time axis (`Timeline`/`TimeIndex`), same length as
    the bound slate.

**right_axis** (`const Slate&`)
    `other`'s time axis (`Timeline`/`TimeIndex`), same length as `other`.

**mode** (`TsAlignMode`, default `Exact`)
    The alignment strategy — same meaning as [`align_to()`](align_to.md)'s
    `mode`: `Exact`, `Backward`, `Forward`, or `Nearest`.

## Returns

`Slate`
    A new slate, same length as the bound slate, holding
    `bound + aligned(other)` at each position.

## Examples

Example 1. Two slates on different (non-overlapping) daily axes, added after
exact alignment.

```cpp
using namespace pluss::table;

auto left_axis = Slate(pluss::make_timeline(
    {"2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-03T00:00:00Z"}));
auto left = Slate(pluss::make_series({10.0, 20.0, 30.0}));

auto right_axis = Slate(pluss::make_timeline(
    {"2024-01-02T00:00:00Z", "2024-01-04T00:00:00Z"}));
auto right = Slate(pluss::make_series({100.0, 200.0}));

auto out = left.ts().add(right, left_axis, right_axis);
std::cout << out;
```

### Output

```text
0  NULL
1  120
2  NULL

ptype: Series
```

Only `2024-01-02` has an exact tick match on both sides (`20 + 100 = 120`);
`2024-01-01` and `2024-01-03` have no matching tick in `right_axis` at all
under `Exact` alignment, so `other`'s contribution is `NULL` there, and adding
`NULL` to anything is `NULL`.

Example 2. The same slates, aligned with `Backward` instead — `other`'s most
recent known value carries forward.

```cpp
auto out2 = left.ts().add(right, left_axis, right_axis, TsAlignMode::Backward);
std::cout << out2;
```

### Output

```text
0  NULL
1  120
2  130

ptype: Series
```

`2024-01-03` now picks up `right`'s most recent value at or before it
(`2024-01-02`'s `100`), so `30 + 100 = 130` instead of `NULL`. `2024-01-01`
still has nothing before it in `right_axis` at all, so it stays `NULL`.

Example 3. A `right_axis` whose length doesn't match `other` throws.

```cpp
auto short_axis = Slate(pluss::make_timeline({"2024-01-02T00:00:00Z"}));
try {
  auto bad = left.ts().add(right, left_axis, short_axis);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
TimeSeriesAccessor::add: right values and right time axis length mismatch
```

## Notes

- `left_axis` must be the same length as the bound slate, and `right_axis`
  must be the same length as `other`, or it throws
  `"TimeSeriesAccessor::add: left values and left time axis length mismatch"` /
  `"TimeSeriesAccessor::add: right values and right time axis length mismatch"`
  respectively.
- The alignment step is identical to [`align_to()`](align_to.md) — see its
  reference for the exact `Exact`/`Backward`/`Forward`/`Nearest` matching
  rules.
- The addition itself, once `other` is aligned, is an ordinary
  [`Slate::add`](../slate/add.md) — a `NULL` on either side produces `NULL`.

## See Also

- [`TimeSeriesAccessor::align_to`](align_to.md): the alignment step this
  method runs automatically before adding.
- [`Slate::add`](../slate/add.md): the plain positional addition this method
  performs after alignment.
- [`TimeSeriesAccessor::sub`](sub.md), [`TimeSeriesAccessor::mul`](mul.md),
  [`TimeSeriesAccessor::div`](div.md): the same align-then-combine pattern for
  the other arithmetic operators.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="align_to.html">
    <span class="label">Next</span>
    <span class="title">align_to</span>
  </a>
</div>
