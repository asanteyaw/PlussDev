<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7da62ed497ae -->

# DataFrame::bucketize

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::bucketize(pluss::msharp::RangeView::Double bins, pluss::msharp::RangeView::Int labels = {}, bool right = true, bool include_lowest = true) const -> DataFrame
```

## What It Does

Replaces every numeric `Series` feature currently on the frame with integer bucket
codes, based on a strictly increasing list of bin edges — the classic "cut into
bands" transform used for grading scales, coarse histogram binning, or turning a
continuous value into a small number of discrete tiers before grouping or one-hot
encoding. `N + 1` edges define `N` buckets; every non-numeric feature on the current
frame passes through completely unchanged.

`bucketize()` carries no feature selector of its own: it always bucketizes every
numeric feature it finds on the current frame. Narrow to one or a few features first
with [`features()`](features.md) when only some columns should be bucketized —
`readings.features({"amount"}).bucketize({...})` — and recombine with
[`update()`](update.md) afterward if the untouched features need to survive alongside
the bucketed result.

A value that doesn't fall into any bucket — outside the full `[bins.front(),
bins.back()]` span, given the current `right`/`include_lowest` settings — becomes
`NULL` in the output rather than being clamped into the nearest bucket.

## Parameters

**bins** (`pluss::RangeView<double>`)

Strictly increasing bin edges. `N + 1` edges produce `N` buckets: edges `{0, 10, 20,
30}` define three buckets. Must contain at least two edges, none of them `NaN`.

**labels** (`pluss::RangeView<std::int64_t>`, default empty)

Explicit integer code for each bucket, in edge order. Leave empty to get the default
`0, 1, 2, ..., N-1`. If supplied, its length must equal the bucket count exactly
(`bins.size() - 1`).

**right** (`bool`, default `true`)

Which side of each interval is closed:

- `true`: every bucket is right-closed — `(edge[i], edge[i+1]]` — except optionally
  the first, governed by `include_lowest`.
- `false`: every bucket is left-closed and right-open — `[edge[i], edge[i+1])` — for
  *every* bucket, including the first; `include_lowest` has no effect in this mode.

**include_lowest** (`bool`, default `true`)

Only consulted when `right = true`: whether the very first bucket also accepts values
exactly equal to its lower edge (making it `[edge[0], edge[1]]` instead of
`(edge[0], edge[1]]`). Defaults to `true`, so — unlike some cut-style APIs elsewhere —
a value sitting exactly on the lowest edge is bucketed rather than dropped by default.

## Returns

`DataFrame`
    A new frame where every targeted feature has been replaced by its integer bucket
    codes (or `NULL` for out-of-range values); every other feature is unchanged.

## Examples

Example 1. Bucketize a single feature with default labels, including an out-of-range
value.

```cpp
using namespace pluss::table;

auto readings = DataFrame({
    {"amount", Thread(pluss::make_series({5.0, 15.0, 25.0, 35.0, 45.0, 100.0}))},
});
std::cout << readings;
```

### Output

```text
=============
     | amount
-------------
0    | 5     
1    | 15    
2    | 25    
3    | 35    
4    | 45    
5    | 100   
-------------
[6 cs x 1 feat]
```

```cpp
auto bucketed = readings.bucketize({0.0, 10.0, 20.0, 30.0, 40.0, 50.0});
std::cout << bucketed;
```

### Output

```text
=============
     | amount
-------------
0    | 0     
1    | 1     
2    | 2     
3    | 3     
4    | 4     
5    | NULL  
-------------
[6 cs x 1 feat]
```

Five buckets from six edges, default labels `0..4`. `100.0` falls outside the full
`[0, 50]` span covered by the bins, so it becomes `NULL` rather than clamping into
bucket `4`.

Example 2. Switch to left-closed intervals (`right = false`) with custom labels, to
see how a boundary value moves between buckets.

```cpp
auto readings2 = DataFrame({
    {"amount", Thread(pluss::make_series({0.0, 10.0, 20.0, 30.0}))},
});

auto bucketed2 = readings2.bucketize(
    {0.0, 10.0, 20.0, 30.0, 40.0}, {100, 200, 300, 400},
    /*right=*/false);
std::cout << bucketed2;
```

### Output

```text
=============
     | amount
-------------
0    | 100   
1    | 200   
2    | 300   
3    | 400   
-------------
[4 cs x 1 feat]
```

With `right = false`, every bucket is `[edge, next_edge)`: `10.0` lands in bucket
`[10, 20)` (label `200`), not in `[0, 10)`. Had `right` stayed at its default `true`,
that same `10.0` would instead have landed in the *first* bucket (`(0, 10]`, or
`[0, 10]` with `include_lowest`), one bucket earlier than shown here.

Example 3. Bucketize two numeric features together, leaving a third, non-numeric
feature untouched — narrow to the two target features with
[`features()`](features.md), then merge the bucketed pair back into the full frame
with [`update()`](update.md).

```cpp
auto orders = DataFrame({
    {"amount", Thread(pluss::make_series({5.0, 15.0, 25.0, 35.0}))},
    {"weight", Thread(pluss::make_series({8.0, 22.0, 12.0, 38.0}))},
    {"region", Thread(pluss::make_note({"north", "south", "north", "east"}))},
});

