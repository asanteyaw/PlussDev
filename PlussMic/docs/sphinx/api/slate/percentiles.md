<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: af8339aada21 -->

# Slate::percentiles

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
Slate Slate::percentiles(pluss::RangeView<double> points = {0.25, 0.5, 0.75}, std::int64_t min_count = 1) const
```

## What It Does

Computes the value at each requested fractional position (`0.0` = minimum,
`1.0` = maximum, `0.5` = median) among the slate's observed values, using the
same linear-interpolation-between-ranks convention as
[`describe()`'s](describe.md) `25%`/`50%`/`75%` rows and
[`median()`](median.md). Missing entries are dropped first (via
[`dropnull()`](dropnull.md)). Returns a new slate — one entry per requested
point, in the order given — rather than a scalar `SlateValue`, so the
`0.5` point (for example) computes exactly the same value
[`median()`](median.md) would.

If there are no observed values at all (or fewer than `min_count`), every
requested point comes back **missing** rather than throwing — the underlying
storage is `0`, but it's marked invalid, so it renders and inspects as `NULL`
like any other missing entry.

## Parameters

**points** (`pluss::RangeView<double>`, default `{0.25, 0.5, 0.75}`)
    The fractional positions to evaluate, each in `[0, 1]`. Must be
    non-empty, or it throws.

**min_count** (`std::int64_t`, default `1`)
    The smallest number of observed values still considered valid. Below it,
    every requested point comes back missing rather than throwing.

## Returns

`Slate`
    A new slate, one entry per requested point (in the order given), holding
    the interpolated value at each fractional position.

## Examples

Example 1. Default quartiles.

```cpp
using namespace pluss::table;

auto scores = Slate(pluss::make_series({10.0, 20.0, 30.0, 40.0}));
auto out = scores.percentiles();
std::cout << out;
```

### Output

```text
0  17.5
1  25
2  32.5

ptype: Series
```

Position `1` (the `0.5` point) is exactly [`scores.median()`](median.md)'s
result — `25`.

Example 2. Custom points — the minimum and maximum, as `0.0` and `1.0`.

```cpp
auto out2 = scores.percentiles({0.0, 1.0});
std::cout << out2;
```

### Output

```text
0  10
1  40

ptype: Series
```

Example 3. Missing entries are excluded before computing.

```cpp
auto with_gap = Slate(
    pluss::make_series({10.0, 100.0, 20.0, 30.0, 40.0}),
    pluss::make_series(std::vector<bool>{true, false, true, true, true})
);
auto out3 = with_gap.percentiles();
std::cout << out3;
```

### Output

```text
0  17.5
1  25
2  32.5

ptype: Series
```

Same result as Example 1 — the missing `100.0` never enters the computation.

Example 4. No observed values (or fewer than `min_count`) — every point comes
back missing, rather than throwing.

```cpp
auto all_missing = Slate(
    pluss::make_series({1.0, 2.0}),
    pluss::make_series(std::vector<bool>{false, false})
);
auto out4 = all_missing.percentiles();
std::cout << out4;
```

### Output

```text
0  NULL
1  NULL
2  NULL

ptype: Series
```

Example 5. A point outside `[0, 1]` throws.

```cpp
try {
  auto bad = scores.percentiles({0.5, 1.5});
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slate::percentiles: points must be in [0, 1]
```

## Notes

- The `0.5` point always matches [`median()`](median.md) exactly, and every
  point matches the corresponding quartile row in
  [`describe()`](describe.md) when it happens to request `0.25`/`0.5`/`0.75`.
- No observed values (or a count below `min_count`) produces a fully-missing
  result, not a thrown error.
- `points` must be non-empty and every value in `[0, 1]`, or it throws.
- Restricted to numeric `Series` payloads; anything else throws
  `"Slate::percentiles: expected a real numeric Series Slate"`.

## See Also

- [`Slate::median`](median.md): the special case of this method at the `0.5`
  point.
- [`Slate::describe`](describe.md): a wider numeric summary that includes the
  default quartiles.
- [`Slate::rank`](rank.md): assigns a rank to each entry, the inverse
  question to evaluating a percentile.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="partition.html">
    <span class="label">Previous</span>
    <span class="title">partition</span>
  </a>
  <a class="next-link" href="plot.html">
    <span class="label">Next</span>
    <span class="title">plot</span>
  </a>
</div>
