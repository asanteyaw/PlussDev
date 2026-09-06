<!-- pluss-hand-authored -->

# Series::multinomial

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::multinomial() const
Series Series::multinomial(std::int64_t num_samples) const
Series Series::multinomial(std::int64_t num_samples, bool replacement) const
```

## What It Does

Treats the **last axis** of the receiver as a list of category weights and
returns sampled **category indices** — not probabilities, not one-hot
vectors, just `int64` positions along that axis.

- A **1-D** receiver is one distribution; the result is `[num_samples]`.
- A **2-D** receiver is one distribution per row; rows are sampled
  independently and stay rows: `[rows, categories]` → `[rows, num_samples]`.

Weights do **not** need to sum to `1` — only their ratios matter, so
`[1, 2, 7]` and `[0.1, 0.2, 0.7]` sample identically. A zero-weight category
is never chosen. Negative, `NaN`, infinite, or all-zero rows are rejected,
not repaired.

The overloads:

| call | `num_samples` | `replacement` |
|---|---|---|
| `multinomial()` | `1` | `true` |
| `multinomial(n)` | `n` | `false` |
| `multinomial(n, r)` | `n` | `r` |

### Reproducibility

Deterministic given the process-wide RNG seed. Call `pluss::rng_seed(seed)`
before sampling to fix the indices; two runs from the same seed produce
`equal` tensors. Each call advances a stream counter.

## Parameters

**num_samples** (`std::int64_t`): how many indices to draw per distribution.
Must be positive. With `replacement = false` it cannot exceed the number of
categories — nor, in any row, the number of positive-weight categories.

**replacement** (`bool`): with `true`, a category may be drawn repeatedly and
`num_samples` can exceed the category count. With `false`, each drawn
category is removed from that row's remaining choices.

## Returns

`Series`
    An `int64` tensor of category indices. Shape `[num_samples]` for a 1-D
    receiver, `[rows, num_samples]` for a 2-D one.

## Raises

- if the receiver rank is not `1` or `2`
- if the receiver dtype is not floating-point
- if `num_samples <= 0`
- if any row has a non-positive sum, or contains a negative / non-finite
  weight
- if `replacement = false` and `num_samples` exceeds the (positive-weight)
  category count

## Gradient

Not differentiable — discrete indices carry no derivative back to the
weights.

## Examples

Example 1. 1-D weights → indices, with replacement. Weights `[1, 2, 7]` favor
category `2`. Seeded, so fixed.

```cpp
pluss::rng_seed(42);
auto w = pluss::make_series({1.0, 2.0, 7.0});
pluss::series::print(std::cout, w.multinomial(12, true));
```

### Output

```text
Series(shape=[12], strides=[1], dtype=int64, platform=cpu, numel=12, contiguous=true)
data=[2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 2, 2]
```

Category `2` carries `7/10` of the weight, so it dominates a small draw.

Example 2. Over a large sample the empirical frequencies match the
normalized weights.

```cpp
pluss::rng_seed(7);
auto w = pluss::make_series({1.0, 2.0, 7.0});
auto s = w.multinomial(100000, true);
auto v = s.to_vec<long long>();
long c0 = 0, c1 = 0, c2 = 0;
for (auto x : v) { if (x == 0) ++c0; else if (x == 1) ++c1; else ++c2; }
std::cout << "freq: " << c0 / 1e5 << " " << c1 / 1e5 << " " << c2 / 1e5
          << "  (weights normalize to 0.1 0.2 0.7)\n";
```

### Output

```text
freq: 0.09959 0.20101 0.6994  (weights normalize to 0.1 0.2 0.7)
```

Example 3. A 2-D receiver samples each row independently. Here each row has a
single possible category, so the result is deterministic.

```cpp
pluss::rng_seed(1);
auto w = pluss::make_series({{0.0, 1.0, 0.0},
                             {1.0, 0.0, 0.0}});
