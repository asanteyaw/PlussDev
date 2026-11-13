<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1339aabd225b -->

# DataFrame::cummin

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame DataFrame::cummin(bool skip_nulls = true) const
DataFrame DataFrame::cummin(Axis axis, bool skip_nulls = true) const
```

## What It Does

Replaces every numeric feature with its running (cumulative) minimum, case by case:
the value at case `i` becomes the smallest value seen in that feature from the first
case through case `i`, not a single reduced summary value.

`cummin()`, [`cummax()`](cummax.md), [`cumprod()`](cumprod.md), and
[`cumsum()`](cumsum.md) are all thin wrappers over the same underlying running-statistic
machinery, so they all share identical `axis` and `skip_nulls` semantics — only the
running operation itself (min, max, product, or sum) differs.

## Parameters

**axis** (`Axis`, default `Axis::Case`)

Direction to accumulate along. `Axis::Case` (the default) runs down each feature's
cases in order. Passing `Axis::Feature` accumulates across a row's features left to
right instead.

**skip_nulls** (`bool`, default `true`)

Controls what happens to the running minimum once a null is encountered:

- `true` (the default): a null produces a null *only at that case*. The running
  minimum is preserved underneath it and accumulation resumes normally from the next
  observed value.
- `false`: the first null poisons every case after it. Once a null is hit, that case
  and every subsequent case in the cumulative output become null too.

## Returns

`DataFrame`
    A new frame, same shape as the source, with every numeric feature replaced by its
    running minimum. Non-numeric features are left unchanged.

## Examples

Example 1. Default behavior: a null is a gap, not a break.

```cpp
using namespace pluss::table;

auto scores = DataFrame({
    {"student", pluss::make_note({"Ada", "Bola", "Chen", "Dede"})},
    {"score",   pluss::make_series({10.0, 20.0, std::numeric_limits<double>::quiet_NaN(), 5.0})},
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
auto running_min = scores.cummin();  // skip_nulls = true (default)
std::cout << running_min;
```

### Output (after)

```text
=====================
     | student  score
---------------------
0    | Ada      10   
1    | Bola     10   
2    | Chen     NULL 
3    | Dede     5    
---------------------
[4 cs x 2 feat]
```

Case `2` (`Chen`) is `NULL` because that case's own value was missing — but case `3`
(`Dede`) is `5`, not `NULL`: the running minimum silently carried `10` forward under
the null and compared it against `5`, dropping to `5`.

Example 2. `skip_nulls = false`: the null poisons everything after it.

```cpp
auto running_min_strict = scores.cummin(false);
std::cout << running_min_strict;
```

### Output

```text
=====================
     | student  score
---------------------
0    | Ada      10   
1    | Bola     10   
2    | Chen     NULL 
3    | Dede     NULL 
---------------------
[4 cs x 2 feat]
```

With `skip_nulls = false`, case `3` is also `NULL` even though `Dede`'s own `score` was
a perfectly good `5` — the missing value at case `2` invalidated every running minimum
downstream of it.

## Notes

- `cummin()` **replaces** each numeric feature's values with the running minimum; it
  does not add a new column alongside the original.
- Non-numeric features pass through completely unchanged.
- For the running sum, product, or maximum instead, see [`cumsum()`](cumsum.md),
  [`cumprod()`](cumprod.md), and [`cummax()`](cummax.md) — all four share the
  `skip_nulls` semantics described here.
- To find *which case* holds each feature's minimum instead of a running value, see
  [`csmin()`](csmin.md).

## See Also

- [`DataFrame::min`](min.md): a single reduced minimum per feature, instead of a
  running value per case.
- [`DataFrame::cummax`](cummax.md), [`DataFrame::cumprod`](cumprod.md),
  [`DataFrame::cumsum`](cumsum.md): the other cumulative reductions.
- [`DataFrame::csmin`](csmin.md): the case label where each feature's minimum occurs.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cummax.html">
    <span class="label">Previous</span>
    <span class="title">cummax</span>
  </a>
  <a class="next-link" href="cumprod.html">
    <span class="label">Next</span>
    <span class="title">cumprod</span>
  </a>
</div>
