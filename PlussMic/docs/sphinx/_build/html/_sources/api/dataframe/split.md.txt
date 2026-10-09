<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c80d3f1de3b4 -->

# DataFrame::split

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
std::pair<DataFrame, DataFrame> DataFrame::split(double frac) const
std::pair<DataFrame, DataFrame> DataFrame::split(double frac,
                                                 const SplitOptions& options) const
std::pair<DataFrame, DataFrame> DataFrame::split(double frac,
                                                 Axis axis,
                                                 bool shuffle = false) const
std::pair<DataFrame, DataFrame> DataFrame::split(double frac,
                                                 std::uint64_t random_state,
                                                 Axis axis = Axis::Case,
                                                 bool shuffle = true) const
```

## What It Does

Splits the current frame into two frames and returns them as a `std::pair`.

The supplied `frac` always means: the fraction assigned to the **second**
returned frame.

So:

- `split(0.2)` returns `(first_80_percent, last_20_percent)` by default
- `split(0.5, Axis::Feature)` returns `(left_half_of_features, right_half_of_features)`
- `split(0.2, seed)` performs a randomized case split with deterministic
  shuffling

This is useful outside ML-specific train/test workflows too: ordered time-ish
splits, random analytical holdouts, and wide-frame feature partitioning.

## Parameters

**frac** (`double`)
    Must be in `[0, 1]`. The proportion assigned to the second returned frame.

**options** (`SplitOptions`, optional)
    Controls:

    - `axis`: split rows (`Axis::Case`) or columns (`Axis::Feature`)
    - `shuffle`: whether the chosen axis is shuffled before splitting
    - `random_state`: optional deterministic seed used when shuffling

**axis** (`Axis`)
    Shorthand overload for choosing row-wise vs. feature-wise splitting.

**shuffle** (`bool`)
    Whether the chosen axis is shuffled before the split point is applied.

**random_state** (`std::uint64_t`)
    Deterministic seed for randomized splitting.

## Returns

`std::pair<DataFrame, DataFrame>`
    The first output receives the remainder of the selected axis; the second
    output receives the rounded `frac` share.

The exact count assigned to the second output is computed as:

```cpp
std::llround(frac * axis_size)
```

So with `5` rows, `split(0.4)` yields `2` rows on the right, while
`split(0.3)` also yields `2` rows on the right because `std::llround(1.5) == 2`.

## Examples

Example 1. Ordered row split: keep the leading rows on the left and send the
trailing `40%` to the right.

```cpp
using namespace pluss::table;

auto panel = DataFrame(
    {
        {"city", Slate(pluss::make_note({"A", "A", "B", "B", "C"}))},
        {"channel", Slate(pluss::make_note({"Cash", "Card", "Cash", "Card", "Cash"}))},
        {"amount", Slate(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}))},
        {"score", Slate(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}))},
    },
    Slate(pluss::make_note({"r0", "r1", "r2", "r3", "r4"})));

auto [lhs, rhs] = panel.split(0.4);
std::cout << lhs;
std::cout << rhs;
```

### Output

```text
==================================
     | city  channel  amount  score
----------------------------------
r0   | A     Cash     10      1
r1   | A     Card     20      2
r2   | B     Cash     30      3
----------------------------------
[3 cs x 4 feat]
==================================
     | city  channel  amount  score
----------------------------------
r3   | B     Card     40      4
r4   | C     Cash     50      5
----------------------------------
[2 cs x 4 feat]
```

Because the default axis is `Axis::Case` and the default is `shuffle = false`,
the split respects the existing row order. With `5` rows and `frac = 0.4`, the
second output receives `llround(5 * 0.4) = 2` rows.

Example 2. Ordered feature split: partition columns instead of rows.

```cpp
auto [left_features, right_features] = panel.split(0.5, Axis::Feature, false);
std::cout << left_features;
std::cout << right_features;
```

### Output

```text
===================
     | city  channel
-------------------
r0   | A     Cash
r1   | A     Card
r2   | B     Cash
r3   | B     Card
r4   | C     Cash
-------------------
[5 cs x 2 feat]
===================
     | amount  score
-------------------
r0   | 10      1
r1   | 20      2
r2   | 30      3
r3   | 40      4
r4   | 50      5
-------------------
[5 cs x 2 feat]
```

Here the row axis stays intact while the feature axis is split in order. The
first output keeps the leading features; the second output keeps the trailing
features.

Example 3. Seeded random row split: verify reproducibility and lossless
partitioning without depending on one library-specific shuffle order.

```cpp
auto first = panel.split(0.4, 7);
auto second = panel.split(0.4, 7);

std::cout << first.first.equals(second.first) << "\n";
std::cout << first.second.equals(second.second) << "\n";
std::cout << first.first.ncases() << ", " << first.second.ncases() << "\n";

auto recombined = first.first.extend(first.second)
                        .arrange({"city", "channel", "amount", "score"});
auto original_sorted = panel.arrange({"city", "channel", "amount", "score"});
std::cout << recombined.equals(original_sorted) << "\n";
```

### Output

```text
true
true
3, 2
true
```

The important portable contract is not the exact shuffled row order. It is that
the same seed reproduces the same split on the same build, the requested counts
are respected, and the two outputs still partition the original rows without
loss or duplication.

Example 4. Use `SplitOptions` when you want the full policy in one place.

```cpp
DataFrame::SplitOptions options;
options.axis = Axis::Case;
options.shuffle = false;

auto [headish, tailish] = panel.split(0.3, options);
std::cout << headish.label().values() << "\n";
std::cout << tailish.label().values() << "\n";
```

### Output

```text
[r0, r1, r2]
[r3, r4]
```

This example is intentionally small because it highlights the rounding rule.
With `5` rows and `frac = 0.3`, the second output still receives
`llround(1.5) = 2` rows.

Example 5. The boundary cases `0.0` and `1.0` produce empty/full extremes.

```cpp
auto all_left = panel.split(0.0);
auto all_right = panel.split(1.0);

std::cout << all_left.first.ncases() << ", " << all_left.second.ncases() << "\n";
std::cout << all_right.first.ncases() << ", " << all_right.second.ncases() << "\n";
```

### Output

```text
5, 0
0, 5
```

These edge cases are often useful when `frac` is computed dynamically and can
legitimately collapse to either boundary.

## Notes

- `split(0.0)` returns `(whole_frame, empty_frame)`.
- `split(1.0)` returns `(empty_frame, whole_frame)`.
- The second-output count is rounded with `std::llround(frac * axis_size)`, not
  floored.
- For `Axis::Case`, case labels are preserved on each side.
- For `Axis::Feature`, the original row axis stays intact while features are
  partitioned.
- Randomized splits partition the original data; they do not sample with
  replacement.

## See Also

- [`DataFrame::shuffle`](shuffle.md)
- [`DataFrame::reshuffle`](reshuffle.md)
- [`DataFrame::slice`](slice.md)
- [`DataFrame::take`](take.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sql.html">
    <span class="label">Previous</span>
    <span class="title">sql</span>
  </a>
  <a class="next-link" href="stack.html">
    <span class="label">Next</span>
    <span class="title">stack</span>
  </a>
</div>
