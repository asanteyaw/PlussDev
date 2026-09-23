<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9e35ee07b6ed -->

# DataFrameTsAccessor::ewm

[Back to DataFrameTsAccessor API Reference](../../dataframe_ts_accessor_api_reference)

## Signature

```cpp
DataFrameTsAccessor::ewm(double alpha, WindowStatistic statistic = WindowStatistic::Mean, bool adjusted = true, bool bias_corrected = true, std::int64_t min_count = 1, bool ignore_nulls = false) const -> DataFrame
```

## What It Does

An exponentially weighted moving statistic: each row's output blends every prior
row (and itself), with weight decaying geometrically the further back a row is.
Only `WindowStatistic::Mean`, `Var`, and `Std` are supported. With smoothing factor
`alpha` and `beta = 1 - alpha`, row `i`'s weight for an observation `j` rows back is
proportional to `beta^j` — larger `alpha` forgets the past faster.

`adjusted` selects between two well-known formulations:

- **`adjusted = true`** (the default): each row's mean is a normalized weighted
  average, `sum(x[k] * beta^(i-k)) / sum(beta^(i-k))` for `k` from `0` to `i`. This
  is well-defined from the very first row.
- **`adjusted = false`**: the classic recursive form, `y[0] = x[0]`,
  `y[i] = beta * y[i-1] + alpha * x[i]`. This form requires the input to have no
  nulls at all (throws otherwise).

## Parameters

**alpha** (`double`)

Smoothing factor in `(0, 1]`. Larger values weight recent observations more
heavily.

**statistic** (`WindowStatistic`, default `Mean`)

`Mean`, `Var`, or `Std` — any other value throws.

**adjusted** (`bool`, default `true`)

Selects the normalized-weighted-average form (`true`) versus the classic recursive
form (`false`), described above.

**bias_corrected** (`bool`, default `true`)

For `Var`/`Std` only: whether to apply a finite-sample bias correction to the
exponentially weighted variance.

**min_count** (`std::int64_t`, default `1`)

Minimum non-null observations seen so far required for a row's output to be
non-`NULL`.

**ignore_nulls** (`bool`, default `false`)

When `true`, a null observation is skipped entirely when computing decay weights
(as if it were never in the series), rather than contributing a gap in the decay
schedule.

## Returns

`DataFrame`
    A new frame, same shape and case labels as the accessor's bound features, with
    each feature replaced by its exponentially weighted statistic.

## Examples

Example 1. The default (`adjusted = true`) exponentially weighted mean.

```cpp
using namespace pluss::table;

auto readings = DataFrame({
    {"sensor_id", Thread(pluss::make_series({1, 2, 3, 4}))},
    {"load",      Thread(pluss::make_series({10.0, 20.0, 15.0, 30.0}))},
});

auto out = readings.features({"load"}).ts().ewm(0.5);
std::cout << out;
```

### Output

```text
==================
     | load       
------------------
0    | 10         
1    | 16.66666667
2    | 15.71428571
3    | 23.33333333
------------------
[4 cs x 1 feat]
```

With `alpha = 0.5` (`beta = 0.5`), row `0` is just `10` (nothing to blend with yet).
Row `1` is `(10 * 0.5 + 20 * 1) / (0.5 + 1) = 16.667`. Row `2` is
`(10 * 0.25 + 20 * 0.5 + 15 * 1) / (0.25 + 0.5 + 1) = 15.714`. Row `3` is
`(10 * 0.125 + 20 * 0.25 + 15 * 0.5 + 30 * 1) / (0.125 + 0.25 + 0.5 + 1) = 23.333`.

Example 2. The classic recursive form, `adjusted = false`.

```cpp
auto recursive = readings.features({"load"}).ts().ewm(0.5, WindowStatistic::Mean, false);
std::cout << recursive;
```

### Output

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

Here `y[0] = 10`; `y[1] = 0.5 * 10 + 0.5 * 20 = 15`; `y[2] = 0.5 * 15 + 0.5 * 15 =
15`; `y[3] = 0.5 * 15 + 0.5 * 30 = 22.5` — a different (and, for `alpha < 1`,
always more `y[0]`-anchored) result than the adjusted form above.

Example 3. `WindowStatistic::Std`, contrasting the default `bias_corrected =
true` against `bias_corrected = false`.

```cpp
auto std_corrected = readings.features({"load"}).ts().ewm(0.5, WindowStatistic::Std);
std::cout << std_corrected;

auto std_uncorrected = readings.features({"load"}).ts().ewm(
    0.5, WindowStatistic::Std, true, false);
std::cout << std_uncorrected;
```

### Output (bias_corrected = true, the default)

