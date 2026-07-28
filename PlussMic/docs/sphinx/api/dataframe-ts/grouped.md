<!-- pluss-hand-authored -->

# DataFrameTsAccessor::grouped

[Back to DataFrameTsAccessor API Reference](../../dataframe_ts_accessor_api_reference)

## Signatures

```cpp
DataFrameTsAccessor::grouped(std::int64_t window, WindowStatistic statistic = WindowStatistic::Mean, const WindowOptions& options = {}) const -> DataFrame
DataFrameTsAccessor::grouped(std::string_view group_feature, std::int64_t window, WindowStatistic statistic = WindowStatistic::Mean, const WindowOptions& options = {}) const -> DataFrame
DataFrameTsAccessor::grouped(const Thread& groups, std::int64_t window, WindowStatistic statistic = WindowStatistic::Mean, const WindowOptions& options = {}) const -> DataFrame
```

## What It Does

The same fixed-size trailing rolling statistic as
[`rolling(window, statistic, options)`](rolling.md), but computed **independently
within each group**, using each group's own rows in their original relative order
(not resorted by group). A row's window never reaches across a group boundary.

- The **no-argument-group overload** (`grouped(window, statistic, options)`)
  requires `TsOptions.group` to already be set (via
  `df.features({...}).ts({.group = ...})`) and throws otherwise.
- The **`group_feature` overload** looks up an existing feature *by name* on the
  exact frame `.ts()` was called on — since `ts()` requires every feature on that
  frame to already be numeric, this only works if the group column is itself
  numeric (or if it wasn't dropped by an earlier `.features({...})` narrowing
  step). In practice, the **`groups`-`Thread` overload** below is the more
  reliable choice whenever the grouping key is non-numeric, since it lets you
  extract that key *before* narrowing to the numeric value feature(s).
- The **`groups` overload** takes an explicit `Thread` of grouping keys, one per
  case, matching the frame's case count, independent of whichever features are
  currently bound to the accessor.

## Parameters

**window** (`std::int64_t`)

The trailing window size, in rows, applied independently within each group.

**statistic** (`WindowStatistic`, default `Mean`)

The statistic computed over each window — see [`rolling()`](rolling.md) for the
full set.

**options** (`const WindowOptions&`)

Same fields as [`rolling()`](rolling.md)'s fixed-window overload: `min_count`,
`ddof`, `centered`, `closure`.

**group_feature** (`std::string_view`, one overload)

The name of an existing feature on the frame whose values define the groups.

**groups** (`const Thread&`, one overload)

An explicit per-case grouping key; must be defined and match the frame's case
count.

## Returns

`DataFrame`
    A new frame, same shape and case labels as the accessor's bound features, with
    each feature replaced by its grouped rolling statistic.

## Examples

Example 1. Compute a rolling mean scoped independently to each of two groups, using
the `groups`-`Thread` overload so the (non-numeric) group key can be extracted
before narrowing to the numeric value feature.

```cpp
using namespace pluss::table;

auto readings = DataFrame({
    {"group", Thread(pluss::make_note({"A", "A", "B", "B"}))},
    {"load",  Thread(pluss::make_series({10.0, 20.0, 15.0, 30.0}))},
});
std::cout << readings;
```

### Output (before)

```text
==================
     | group  load
------------------
0    | A      10  
1    | A      20  
2    | B      15  
3    | B      30  
------------------
[4 cs x 2 feat]
```

```cpp
auto groups = readings.features("group");
auto out = readings.features({"load"}).ts().grouped(groups, 2);
std::cout << out;
```

### Output (after)

```text
===========
     | load
-----------
0    | 10  
1    | 15  
2    | 15  
3    | 22.5
-----------
[4 cs x 1 feat]
```

Group `A`'s own subsequence is `[10, 20]`; its window-2 rolling mean is `[10, 15]`,
same as calling [`rolling(2)`](rolling.md) on that pair alone. Group `B`'s
subsequence `[15, 30]` similarly rolls to `[15, 22.5]`. The window for case `2`
(the first row of group `B`) never reaches back into group `A`'s `20` — grouping
resets the window at each group boundary. The output keeps only the accessor's
bound feature (`load`), replaced by its grouped rolling values; `group` itself
isn't part of the result.

Example 2. The `group_feature` overload, naming the grouping column instead of
extracting it into a `Thread` first.

```cpp
auto numeric_groups = DataFrame({
    {"group_id", Thread(pluss::make_series({1, 1, 2, 2}))},
    {"load",     Thread(pluss::make_series({10.0, 20.0, 15.0, 30.0}))},
});

auto out2 = numeric_groups.ts().grouped("group_id", 2);
std::cout << out2;
```

### Output

```text
=====================
     | group_id  load
---------------------
0    | 1         10  
1    | 1         15  
2    | 2         15  
3    | 2         22.5
---------------------
[4 cs x 2 feat]
```

`numeric_groups.ts()` (no `.features({...})` narrowing) auto-binds every
numeric column, including `group_id` itself, so it comes along in the output
unchanged, alongside `load`'s grouped rolling mean — the same values as
Example 1, just keyed by a numeric grouping feature named directly instead of
extracted as a `Thread` beforehand.

Example 3. Call the no-argument-group overload without `TsOptions.group` set,
to see it throw.

```cpp
try {
  auto out3 = readings.features({"load"}).ts().grouped(2);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
DataFrameTsAccessor::grouped: TsOptions.group is required. Use df.features(...).ts({.group = df.features("group")})
```

`readings.features({"load"}).ts()` was built with plain `.ts()`, no `.group`
option — the no-argument-group overload has no grouping key to fall back on,
so it throws instead of silently behaving like ungrouped [`rolling()`](rolling.md).

## Notes

- Rows are grouped by matching key value, using each group's original row order —
  `grouped()` does not sort or otherwise reorder cases.
- Every group's rolling computation uses the exact same
  [`rolling()`](rolling.md)-style partial-window and `min_count`/`ddof` semantics,
  just scoped to that group's own rows.
- The no-argument-group overload is the one to reach for when `TsOptions.group` was
  already set once (via `.ts({.group = ...})`) and several grouped operations will
  reuse it without repeating the group argument each time.

## See Also

- [`DataFrameTsAccessor::rolling`](rolling.md): the ungrouped fixed-window
  statistic this method scopes per group.
- [`DataFrame::group`](../dataframe/group.md): general-purpose grouping and
  aggregation, for reductions rather than row-aligned rolling statistics.

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
