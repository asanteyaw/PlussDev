<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2e2ee5483868 -->

# DataFrame::corr_with

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::corr_with(const DataFrame& other, CorrMethod method = CorrMethod::Pearson, std::int64_t min_count = 1) const -> Slate
DataFrame::corr_with(pluss::msharp::RangeView::Of<DataFrame> others, CorrMethod method = CorrMethod::Pearson, std::int64_t min_count = 1) const -> DataFrame
```

## What It Does

Correlates this frame against one or more *peer* frames, feature-by-feature under the
*same* name — the cross-frame counterpart to [`corr()`](corr.md)'s within-frame,
feature-against-feature matrix. For each feature name shared by both frames (matched
by name, restricted to numeric `Series` features on both sides), `corr_with()`
computes a single correlation value between this frame's column and `other`'s
same-named column, treating each row (matched by case label, restricted to labels
present on both sides) as one paired observation. The result is a compact, one-value-
per-shared-feature summary — not a full N×N matrix, since there's exactly one
sensible comparison per shared name.

The single-frame overload returns a `Slate` whose case labels *are* the shared
feature names; the multi-frame overload runs the same computation once per peer frame
and lays the results side by side as columns named `frame_0`, `frame_1`, ... in the
order `others` was given.

## Parameters

**other** (`const DataFrame&`, single-peer overload)

The one frame to correlate against, feature-by-feature under matching names.

**others** (`pluss::RangeView<DataFrame>`, multi-peer overload)

Several frames to correlate against in one call; each becomes one output column,
named positionally (`frame_0`, `frame_1`, ...) rather than by any property of the
peer frame itself. Throws `"DataFrame::corr_with: expected at least one comparison
frame"` if empty.

**method** (`CorrMethod`, default `Pearson`)

`Pearson`, `Spearman`, or `KendallTau` — the same three correlation statistics
[`corr()`](corr.md) supports, with the same tradeoffs described there.

**min_count** (`std::int64_t`, default `1`)

Minimum number of paired, non-null observations required for a feature's correlation
to be computed at all; must be `>= 1`.

## Returns

`Slate`
    (single-peer overload) One value per feature name shared by both frames, case-
    labeled by feature name. Throws `"DataFrame::corr_with: no common numeric
    features"` if this frame and `other` share no numeric feature name at all.

`DataFrame`
    (multi-peer overload) One column per peer frame (`frame_0`, `frame_1`, ...), each
    holding the same per-feature correlation values as the single-peer overload would
    for that one comparison — including the same `"no common numeric features"` throw,
    per peer frame, since each column is computed via one single-peer call.

## Examples

Example 1. Correlate against one peer frame, feature-by-feature.

```cpp
using namespace pluss::table;

auto this_run = DataFrame({
    {"x", Slate(pluss::make_series({1.0, 2.0, 3.0, 4.0}))},
    {"y", Slate(pluss::make_series({2.0, 4.0, 6.0, 8.0}))},
});

auto other_run = DataFrame({
    {"x", Slate(pluss::make_series({2.0, 4.0, 5.0, 8.0}))},
    {"y", Slate(pluss::make_series({1.0, 3.0, 2.0, 4.0}))},
});

auto out = this_run.corr_with(other_run);
std::cout << out;
```

### Output

```text
x  0.981155781
y  0.8
ptype: Series
```

`x` correlates almost perfectly (`0.981...`) between the two runs; `y` correlates more
loosely (`0.8`). Each value compares *this frame's* `x` against *the other frame's*
`x`, row-for-row — not `this_run.x` against `this_run.y`, which is what
[`corr()`](corr.md) would give you instead.

Example 2. Correlate against two peer frames at once, one of which is a perfect
mirror-image (and therefore perfectly *anti*-correlated).

