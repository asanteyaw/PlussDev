<!-- pluss-hand-authored -->

# Thread::bucketize

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::bucketize(pluss::RangeView<double> bins, pluss::RangeView<std::int64_t> labels = {}, bool right = true, bool include_lowest = true) const -> Thread
```

## What It Does

Converts a numeric thread into interval labels defined by explicit bin edges —
the one-dimensional building block behind histogram-style feature engineering,
grading bands, and coarse segmentation. `bins` must be strictly increasing and
supply `N + 1` edges to define `N` buckets; each value is assigned the index
(or explicit label) of the bucket whose interval contains it. A value falling
outside every bucket's interval (below the first edge, or above the last, given
the `right`/`include_lowest` closure rules) becomes missing in the result,
rather than clamping into the nearest bucket or throwing.

`right` controls interval closure: `right = true` (the default) makes each
interval `(edge[i], edge[i+1]]` — closed on the right, open on the left — while
`right = false` makes it `[edge[i], edge[i+1])`. `include_lowest` (only
relevant when `right = true`) additionally admits the exact lower bound of the
*first* interval, so it behaves as `[edge[0], edge[1]]` instead of `(edge[0],
edge[1]]` for that one interval only.

## Parameters

**bins** (`pluss::RangeView<double>`)
    Strictly increasing interval edges — `N + 1` edges define `N` buckets. Must
    contain at least two edges, none of them `NaN`, or it throws.

**labels** (`pluss::RangeView<std::int64_t>`, optional)
    Explicit bucket labels, one per bucket. Left empty (the default) to use
    `0, 1, 2, ...` as labels. If given, its length must equal `bins.size() - 1`,
    or it throws.

**right** (`bool`, default `true`)
    `true`: each interval is right-closed, left-open (`(a, b]`). `false`:
    left-closed, right-open (`[a, b)`).

**include_lowest** (`bool`, default `true`)
    When `right = true`, additionally admits the exact lower bound of the
    *first* interval only. Has no additional effect when `right = false`,
    since the first interval is already left-closed in that case.

## Returns

`Thread`
    A numeric thread of bucket labels, the same length as the source, with a
    missing entry wherever the source value falls outside every bucket's
    interval.

## Examples

Example 1. Bucketize with default (`0, 1, 2, ...`) output labels.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));
auto out = scores.bucketize({0.0, 20.0, 40.0, 60.0});
std::cout << out;
```

### Output

```text
0  0
1  0
2  1
3  1
4  2

ptype: Series
```

Bucket `0` covers `[0, 20]` (`include_lowest` admits `0` itself, and `right`
closes at `20`), so both `10.0` and `20.0` land there. Bucket `1` covers `(20,
40]`, catching `30.0` and `40.0`. Bucket `2` covers `(40, 60]`, catching `50.0`.

Example 2. Bucketize with explicit output labels.

```cpp
auto out2 = scores.bucketize({0.0, 20.0, 40.0, 60.0}, {10, 20, 30}, true, true);
std::cout << out2;
```

### Output

```text
0  10
1  10
2  20
3  20
4  30

ptype: Series
```

Same bucket assignment as Example 1, just relabeled with the explicit codes
`10`/`20`/`30` instead of the default `0`/`1`/`2`.

Example 3. A value outside every bucket's interval becomes missing.

```cpp
auto with_outlier = Thread(pluss::make_series({-5.0, 10.0, 70.0}));
auto out3 = with_outlier.bucketize({0.0, 20.0, 40.0, 60.0});
std::cout << out3;
```

### Output

```text
0  NULL
1  0
2  NULL

ptype: Series
```

`-5.0` is below the first edge and `70.0` is above the last — both fall outside
every bucket's interval and become missing, rather than clamping into the
nearest bucket.

## Notes

- `N + 1` bin edges are required to produce `N` buckets.
- Bin edges must be strictly increasing and non-`NaN`, or it throws
  `"Thread::bucketize: bin edges must be strictly increasing"` /
  `"Thread::bucketize: bin edges must not be NaN"`.
- A value outside every interval becomes missing in the result — it is never
  clamped into the nearest bucket.
- Restricted to numeric `Series` payloads; any other kind throws
  `"Thread::bucketize is only valid for numeric Series payload"`.
- Explicit `labels`, when given, must have exactly `bins.size() - 1` entries,
  or it throws `"Thread::bucketize: labels size must equal bins.size() - 1"`.

## See Also

- [`DataFrame::bucketize`](../dataframe/bucketize.md): bucketize selected
  numeric frame features while preserving the rest of the table.
- [`Thread::btw`](btw.md): test interval membership directly, without
  collapsing values into bucket labels.
- [`Thread::clip`](clip.md): constrain values into a range instead of
  labeling which sub-interval they fall in.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="btw.html">
    <span class="label">Previous</span>
    <span class="title">btw</span>
  </a>
  <a class="next-link" href="casefold.html">
    <span class="label">Next</span>
    <span class="title">casefold</span>
  </a>
</div>
