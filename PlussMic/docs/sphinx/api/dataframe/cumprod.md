<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: add5a0f92893 -->

# DataFrame::cumprod

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame DataFrame::cumprod(bool skip_nulls = true) const
DataFrame DataFrame::cumprod(Axis axis, bool skip_nulls = true) const
```

## What It Does

Replaces every numeric feature with its running (cumulative) product, case by case:
the value at case `i` becomes the product of that feature's values from the first
case through case `i`, not a single reduced summary value.

`cumprod()`, [`cummax()`](cummax.md), [`cummin()`](cummin.md), and
[`cumsum()`](cumsum.md) are all thin wrappers over the same underlying running-statistic
machinery, so they all share identical `axis` and `skip_nulls` semantics — only the
running operation itself (product, max, min, or sum) differs.

## Parameters

**axis** (`Axis`, default `Axis::Case`)

Direction to accumulate along. `Axis::Case` (the default) runs down each feature's
cases in order. Passing `Axis::Feature` accumulates across a row's features left to
right instead.

**skip_nulls** (`bool`, default `true`)

Controls what happens to the running product once a null is encountered:

- `true` (the default): a null produces a null *only at that case*. The running
  product is preserved underneath it and accumulation resumes normally from the next
  observed value.
- `false`: the first null poisons every case after it. Once a null is hit, that case
  and every subsequent case in the cumulative output become null too.

## Returns

`DataFrame`
    A new frame, same shape as the source, with every numeric feature replaced by its
    running product. Non-numeric features are left unchanged.

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
auto running_product = scores.cumprod();  // skip_nulls = true (default)
std::cout << running_product;
```

### Output (after)

```text
=====================
     | student  score
---------------------
0    | Ada      10   
1    | Bola     200  
2    | Chen     NULL 
3    | Dede     1000 
---------------------
[4 cs x 2 feat]
```

Case `2` (`Chen`) is `NULL` because that case's own value was missing — but case `3`
(`Dede`) is `1000`, not `NULL`: the running product silently carried `200` (`10 * 20`)
forward under the null and multiplied it by `5`.

Example 2. `skip_nulls = false`: the null poisons everything after it.

```cpp
auto running_product_strict = scores.cumprod(false);
std::cout << running_product_strict;
```

### Output

```text
=====================
     | student  score
---------------------
0    | Ada      10   
1    | Bola     200  
2    | Chen     NULL 
3    | Dede     NULL 
---------------------
[4 cs x 2 feat]
```

With `skip_nulls = false`, case `3` is also `NULL` even though `Dede`'s own `score` was
a perfectly good `5` — the missing value at case `2` invalidated every running product
downstream of it.

## Notes

- `cumprod()` **replaces** each numeric feature's values with the running product; it
  does not add a new column alongside the original.
- Non-numeric features pass through completely unchanged.
- For the running sum, minimum, or maximum instead, see [`cumsum()`](cumsum.md),
  [`cummin()`](cummin.md), and [`cummax()`](cummax.md) — all four share the
  `skip_nulls` semantics described here.
- A single `0` observed anywhere in a feature collapses every case after it to `0`,
  since anything multiplied by `0` stays `0` — this is expected behavior, not a bug.

## See Also

- [`DataFrame::prod`](prod.md): a single reduced product per feature, instead of a
  running value per case.
- [`DataFrame::cummax`](cummax.md), [`DataFrame::cummin`](cummin.md),
  [`DataFrame::cumsum`](cumsum.md): the other cumulative reductions.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cummin.html">
    <span class="label">Previous</span>
    <span class="title">cummin</span>
  </a>
  <a class="next-link" href="cumsum.html">
    <span class="label">Next</span>
    <span class="title">cumsum</span>
  </a>
</div>
