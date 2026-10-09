<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 43171cfdf167 -->

# DataFrame::sample

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::sample(std::int64_t n) const -> DataFrame
DataFrame::sample(std::int64_t n, bool replace, Axis axis) const -> DataFrame
DataFrame::sample(std::int64_t n, std::uint64_t random_state, bool replace = false, Axis axis = Axis::Case) const -> DataFrame

template <std::floating_point Float>
DataFrame::sample(Float frac) const -> DataFrame
template <std::floating_point Float>
DataFrame::sample(Float frac, bool replace, Axis axis) const -> DataFrame
template <std::floating_point Float>
DataFrame::sample(Float frac, std::uint64_t random_state, bool replace = false, Axis axis = Axis::Case) const -> DataFrame
```

## What It Does

Draws a random subset of cases (or, with `Axis::Feature`, a random subset of
features) from the frame — either a fixed count (`n`) or a proportion of the axis
(`frac`). Internally this is built on [`take()`](take.md): a set of positions is drawn
by a `std::mt19937_64` random-number engine, then those positions are taken in one
call, so the same shape guarantees `take()` makes (original case labels preserved,
feature order preserved) apply here too.

A note on reproducibility: passing `random_state` seeds the engine so the **same
seed on the same build (same compiler and standard library) reproduces the same
draw**. The draw itself, however, is produced by `std::shuffle` / a uniform integer
distribution from the C++ standard library, and the exact sequence those algorithms
produce for a given seed is not fixed by the C++ standard — only the underlying
`std::mt19937_64` engine is fully specified. In practice this means a `random_state`
reproduces results reliably across runs on the *same* platform and toolchain, but the
specific rows two different compilers or standard library implementations draw for
the same seed are not guaranteed to match each other.

## Parameters

**n** (`std::int64_t`)

The exact number of items to draw from the selected axis.

**frac** (floating-point)

The proportion of the selected axis to draw, computed as
`round(frac * axis_size)`. Values greater than `1.0` require `replace = true` (there
aren't enough distinct rows to draw more than the total without repeating some).

**replace** (`bool`, default `false`)

Whether the same position can be drawn more than once:

- `false` (the default): every drawn position is distinct — equivalent to shuffling
  the axis and taking the first `n` (or `frac`-implied count).
- `true`: positions are drawn independently with replacement, so the same row (or
  feature) can appear more than once in the result. Feature sampling
  (`axis = Axis::Feature`) does not support `replace = true` — that combination is a
  validation error.

**axis** (`Axis`, default `Axis::Case`)

Which axis to sample from. `Axis::Case` (the default) draws rows. `Axis::Feature`
draws columns instead, returning a frame with a random subset (or resampling, subject
to the `replace` restriction above) of the original features, each with all of its
data intact.

**random_state** (`std::uint64_t`)

An explicit seed for the underlying random engine. Omit it (use one of the shorter
overloads) to seed from `std::random_device` instead, which draws fresh entropy each
call and is never reproducible run-to-run.

## Returns

`DataFrame`
    A new frame containing the drawn subset, with the source frame's feature order,
    dtypes, and original case labels preserved for whichever rows/features were
    selected.

## Examples

Example 1. Draw a fixed number of rows without a seed (result varies each run).

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"order_id", Slate(pluss::make_series({101, 102, 103, 104, 105, 106}))},
    {"units",    Slate(pluss::make_series({2, 5, 1, 3, 4, 2}))},
});

auto three_rows = orders.sample(3);
std::cout << three_rows;
```

### Output (one possible draw)

```text
======================
     | order_id  units
----------------------
1    | 102       5    
3    | 104       3    
5    | 106       2    
----------------------
[3 cs x 2 feat]
```

The result always has exactly `3` rows, each a distinct case from `orders` (since
`replace` defaults to `false`), always rendered in the same box-table format any
`DataFrame` prints in. *Which* three rows, and in what order, is genuinely random
without a `random_state` — the table above is one possible draw, not a fixed result;
rerunning this exact code will draw a different set of rows almost every time, but it
will always be three distinct case labels pulled from `{0, 1, 2, 3, 4, 5}` with their
original `order_id`/`units` values intact.

Example 2. Draw with replacement, allowing more rows than the frame has.

```cpp
auto with_repeats = orders.sample(10, true, Axis::Case);
std::cout << with_repeats;
```

### Output (one possible draw)

```text
======================
     | order_id  units
----------------------
3    | 104       3    
0    | 101       2    
5    | 106       2    
5    | 106       2    
1    | 102       5    
2    | 103       1    
0    | 101       2    
4    | 105       4    
3    | 104       3    
1    | 102       5    
----------------------
[10 cs x 2 feat]
```

`orders` only has `6` rows, but `replace = true` allows drawing more than that — case
`5` and case `0` each happen to appear twice above, and case `3` and case `1` also
repeat, since with-replacement sampling can draw the same row more than once. The
specific repeats and their order are random and will differ run to run; what is fixed
is the shape (`10` rows) and that every value in the output traces back to one of
`orders`'s original `6` rows. Without `replace = true`, requesting more rows than
exist is not possible for `Axis::Case` sampling.

Example 3. Draw a proportion of the features instead of the cases.

```cpp
auto orders_wide = DataFrame({
    {"order_id",   Slate(pluss::make_series({101, 102, 103}))},
    {"units",      Slate(pluss::make_series({2, 5, 1}))},
    {"unit_price", Slate(pluss::make_series({1200.0, 45.5, 1150.0}))},
    {"discount",   Slate(pluss::make_series({0.1, 0.05, 0.08}))},
});

auto half_the_features = orders_wide.sample(0.5, false, Axis::Feature);
std::cout << half_the_features;
```

### Output (one possible draw)

```text
===========================
     | order_id  unit_price
---------------------------
0    | 101       1200      
1    | 102       45.5      
2    | 103       1150      
---------------------------
[3 cs x 2 feat]
```

`frac = 0.5` over `4` features draws `2` of them — here, `order_id` and `unit_price`
happened to be chosen, but `units` and `discount` are equally likely candidates on
another run. What is fixed is the shape: the result always keeps all `3` cases for
whichever features were chosen, since sampling `Axis::Feature` only decides which
*columns* participate, not which rows.

## Notes

- `sample()` is built on [`take()`](take.md); a drawn position's original case label
  is preserved in the result, exactly as `take()` would preserve it.
- A `random_state` reproduces the same draw on the same compiler/standard-library
  build, but is not guaranteed to reproduce identically across different platforms —
  don't rely on a specific seed producing byte-identical results in, say, CI running a
  different toolchain than a developer's laptop.
- `replace = true` is not supported for `Axis::Feature` sampling; every feature draw
  is always without replacement.
- `frac` values above `1.0` require `replace = true`, since there is no way to draw
  more distinct items than the axis contains.

## See Also

- [`DataFrame::take`](take.md): the deterministic positional-selection primitive this
  method draws random positions for.
- [`DataFrame::shuffle`](shuffle.md) / [`DataFrame::reshuffle`](reshuffle.md): a full
  random reordering of the frame instead of a subset.
- [`DataFrame::head`](head.md) / [`DataFrame::tail`](tail.md): deterministic
  positional alternatives when randomness isn't what you need.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rxor.html">
    <span class="label">Previous</span>
    <span class="title">rxor</span>
  </a>
  <a class="next-link" href="save_as.html">
    <span class="label">Next</span>
    <span class="title">save_as</span>
  </a>
</div>
