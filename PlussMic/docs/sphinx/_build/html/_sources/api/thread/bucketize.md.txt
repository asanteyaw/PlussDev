# Thread::bucketize

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::bucketize(pluss::RangeView<double> bins, pluss::RangeView<std::int64_t> labels = {}, bool right = true, bool include_lowest = true) -> Thread
```

Convert a numeric thread into interval labels defined by explicit bin edges.

`Thread::bucketize(...)` is the one-dimensional interval bucketing helper behind histogram-style feature engineering, grading bands, and coarse segmentation rules.

## Parameters

**bins**

Strictly increasing interval edges.

Example: `{0.0, 20.0, 40.0, 60.0}` defines three buckets.

**labels**

Optional explicit bucket labels.

- Leave empty for default labels `0, 1, 2, ...`.
- Pass explicit labels such as `{10, 20, 30}` when later logic expects custom codes.

**right**

Right-closure policy.

- `true`: intervals are right-closed.
- `false`: intervals are left-closed and right-open.

**include_lowest**

Whether the first interval includes its lower boundary.

- `true`: values exactly equal to the first edge can enter the first bucket.
- `false`: the first lower boundary stays open.

## Returns

`Thread`
    A numeric thread of bucket labels, with missing output where the source value falls outside the provided bin range.

## See Also

- <a href="../dataframe/bucketize.html"><code>DataFrame::bucketize</code></a>: bucketize selected numeric frame features while preserving the rest of the table.
- <a href="btw.html"><code>Thread::btw</code></a>: perform interval-membership tests without collapsing values into bucket labels.

## Notes

For `N` output buckets you need `N + 1` bin edges.

Explicit labels are useful when downstream rules expect domain codes instead of `0, 1, 2, ...`.

## Examples

Example 1. Bucketize with default output labels.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));

auto out = scores.bucketize({0.0, 20.0, 40.0, 60.0});
```

### Output

```text
0  0
1  0
2  1
3  1
4  2
dtype: Series
```

Example 2. Bucketize with explicit output labels.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));

auto out = scores.bucketize({0.0, 20.0, 40.0, 60.0}, {10, 20, 30}, true, true);
```

### Output

```text
0  10
1  10
2  20
3  20
4  30
dtype: Series
```



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