```text
==================
     | load       
------------------
0    | NULL       
1    | 7.071067812
2    | 4.225771274
3    | 9.449111825
------------------
[4 cs x 1 feat]
```

### Output (bias_corrected = false)

```text
==================
     | load       
------------------
0    | 0          
1    | 4.714045208
2    | 3.194382825
3    | 7.453559925
------------------
[4 cs x 1 feat]
```

With `bias_corrected = true`, row `0` needs at least 2 observations to apply
the correction factor, so it's `NULL` even though `min_count = 1`; every other
row's std is scaled up relative to the uncorrected version. With
`bias_corrected = false`, row `0` is a well-defined (if trivial) `0` — the
"variance" of a single point — and no correction factor is ever applied to any
row.

Example 4. `ignore_nulls = true` changes how much weight later rows put on a
null gap.

```cpp
auto readings_with_null = DataFrame({
    {"sensor_id", Thread(pluss::make_series({1, 2, 3, 4}))},
    {"load",      Thread(pluss::make_series(
        {10.0, std::numeric_limits<double>::quiet_NaN(), 15.0, 30.0}))},
});

auto keep_gap = readings_with_null.features({"load"}).ts().ewm(0.5);
std::cout << keep_gap;

auto skip_gap = readings_with_null.features({"load"}).ts().ewm(
    0.5, WindowStatistic::Mean, true, true, 1, true);
std::cout << skip_gap;
```

### Output (ignore_nulls = false, the default)

```text
==================
     | load       
------------------
0    | 10         
1    | 10         
2    | 14         
3    | 23.84615385
------------------
[4 cs x 1 feat]
```

### Output (ignore_nulls = true)

```text
==================
     | load       
------------------
0    | 10         
1    | 10         
2    | 13.33333333
3    | 22.85714286
------------------
[4 cs x 1 feat]
```

Row `1`'s null contributes no value either way, and both forms report `10` —
the running average carried over from row `0`, since a null row never
resets or invalidates the decay (it's still `count.ge(min_count)`-valid, just
unchanged from its predecessor). The two forms diverge starting at row `2`:
with the default `ignore_nulls = false`, row `2` is still treated as being
*two* decay-steps behind row `0` (the gap itself counts toward the decay
distance), giving it less relative weight (`14`); with `ignore_nulls = true`,
the null row is skipped entirely when counting decay distance, so row `2` is
treated as only *one* step behind row `0`, giving it more relative weight
(`13.333`) — and the same pattern carries through to row `3`.

Example 5. `adjusted = false` on a feature with a null value, to see it throw.

```cpp
auto readings_with_null = DataFrame({
    {"sensor_id", Thread(pluss::make_series({1, 2, 3, 4}))},
    {"load",      Thread(pluss::make_series(
        {10.0, std::numeric_limits<double>::quiet_NaN(), 15.0, 30.0}))},
});

try {
  auto out = readings_with_null.features({"load"}).ts().ewm(0.5, WindowStatistic::Mean, false);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Thread::decay(adjusted=false): nullable input requires adjusted=true
```

The classic recursive form has no defined way to skip over a missing
observation, so a null anywhere in the feature throws immediately — the
`adjusted = true` (default) form, used in Example 1, tolerates nulls instead.

Example 6. Pass a `statistic` other than `Mean`/`Var`/`Std`, to see it throw.

```cpp
try {
  auto out = readings.features({"load"}).ts().ewm(0.5, static_cast<WindowStatistic>(-1));
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Thread::decay: only Mean, Var, and Std are supported
```

`WindowStatistic` has other members used elsewhere (by [`rolling()`](rolling.md)
and friends), but `ewm()` only implements these three — anything else is
rejected rather than silently falling back to one of them.

## Notes

- `adjusted = false` requires the source feature to have no null values at all —
  it throws immediately otherwise (`adjusted = true` tolerates nulls, subject to
  `min_count`/`ignore_nulls`).
- `alpha = 1.0` degenerates to returning the input unchanged (each row weighs only
  itself).
- `bias_corrected` only affects `Var`/`Std`, applying a correction factor akin to
  Bessel's correction for exponentially weighted variance; it has no effect on
  `Mean`.

## See Also

- [`DataFrameTsAccessor::expanding`](expanding.md): an equally-weighted "everything
  so far" statistic, as opposed to this method's exponentially decaying weights.
- [`DataFrameTsAccessor::weigh`](weigh.md): a fixed-size window with an explicit,
  caller-supplied weight kernel instead of a geometric decay.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="eq.html">
    <span class="label">Previous</span>
    <span class="title">eq</span>
  </a>
  <a class="next-link" href="expanding.html">
    <span class="label">Next</span>
    <span class="title">expanding</span>
  </a>
</div>
