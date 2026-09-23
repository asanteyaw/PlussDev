<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2573bc7ad7c9 -->

# DataFrame::scan

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::scan(ScanStatistic statistic, bool skip_nulls = true) const -> DataFrame
DataFrame::scan(ScanStatistic statistic, Axis axis, bool skip_nulls = true) const -> DataFrame
```

## What It Does

The general-purpose running/cumulative-statistic engine behind
[`cumsum()`](cumsum.md), [`cumprod()`](cumprod.md), [`cummin()`](cummin.md), and
[`cummax()`](cummax.md) — each of those four is exactly `scan()` with one fixed
`ScanStatistic`:

- `cumsum(...)` = `scan(ScanStatistic::Sum, ...)`
- `cumprod(...)` = `scan(ScanStatistic::Product, ...)`
- `cummin(...)` = `scan(ScanStatistic::Minimum, ...)`
- `cummax(...)` = `scan(ScanStatistic::Maximum, ...)`

Call `scan()` directly when you want to select the statistic dynamically (as a
variable rather than a fixed call name); reach for the four named convenience
methods otherwise, since they read more clearly at the call site.

## Parameters

**statistic** (`ScanStatistic`)

Which running statistic to compute down (or across) each numeric feature:
`ScanStatistic::Sum`, `Product`, `Minimum`, or `Maximum`.

**axis** (`Axis`, default `Axis::Case`)

Direction to accumulate along. `Axis::Case` (the default) runs down each feature's
cases; `Axis::Feature` accumulates across a row's features left to right instead.

**skip_nulls** (`bool`, default `true`)

Controls what happens once a null is encountered — identical semantics to
[`cumsum()`](cumsum.md)'s `skip_nulls`:

- `true` (the default): a null produces a null only at that case; the running
  statistic is preserved underneath it and resumes normally at the next observed
  value.
- `false`: the first null poisons every case after it, all the way to the end of
  that feature.

## Returns

`DataFrame`
    A new frame, same shape as the source, with every numeric feature replaced by
    its running `statistic`. Non-numeric features are left unchanged.

## Examples

Example 1. Compute a running maximum — a null is a gap, not a break (the default).

```cpp
using namespace pluss::table;

auto scores = DataFrame({
    {"student", Thread(pluss::make_note({"Ada", "Bola", "Chen", "Dede"}))},
    {"score",   Thread(pluss::make_series({10.0, 20.0, std::numeric_limits<double>::quiet_NaN(), 5.0}))},
});
std::cout << scores;
```

### Output (before)

```text
=====================
     | student  score
---------------------
0    | Ada      10   
1    | Bola     20   
2    | Chen     NULL 
3    | Dede     5    
---------------------
[4 cs x 2 feat]
```

```cpp
auto running_max = scores.scan(ScanStatistic::Maximum);  // skip_nulls = true (default)
std::cout << running_max;
```

### Output (after)

```text
=====================
     | student  score
---------------------
0    | Ada      10   
1    | Bola     20   
2    | Chen     NULL 
3    | Dede     20   
---------------------
[4 cs x 2 feat]
```

Case `3` (`Dede`)'s own value (`5`) is smaller than the running maximum carried
forward from case `1` (`20`), so the running maximum stays `20` — exactly what
[`cummax()`](cummax.md) would produce, since `cummax()` is this same call with
`ScanStatistic::Maximum` hardcoded.

Example 2. `skip_nulls = false`: the null poisons everything after it.

```cpp
auto strict = scores.scan(ScanStatistic::Maximum, false);
std::cout << strict;
```

### Output

```text
=====================
     | student  score
---------------------
0    | Ada      10   
1    | Bola     20   
2    | Chen     NULL 
3    | Dede     NULL 
---------------------
[4 cs x 2 feat]
```

Case `3` is `NULL` here even though `Dede`'s own `score` (`5`) is perfectly present —
the missing value at case `2` invalidates every running result downstream of it.

## Notes

- `scan()` **replaces** each numeric feature's values with the running statistic; it
  does not add a new column alongside the original.
- Non-numeric features pass through completely unchanged.
- `ScanStatistic::Sum`/`Product`/`Minimum`/`Maximum` are the only four supported
  statistics; there is no running mean or running standard deviation available
  through `scan()`.

## See Also

- [`DataFrame::cumsum`](cumsum.md), [`DataFrame::cumprod`](cumprod.md),
  [`DataFrame::cummin`](cummin.md), [`DataFrame::cummax`](cummax.md): the four named
  convenience wrappers around this method, one per `ScanStatistic`.
- [`DataFrame::sum`](sum.md), [`DataFrame::max`](max.md): the single reduced totals,
  instead of a running value per case.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="save_txt.html">
    <span class="label">Previous</span>
    <span class="title">save_txt</span>
  </a>
  <a class="next-link" href="se.html">
    <span class="label">Next</span>
    <span class="title">se</span>
  </a>
</div>