pluss::series::print(std::cout, w.multinomial(4, true));
```

### Output

```text
Series(shape=[2,4], strides=[4,1], dtype=int64, platform=cpu, numel=8, contiguous=true)
data=
⎡ 1 1 1 1 ⎤
⎣ 0 0 0 0 ⎦
```

Row `0` can only pick category `1`; row `1` only category `0`.

Example 4. `replacement = false` gives distinct indices — and `num_samples`
cannot exceed the category count.

```cpp
pluss::rng_seed(2);
auto w = pluss::make_series({1.0, 3.0, 6.0});
pluss::series::print(std::cout, w.multinomial(3, false));
try {
  auto bad = pluss::make_series({1.0, 3.0, 6.0}).multinomial(5, false);
} catch (const std::exception& e) {
  std::string m = e.what();
  std::cout << m.substr(0, m.find('\n')) << "\n";
}
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[0, 2, 1]multinomial: cannot sample more entries than categories without replacement
```

All three indices are distinct; the weights bias which comes first, and each
chosen category's weight is removed before the next draw.

Example 5. All-zero, negative-weight, and non-floating inputs are all
rejected.

```cpp
try {
  pluss::make_series({0.0, 0.0}).multinomial(1);
} catch (const std::exception& e) {
  std::string m = e.what();
  std::cout << m.substr(0, m.find('\n')) << "\n";
}
try {
  pluss::make_series({1.0, -1.0}).multinomial(1);
} catch (const std::exception& e) {
  std::string m = e.what();
  std::cout << m.substr(0, m.find('\n')) << "\n";
}
try {
  pluss::make_series({1, 2, 3}).multinomial(1);   // int64 weights
} catch (const std::exception& e) {
  std::string m = e.what();
  std::cout << m.substr(0, m.find('\n')) << "\n";
}
```

### Output

```text
multinomial: each probability row must have a positive sum
multinomial: probabilities must be finite and nonnegative
multinomial: probabilities must have a floating-point dtype
```

Example 6. Reproducible under a fixed seed.

```cpp
pluss::rng_seed(9);
auto a = pluss::make_series({1.0, 2.0, 3.0, 4.0}).multinomial(6, true);
pluss::rng_seed(9);
auto b = pluss::make_series({1.0, 2.0, 3.0, 4.0}).multinomial(6, true);
std::cout << std::boolalpha << "a == b: " << a.equal(b) << "\n";
```

### Output

```text
a == b: true
```

## Gotchas And Failure Modes

- The result is **indices**, `int64`, along the last axis — not
  probabilities, not one-hot rows.
- Weights are relative. They do not need to sum to `1`; a row of all zeros is
  an error, not "uniform".
- `replacement = false` capacity: `num_samples` must not exceed the category
  count, *or* the number of positive-weight categories in any row.
- The receiver rank must be exactly `1` or `2`, and the dtype must be
  floating-point.
- Reproducible only after `pluss::rng_seed(seed)`; each call advances the
  stream.
- Not differentiable.

## Summary

`multinomial(num_samples, replacement = false)` draws category indices from
last-axis weights:

- 1-D → `[num_samples]`; 2-D → `[rows, num_samples]`, rows independent
- weights are relative and must be nonnegative with a positive row sum
- `replacement = false` gives distinct indices, capped by the category count
- `int64` result; reproducible under `pluss::rng_seed(seed)`; not
  differentiable

## See Also

- [`Series::bernoulli`](bernoulli.md): independent binary events (the
  two-category, one-sample special case is close in spirit).
- [`Series::random_`](random_.md): *uniform* integers over a range — no
  weighting.
- [`pluss::series::multinomial`](../series_factory/multinomial.md): the
  free-function form.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mul.html">
    <span class="label">Previous</span>
    <span class="title">mul</span>
  </a>
  <a class="next-link" href="mvlgamma.html">
    <span class="label">Next</span>
    <span class="title">mvlgamma</span>
  </a>
</div>
