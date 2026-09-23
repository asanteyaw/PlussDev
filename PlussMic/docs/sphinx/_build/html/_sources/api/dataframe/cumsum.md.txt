<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d8adecc3f2cb -->

# DataFrame::cumsum

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::cumsum(bool skip_nulls = true) const -> DataFrame
DataFrame::cumsum(Axis axis, bool skip_nulls = true) const -> DataFrame
```

## What It Does

Replaces every numeric feature with its running (cumulative) sum, case by case: the
value at case `i` becomes the sum of that feature's values from the first case through
case `i` — a running total, not a single reduced summary value.

The interesting behavior to understand up front is what happens at a null. `cumsum()`
does not simply skip the *cell* the way [`sum()`](sum.md) skips nulls when reducing to
one total; a running total has to decide what to do at every case *after* the null too,
and that decision is controlled entirely by `skip_nulls`.

## Parameters

**axis** (`Axis`, default `Axis::Case`)

Direction to accumulate along. `Axis::Case` (the default) runs down each feature's cases
in order — the usual "running total over time/rows" reading. Passing `Axis::Feature`
accumulates across a row's features left to right instead.

**skip_nulls** (`bool`, default `true`)

Controls what happens to the running total once a null is encountered:

- `true` (the default): a null produces a null *only at that case*. The running total is
  preserved underneath it and accumulation resumes normally from the next observed value
  — the null is a gap in the output, not a break in the running sum.
- `false`: the first null poisons every case after it. Once a null is hit, that case and
  every subsequent case in the cumulative output become null too.

## Returns

`DataFrame`
    A new frame, same shape as the source, with every numeric feature replaced by its
    running total. Non-numeric features are left unchanged.

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
auto running = scores.cumsum();  // skip_nulls = true (default)
std::cout << running;
```

### Output (after)

```text
=====================
     | student  score
---------------------
0    | Ada      10   
1    | Bola     30   
2    | Chen     NULL 
3    | Dede     35   
---------------------
[4 cs x 2 feat]
```

Case `2` (`Chen`) is `NULL` because that case's own value was missing — but case `3`
(`Dede`) is `35`, not `NULL`: the running total silently carried `30` forward under the
null and added `5` to it. The null only ever affected its own row's output cell.

Example 2. `skip_nulls = false`: the null poisons everything after it.

```cpp
auto running_strict = scores.cumsum(false);
std::cout << running_strict;
```

### Output

```text
=====================
     | student  score
---------------------
0    | Ada      10   
1    | Bola     30   
2    | Chen     NULL 
3    | Dede     NULL 
---------------------
[4 cs x 2 feat]
```

With `skip_nulls = false`, case `3` is also `NULL` even though `Dede`'s own `score` was a
perfectly good `5` — the missing value at case `2` invalidated every running total from
that point on. Reach for `false` specifically when a gap in the source data should mean
"we can no longer trust any total downstream of it," such as an interrupted sensor feed
or a broken audit trail.

## Notes

- `cumsum()` **replaces** each numeric feature's values with the running total; it does
  not add a new column alongside the original. Copy the original feature first (for
  example with [`assign()`](assign.md)) if you need both the raw and cumulative values
  side by side.
- Non-numeric features pass through completely unchanged.
- For the running product, minimum, or maximum instead of a running sum, see
  [`cumprod()`](cumprod.md), [`cummin()`](cummin.md), and [`cummax()`](cummax.md) — all
  four share the same `skip_nulls` semantics described here.

## See Also

- [`DataFrame::sum`](sum.md): a single reduced total per feature, instead of a running
  total per case.
- [`DataFrame::cumprod`](cumprod.md), [`DataFrame::cummax`](cummax.md),
  [`DataFrame::cummin`](cummin.md): the other cumulative reductions.
- [`DataFrame::fillnull`](fillnull.md): resolve nulls before accumulating, if a gap
  should not appear in the running total at all.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cumprod.html">
    <span class="label">Previous</span>
    <span class="title">cumprod</span>
  </a>
  <a class="next-link" href="defined.html">
    <span class="label">Next</span>
    <span class="title">defined</span>
  </a>
</div>
