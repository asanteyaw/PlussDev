<!-- pluss-hand-authored -->

# TimeSeriesAccessor::lag_block

[Back to TimeSeriesAccessor API Reference](../../shelf_ts_accessor_api_reference)

## Signature

```cpp
DataFrame TimeSeriesAccessor::lag_block(pluss::RangeView<std::int64_t> lags) const
```

## What It Does

Computes several [`lag()`](lag.md) shifts of the bound shelf at once, each as
its own output column named `"lag" + <lag value>`. Requires at least one lag.

## Parameters

**lags** (`pluss::RangeView<std::int64_t>`)

The lag distances to compute, in the order given. Must not be empty, or it
throws.

## Returns

`DataFrame`
    A new frame with one column per requested lag (named `"lag1"`, `"lag2"`, and
    so on), same case labels as the bound shelf.

## Examples

Example 1. Compute two lags of the same shelf at once.

```cpp
using namespace pluss::table;

auto load = Shelf(pluss::make_series({10.0, 20.0, 15.0, 30.0}));
auto out = load.ts().lag_block({1, 2});
std::cout << out;
```

### Output

```text
=================
     | lag1  lag2
-----------------
0    | NULL  NULL
1    | 10    NULL
2    | 20    10  
3    | 15    20  
-----------------
[4 cs x 2 feat]
```

`lag1` is the same as [`lag(1)`](lag.md): `[NULL, 10, 20, 15]`. `lag2` is
[`lag(2)`](lag.md): each position now shows the value from two positions
earlier, so both `0` and `1` (which have no position two back) are `NULL`,
position `2` shows position `0`'s `10`, and position `3` shows position `1`'s
`20`.

Example 2. An empty `lags` throws.

```cpp
try {
  auto bad = load.ts().lag_block({});
} catch (const pluss::Error& e) {
  std::cout << e.message();
}
```

### Output

```text
TimeSeriesAccessor::lag_block: expected at least one lag
```

## Notes

- Each requested lag becomes its own output column, letting you compare several
  lag depths of the same shelf side by side in one call, rather than calling
  [`lag()`](lag.md) repeatedly and combining the results yourself.
- Output columns are always named `"lag" + <value>` — there's no way to
  customize the column names from this method directly.
- A pre-existing missing entry in the source shelf is honored in every lag
  column, the same as [`lag()`](lag.md) itself.

## See Also

- [`TimeSeriesAccessor::lag`](lag.md): the single-lag building block this
  method computes several of at once.
- [`TimeSeriesAccessor::rolling_block`](rolling_block.md): the equivalent
  "several at once" pattern for rolling window statistics instead of lags.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lag.html">
    <span class="label">Previous</span>
    <span class="title">lag</span>
  </a>
  <a class="next-link" href="le.html">
    <span class="label">Next</span>
    <span class="title">le</span>
  </a>
</div>
