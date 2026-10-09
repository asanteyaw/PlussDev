<!-- pluss-hand-authored -->

# TimeSeriesAccessor::gt

[Back to TimeSeriesAccessor API Reference](../../slate_ts_accessor_api_reference)

## Signature

```cpp
TimeSeriesAccessor::gt(const Slate& other, const Slate& left_axis, const Slate& right_axis, TsAlignMode mode = TsAlignMode::Exact) const -> Slate
```

## What It Does

Compares the bound slate against `other` with `>` **after first reindexing
`other` onto the bound slate's own positions**, using `left_axis` (the bound
slate's time axis) and `right_axis` (`other`'s own time axis) — exactly the
same alignment [`align_to()`](align_to.md) performs, run automatically before
an ordinary [`Slate::gt`](../slate/gt.md). See [`add()`](add.md) for the
shared alignment mechanics and [`eq()`](eq.md) for how a missing aligned
position behaves.

## Parameters

**other** (`const Slate&`)
    The slate to compare against, currently living at `right_axis` ticks.

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
    A new `Bool`-typed slate, same length as the bound slate, holding
    `bound > aligned(other)` at each position.

## Examples

Example 1. Two slates on different daily axes, compared after exact
alignment.

```cpp
using namespace pluss::table;

auto left_axis = Slate(pluss::make_timeline(
    {"2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-03T00:00:00Z"}));
auto left = Slate(pluss::make_series({10.0, 20.0, 30.0}));

auto right_axis = Slate(pluss::make_timeline(
    {"2024-01-02T00:00:00Z", "2024-01-04T00:00:00Z"}));
auto right = Slate(pluss::make_series({15.0, 200.0}));

auto out = left.ts().gt(right, left_axis, right_axis);
std::cout << out;
```

### Output

```text
0  NULL
1  true
2  NULL

ptype: Series
```

Only `2024-01-02` has an exact tick match on both sides (`20 > 15` is `true`);
the other positions have no matching `right_axis` tick under `Exact`, so the
comparison itself is `NULL`.

Example 2. The same slates, aligned with `Backward` — `other`'s most recent
known value carries forward, giving `2024-01-03` a real comparison instead of
`NULL`.

```cpp
auto out2 = left.ts().gt(right, left_axis, right_axis, TsAlignMode::Backward);
std::cout << out2;
```

### Output

```text
0  NULL
1  true
2  true

ptype: Series
```

`2024-01-03` now compares against `right`'s most recent value at or before it
(`2024-01-02`'s `15`): `30 > 15` is `true`.

## Notes

- `left_axis` must be the same length as the bound slate, and `right_axis`
  must be the same length as `other`, or it throws a length-mismatch error
  naming whichever side is wrong.
- The alignment step is identical to [`align_to()`](align_to.md).
- An unmatched position's comparison is `NULL`, not `false` — see
  [`eq()`](eq.md)'s Notes for the shared reasoning.

## See Also

- [`TimeSeriesAccessor::lt`](lt.md): the mirror-image comparison.
- [`TimeSeriesAccessor::ge`](ge.md): the inclusive counterpart.
- [`Slate::gt`](../slate/gt.md): the plain positional comparison this method
  performs after alignment.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="grouped.html">
    <span class="label">Previous</span>
    <span class="title">grouped</span>
  </a>
  <a class="next-link" href="lag.html">
    <span class="label">Next</span>
    <span class="title">lag</span>
  </a>
</div>