```cpp
auto mirrored_run = DataFrame({
    {"x", Slate(pluss::make_series({4.0, 3.0, 2.0, 1.0}))},
    {"y", Slate(pluss::make_series({8.0, 6.0, 4.0, 2.0}))},
});

std::array<DataFrame, 2> peers{other_run, mirrored_run};
auto out = this_run.corr_with(pluss::msharp::RangeView::Of<DataFrame>(peers));
std::cout << out;
```

### Output

```text
===========================
     | frame_0      frame_1
---------------------------
x    | 0.981155781  -1     
y    | 0.8          -1     
---------------------------
[2 cs x 2 feat]
```

`frame_0` repeats Example 1's result against `other_run`; `frame_1` compares against
`mirrored_run`, whose `x` and `y` are exact reversals of `this_run`'s — an exact
linear anti-correlation, so both features come back at exactly `-1`.

Example 3. Pass `method` explicitly, and see `Pearson` and `Spearman` disagree
on a relationship that is monotonic but not linear.

```cpp
auto this_mono = DataFrame({
    {"x", Slate(pluss::make_series({1.0, 2.0, 3.0, 4.0}))},
});
auto other_mono = DataFrame({
    {"x", Slate(pluss::make_series({1.0, 4.0, 9.0, 16.0}))},
});

auto pearson_out = this_mono.corr_with(other_mono, DataFrame::CorrMethod::Pearson);
std::cout << pearson_out;

auto spearman_out = this_mono.corr_with(other_mono, DataFrame::CorrMethod::Spearman);
std::cout << spearman_out;
```

### Output (Pearson)

```text
x  0.9843740387
ptype: Series
```

### Output (Spearman)

```text
x  1
ptype: Series
```

`other_mono.x` is `this_mono.x` squared — strictly increasing, so every rank in
one sequence corresponds exactly to the same rank in the other, and `Spearman`
(rank correlation) reports a perfect `1`. `Pearson` sees the same relationship
but measures straight-line fit, and `x²` is not a straight line, so it comes
back lower at `0.984...` — the same distinction [`corr()`](corr.md) describes for
its own `method` parameter.

Example 4. Two throws: no shared numeric feature name between the frames
(single-peer overload), and an empty peer list (multi-peer overload).

```cpp
auto disjoint = DataFrame({
    {"z", Slate(pluss::make_series({1.0, 2.0, 3.0, 4.0}))},
});

try {
  auto out = this_run.corr_with(disjoint);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}

try {
  std::array<DataFrame, 0> empty_peers{};
  auto out = this_run.corr_with(pluss::msharp::RangeView::Of<DataFrame>(empty_peers));
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
DataFrame::corr_with: no common numeric features
DataFrame::corr_with: expected at least one comparison frame
```

`this_run` only has `x` and `y`; `disjoint` only has `z` — with no shared
feature name at all, there is nothing to correlate, so the single-peer overload
rejects the call outright rather than returning an empty `Slate`. The
multi-peer overload separately rejects an empty `others` list before looking at
feature names at all.

## Notes

- Only features present, by name, on *both* frames participate; a feature unique to
  either side is silently skipped, never padded with `NULL`.
- Rows are matched by case-label value before the statistic is computed — this is a
  case-aware comparison, not a blind positional zip of the two columns.
- The multi-peer overload's output columns are always named positionally
  (`frame_0`, `frame_1`, ...); there is no way to have it adopt a peer frame's
  `title()` or some other identifying name automatically.

## See Also

- [`DataFrame::corr`](corr.md): the within-frame correlation matrix this method's
  single-peer computation is built from.
- [`DataFrame::covar`](covar.md): the covariance-scale analogue of `corr()`.
- [`DataFrame::combine`](combine.md): merge two frames' features together directly,
  rather than only comparing them statistically.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="corr.html">
    <span class="label">Previous</span>
    <span class="title">corr</span>
  </a>
  <a class="next-link" href="count.html">
    <span class="label">Next</span>
    <span class="title">count</span>
  </a>
</div>
