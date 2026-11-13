<!-- pluss-hand-authored -->

# TimeSeriesAccessor::grouped

[Back to TimeSeriesAccessor API Reference](../../slate_ts_accessor_api_reference)

## Signature

```cpp
Slate TimeSeriesAccessor::grouped(const Slate& groups, std::int64_t window, WindowStatistic statistic = WindowStatistic::Mean, const WindowOptions& options = {}) const
```

## What It Does

Computes the same fixed-row-count rolling window [`rolling()`](rolling.md)
does, but **restarted independently within each group** — positions are
partitioned by `groups` (compared by value, any slate kind), and within each
group the rolling window only ever sees that group's own positions, in their
original relative order, never crossing a group boundary.

Every position whose `groups` entry is **missing** is treated as belonging to
one single, shared "ungrouped" bucket — not its own separate group, and not
excluded from the result. That bucket's positions are windowed together, in
their original relative order, even though they may be far apart (and separated
by other groups' positions) in the source slate. See Example 2.

## Parameters

**groups** (`const Slate&`)
    The grouping key, one entry per position, same length as the bound slate.
    Any slate kind; compared by value.

**window** (`std::int64_t`)
    The trailing window size, in positions, applied independently within each
    group.

**statistic** (`WindowStatistic`, default `Mean`)
    The statistic computed over each window — same set `rolling()` supports.

**options** (`const WindowOptions&`)
    Same meaning as [`rolling()`](rolling.md)'s fixed-window overload
    (`min_count`, `ddof`, `centered`, `closure`).

## Returns

`Slate`
    A new slate, same length as the bound source, holding the per-group
    rolling statistic at each position.

## Examples

Example 1. Two groups, each windowed independently — the window restarts at
each group's own first position, not the slate's.

```cpp
using namespace pluss::table;

auto values = Slate(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0, 60.0}));
auto groups = Slate(pluss::make_note({"A", "A", "B", "B", "A", "B"}));

auto out = values.ts().grouped(groups, 2);
std::cout << out;
```

### Output

```text
0  10
1  15
2  30
3  35
4  35
5  50

ptype: Series
```

Group `"A"` is positions `0, 1, 4` (values `10, 20, 50`, in that relative
order): its own rolling mean(2) is `10`, `mean(10, 20) = 15`,
`mean(20, 50) = 35` — landing back at positions `0`, `1`, and `4` respectively.
Group `"B"` is positions `2, 3, 5` (values `30, 40, 60`): its rolling mean(2) is
`30`, `mean(30, 40) = 35`, `mean(40, 60) = 50` — landing at positions `2`, `3`,
and `5`. Position `4`'s window never sees position `2` or `3`'s values, even
though they sit between position `1` and position `4` in the source slate —
they belong to a different group.

Example 2. Positions with a missing group key share one common "ungrouped"
bucket, windowed together in their original relative order even when other
groups' positions sit between them.

```cpp
auto values2 = Slate(pluss::make_series({10.0, 20.0, 30.0, 40.0}));
auto groups2 = Slate(
    pluss::make_note({"A", "x", "B", "y"}),
    pluss::make_series(std::vector<bool>{true, false, true, false})
);

auto out2 = values2.ts().grouped(groups2, 2);
std::cout << out2;
```

### Output

```text
0  10
1  20
2  30
3  30

ptype: Series
```

Positions `1` and `3` both have a missing group key (their underlying text,
`"x"`/`"y"`, is irrelevant — a missing key is a missing key). They fall into the
same shared bucket, windowed as if adjacent: `mean(20, 40) = 30` lands at
position `3`, using position `1`'s value even though position `2` (a real,
different group) sits between them in the source slate.

Example 3. Mismatched `groups` length throws.

```cpp
auto short_groups = Slate(pluss::make_note({"A", "B"}));
try {
  auto bad = values.ts().grouped(short_groups, 2);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slate::grouped: group length mismatch
```

## Notes

- Groups are compared by value (via each group's own display/string form), not
  identity — any slate kind works as `groups`.
- Every position with a missing `groups` entry shares one common bucket,
  windowed together in original relative order — it is not excluded from the
  result, and it is not treated as its own separate group per distinct missing
  cause. See Example 2.
- `groups` must be the same length as the bound slate, or it throws
  `"Slate::grouped: group length mismatch"`.
- A pre-existing missing entry in the *values* slate (as opposed to `groups`)
  is skipped within any window that contains it, the same as
  [`rolling()`](rolling.md).
- Restricted to numeric `Series` payloads for the values slate; anything else
  throws `"Slate::grouped: expected a real numeric Series Slate"`.

## See Also

- [`TimeSeriesAccessor::rolling`](rolling.md): the same fixed-window engine,
  applied to the whole slate instead of independently per group.
- [`TimeSeriesAccessor::expanding`](expanding.md): a growing (not fixed-width)
  window, applied to the whole slate.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ge.html">
    <span class="label">Previous</span>
    <span class="title">ge</span>
  </a>
  <a class="next-link" href="gt.html">
    <span class="label">Next</span>
    <span class="title">gt</span>
  </a>
</div>
