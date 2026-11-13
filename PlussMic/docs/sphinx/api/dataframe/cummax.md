<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 34ef56b0fc49 -->

# DataFrame::cummax

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame DataFrame::cummax(bool skip_nulls = true) const
DataFrame DataFrame::cummax(Axis axis, bool skip_nulls = true) const
```

## What It Does

Replaces every numeric feature with its running (cumulative) maximum, case by case:
the value at case `i` becomes the largest value seen in that feature from the first
case through case `i`, not a single reduced summary value.

`cummax()`, [`cummin()`](cummin.md), [`cumprod()`](cumprod.md), and
[`cumsum()`](cumsum.md) are all thin wrappers over the same underlying running-statistic
machinery, so they all share identical `axis` and `skip_nulls` semantics — only the
running operation itself (max, min, product, or sum) differs.

## Parameters

**axis** (`Axis`, default `Axis::Case`)

Direction to accumulate along. `Axis::Case` (the default) runs down each feature's
cases in order. Passing `Axis::Feature` accumulates across a row's features left to
right instead.

**skip_nulls** (`bool`, default `true`)

Controls what happens to the running maximum once a null is encountered:

- `true` (the default): a null produces a null *only at that case*. The running
  maximum is preserved underneath it and accumulation resumes normally from the next
  observed value.
- `false`: the first null poisons every case after it. Once a null is hit, that case
  and every subsequent case in the cumulative output become null too.

## Returns

`DataFrame`
    A new frame, same shape as the source, with every numeric feature replaced by its
    running maximum. Non-numeric features are left unchanged.

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
auto running_max = scores.cummax();  // skip_nulls = true (default)
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

Case `2` (`Chen`) is `NULL` because that case's own value was missing — but case `3`
(`Dede`) is `20`, not `NULL`: the running maximum silently carried `20` forward under
the null and compared it against `5`, keeping `20`.

Example 2. `skip_nulls = false`: the null poisons everything after it.

```cpp
auto running_max_strict = scores.cummax(false);
std::cout << running_max_strict;
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

With `skip_nulls = false`, case `3` is also `NULL` even though `Dede`'s own `score` was
a perfectly good `5` — the missing value at case `2` invalidated every running maximum
downstream of it.

## Notes

- `cummax()` **replaces** each numeric feature's values with the running maximum; it
  does not add a new column alongside the original.
- Non-numeric features pass through completely unchanged.
- For the running sum, product, or minimum instead, see [`cumsum()`](cumsum.md),
  [`cumprod()`](cumprod.md), and [`cummin()`](cummin.md) — all four share the
  `skip_nulls` semantics described here.
- To find *which case* holds each feature's maximum instead of a running value, see
  [`csmax()`](csmax.md).

## See Also

- [`DataFrame::max`](max.md): a single reduced maximum per feature, instead of a
  running value per case.
- [`DataFrame::cummin`](cummin.md), [`DataFrame::cumprod`](cumprod.md),
  [`DataFrame::cumsum`](cumsum.md): the other cumulative reductions.
- [`DataFrame::csmax`](csmax.md): the case label where each feature's maximum occurs.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="csmin.html">
    <span class="label">Previous</span>
    <span class="title">csmin</span>
  </a>
  <a class="next-link" href="cummin.html">
    <span class="label">Next</span>
    <span class="title">cummin</span>
  </a>
</div>
