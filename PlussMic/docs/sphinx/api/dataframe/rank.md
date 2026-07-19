<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6492ca4d5725 -->

# DataFrame::rank

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::rank(RankTieRule ties = RankTieRule::Average,
                 RankNullRule nulls = RankNullRule::Keep,
                 bool ascending = true,
                 bool percentage = false) const -> DataFrame
```

## What It Does

Replaces every numeric feature's values with their rank among that feature's other
values — the smallest value gets rank `1` (with `ascending = true`, the default), the
next-smallest gets rank `2`, and so on. The result is the same shape as the source frame;
ranks replace values in place, feature by feature, rather than being reduced to a single
summary.

Two things need a policy decision that plain sorting doesn't: what happens when two
values are **tied**, and what happens to a **null**. `rank()` exposes both as explicit
parameters instead of picking a silent default you'd have to discover by testing.

## Parameters

**ties** (`RankTieRule`, default `Average`)

How to resolve rank ties (equal values):

- `Average`: tied values all receive the mean of the ranks they would have occupied
  (two values tied for ranks `2` and `3` both get `2.5`).
- `Lowest`: tied values all receive the lowest of the ranks they would have occupied.
- `Highest`: tied values all receive the highest of the ranks they would have occupied.
- `First`: ties are broken by original row order — whichever tied value appears first in
  the frame gets the lower rank.
- `Dense`: like `Lowest`, but without leaving a gap in the rank sequence afterward (the
  next distinct value always gets `previous_rank + 1`, not `previous_rank + count_of_ties`).

**nulls** (`RankNullRule`, default `Keep`)

How to rank missing values: `Keep` leaves a null's rank as null (it is excluded from
ranking entirely); `Top` ranks nulls as if they were the smallest values; `Bottom` ranks
them as if they were the largest.

**ascending** (`bool`, default `true`)

`true` ranks smallest-to-largest (smallest gets rank `1`); `false` reverses the order
(largest gets rank `1`).

**percentage** (`bool`, default `false`)

When `true`, each rank is divided by the number of ranked observations, giving a
percentile-style rank in `(0, 1]` instead of a raw integer-like rank.

## Returns

`DataFrame`
    A new frame, same shape as the source, with every numeric feature's values replaced
    by their rank under the requested policy. Non-numeric features are left unchanged.

## Examples

Example 1. Default tie policy: tied values share the average of their ranks.

```cpp
using namespace pluss::table;

auto scores = DataFrame({
    {"student", pluss::make_note({"Ada", "Bola", "Chen", "Dede"})},
    {"score",   pluss::make_series({80.0, 95.0, 80.0, 60.0})},
});

std::cout << scores;
```

### Output (before)

```text
=====================
     | student  score
---------------------
0    | Ada      80   
1    | Bola     95   
2    | Chen     80   
3    | Dede     60   
---------------------
[4 cs x 2 feat]
```

```cpp
auto ranked = scores.rank();  // ties = Average, ascending = true (defaults)
std::cout << ranked;
```

### Output (after)

```text
=====================
     | student  score
---------------------
0    | Ada      2.5  
1    | Bola     4    
2    | Chen     2.5  
3    | Dede     1    
---------------------
[4 cs x 2 feat]
```

Sorted ascending, the four scores occupy ranks `1` (`60`), `2` and `3` (the tied `80`s),
and `4` (`95`). `Ada` and `Chen` — both at `80` — split the difference and each land on
`2.5` rather than one arbitrarily beating the other.

Example 2. Break the same tie by row order instead of averaging.

```cpp
auto ranked_first = scores.rank(DataFrame::RankTieRule::First);
std::cout << ranked_first;
```

### Output

```text
=====================
     | student  score
---------------------
0    | Ada      2    
1    | Bola     4    
2    | Chen     3    
3    | Dede     1    
---------------------
[4 cs x 2 feat]
```

With `RankTieRule::First`, `Ada`'s `80` (case `0`) beats `Chen`'s `80` (case `2`) purely
because it appears earlier in the frame — `Ada` gets `2` and `Chen` gets `3`, with no tie
left unresolved. This is the policy to reach for when downstream logic needs a strict,
unambiguous ordering (assigning unique positions in a leaderboard, for instance) rather
than a statistically fair split.

## Notes

- `rank()` **replaces** each numeric feature's values with ranks; it does not add a new
  column alongside the original. Keep the original values with [`assign()`](assign.md)
  first if you need both.
- `RankNullRule::Keep` (the default) means a row with a null in the ranked feature simply
  has no rank there — it does not consume a rank position, so the remaining values still
  rank as if that row didn't exist for this feature.
- `percentage = true` is useful when you want a `[0, 1]`-scaled competitive standing
  (percentile rank) rather than a raw position number, without a separate division step.

## See Also

- [`DataFrame::sort_values`](sort_values.md): reorder rows by value, rather than
  annotating each row with its rank in place.
- [`DataFrame::percentiles`](percentiles.md): a related but distinct question — the
  value at a given percentile, rather than a value's own rank.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rand.html">
    <span class="label">Previous</span>
    <span class="title">rand</span>
  </a>
  <a class="next-link" href="rdiv.html">
    <span class="label">Next</span>
    <span class="title">rdiv</span>
  </a>
</div>
