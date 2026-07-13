# Thread::bfill

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::bfill(std::int64_t limit = -1) -> Thread
```

Backward-fill missing values by borrowing the next observed value in the thread.

`Thread::bfill(...)` is the one-dimensional backward-fill helper. Use it when each missing position should inherit from the next valid observation below it in the same thread.

## Parameters

**limit**

Maximum number of consecutive missing positions to fill per run.

- `-1`: unlimited backward fill.
- Positive integer: fill at most that many missing positions in each consecutive run.

## Returns

`Thread`
    A new thread with backward-filled values.

## See Also

- <a href="ffill.html"><code>Thread::ffill</code></a>: propagate the last observed value forward instead.
- <a href="../dataframe/bfill.html"><code>DataFrame::bfill</code></a>: backward-fill several frame features together.

## Notes

Trailing missing runs stay missing because there is no later observed value to borrow.

## Examples

Example 1. Backward-fill every eligible missing position.

```cpp
using namespace pluss::table;

auto missing = Thread(pluss::make_series({
    10.0,
    std::numeric_limits<double>::quiet_NaN(),
    std::numeric_limits<double>::quiet_NaN(),
    40.0,
    50.0,
}));

auto out = missing.bfill();
```

### Output

```text
0  10
1  40
2  40
3  40
4  50
dtype: Series
```

Example 2. Limit backward fill to one missing position per run.

```cpp
using namespace pluss::table;

auto missing = Thread(pluss::make_series({
    10.0,
    std::numeric_limits<double>::quiet_NaN(),
    std::numeric_limits<double>::quiet_NaN(),
    40.0,
    50.0,
}));

auto out = missing.bfill(1);
```

### Output

```text
0  10
1  NA
2  40
3  40
4  50
dtype: Series
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="astype.html">
    <span class="label">Previous</span>
    <span class="title">astype</span>
  </a>
  <a class="next-link" href="bounded.html">
    <span class="label">Next</span>
    <span class="title">bounded</span>
  </a>
</div>