auto bucketed_pair = orders.features({"amount", "weight"}).bucketize({0.0, 10.0, 20.0, 30.0, 40.0});
std::cout << bucketed_pair;

auto bucketed3 = orders.update(bucketed_pair);
std::cout << bucketed3;
```

### Output (the narrowed, bucketed-only pair)

```text
=====================
     | amount  weight
---------------------
0    | 0       0     
1    | 1       2     
2    | 2       1     
3    | 3       3     
---------------------
[4 cs x 2 feat]
```

### Output (merged back into the full frame)

```text
==============================
     | amount  weight  region
------------------------------
0    | 0       0       north 
1    | 1       2       south 
2    | 2       1       north 
3    | 3       3       east  
------------------------------
[4 cs x 3 feat]
```

Both `amount` and `weight` are bucketed with the same edges and default labels,
landing in different buckets independently based on their own values —
`weight`'s `22.0` (bucket `2`) and `amount`'s `15.0` (bucket `1`) demonstrate that
each column is evaluated on its own values, not jointly. `region` was never part of
the narrowed frame `bucketize()` ran against, so [`update()`](update.md) leaves it
exactly as it was on `orders`.

Example 4. Pass `include_lowest = false` to see a value sitting exactly on the
lowest edge fall out of every bucket.

```cpp
auto edge_case = DataFrame({
    {"amount", Thread(pluss::make_series({0.0, 10.0, 25.0}))},
});

auto default_lowest = edge_case.bucketize({0.0, 10.0, 20.0, 30.0});
std::cout << default_lowest;

auto excluded_lowest = edge_case.bucketize(
    {0.0, 10.0, 20.0, 30.0}, {}, /*right=*/true, /*include_lowest=*/false);
std::cout << excluded_lowest;
```

### Output (include_lowest = true, the default)

```text
=============
     | amount
-------------
0    | 0     
1    | 0     
2    | 2     
-------------
[3 cs x 1 feat]
```

### Output (include_lowest = false)

```text
=============
     | amount
-------------
0    | NULL  
1    | 0     
2    | 2     
-------------
[3 cs x 1 feat]
```

With the default `include_lowest = true`, the very first bucket is
`[0, 10]` (closed on both ends), so `0.0` lands in bucket `0` alongside
`10.0`. With `include_lowest = false`, that first bucket shrinks to
`(0, 10]` — `0.0` no longer satisfies *any* bucket's lower bound, so it
becomes `NULL` instead. `10.0` and `25.0` are unaffected either way, since
`include_lowest` only ever changes the lowest bucket's lower edge.

Example 5. Pass bin edges that aren't strictly increasing, to see the call
throw.

```cpp
try {
  auto out = edge_case.bucketize({0.0, 20.0, 10.0, 30.0});
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Thread::bucketize: bin edges must be strictly increasing
```

`20.0` followed by `10.0` decreases, violating the strictly-increasing
requirement — checked up front for the whole edge list before any value is
bucketed.

## Notes

- Bin edges must be strictly increasing and non-`NaN`, checked once up front for the
  whole call — a single bad edge rejects the entire `bucketize()` call before any
  column is touched.
- Out-of-range values become `NULL`, never clamped into the nearest bucket — a value
  below the first edge or above the last edge (given the current `right` setting) is
  simply missing in the output.
- `bucketize()` silently limits itself to numeric `Series` features on the current
  frame; it never attempts to bucketize a `Note`, `Timeline`, or `TimeIndex` column.
- It has no feature selector of its own — narrow with [`features()`](features.md)
  first to bucketize only some features, and recombine with [`update()`](update.md)
  if the untouched features need to survive alongside the result (Example 3).
- Follow up with [`group`](group.md) on the bucketed feature (`.group().by({bucketed_feature}).count()`)
  to get a frequency table of how many cases fell into each band.

## See Also

- [`DataFrame::group`](group.md): tally how many cases landed in each bucket via
  `.group().by({...}).count()`.
- [`DataFrame::cast`](cast.md): change a feature's underlying dtype or payload kind
  outright, rather than binning its numeric values into a small set of codes.
- [`DataFrame::clip`](clip.md): cap values into a numeric range instead of assigning
  them a discrete bucket code.
- [`DataFrame::features`](features.md), [`DataFrame::update`](update.md): narrow to a
  feature subset before bucketizing, then merge the result back into the full frame.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bottom.html">
    <span class="label">Previous</span>
    <span class="title">bottom</span>
  </a>
  <a class="next-link" href="case_study.html">
    <span class="label">Next</span>
    <span class="title">case_study</span>
  </a>
</div>
