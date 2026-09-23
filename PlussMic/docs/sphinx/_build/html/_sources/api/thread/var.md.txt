<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f343545e9767 -->

# Thread::var

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::var(bool unbiased = true) const -> ThreadValue
Thread::var(const StatisticOptions& options) const -> ThreadValue
```

## What It Does

Returns the variance of every observed (non-missing) value in a numeric
`Series` thread, as a scalar `ThreadValue`. There are two overloads, and they
handle an insufficient sample differently:

- **`var(unbiased)`**: the simple form. `unbiased = true` (the default)
  computes the **sample** variance (dividing by `n - 1`, Bessel's correction);
  `unbiased = false` computes the **population** variance (dividing by `n`).
  An empty thread, or one where every entry is missing, **throws**.
- **`var(options)`**: the general form, taking a `StatisticOptions{ddof,
  min_count}`. `ddof` (delta degrees of freedom) generalizes the divisor to
  `n - ddof` — `ddof = 1` matches `unbiased = true`, `ddof = 0` matches
  `unbiased = false`, and any other non-negative value is accepted too.
  Instead of throwing when the observed count is too small, this overload
  **returns a missing (`NULL`) result** if the observed count is below
  `min_count` (and always below `ddof + 1`, since a divisor of `0` or less
  would be undefined).

See [`Thread::sum`'s note on printing a `ThreadValue`](sum.md) — every example
below uses `.to_dbl()` for a clean number instead of streaming the
`ThreadValue` directly.

## Parameters

**unbiased** (`bool`, default `true`)
    `true`: sample variance (`n - 1` divisor). `false`: population variance
    (`n` divisor).

**options** (`StatisticOptions`)
    `ddof` (default `1`): the divisor is `n - ddof`. `min_count` (default
    `1`): the smallest observed count still considered valid; below it, the
    result is `NULL` rather than a thrown error.

## Returns

`ThreadValue`
    A scalar `Float64` (or `Float32` off-CPU) holding the variance.

## Examples

Example 1. Sample variance (the default) — divides by `n - 1`.

```cpp
using namespace pluss::table;

auto values = Thread(pluss::make_series({2.0, 4.0, 6.0, 8.0}));
auto out = values.var();
std::cout << out.to_dbl();
```

### Output

```text
6.66667
```

Example 2. Population variance — divides by `n` instead.

```cpp
auto out2 = values.var(false);
std::cout << out2.to_dbl();
```

### Output

```text
5
```

Example 3. The `unbiased`-form throws on an empty (all-missing) selection.

```cpp
auto all_missing = Thread(
    pluss::make_series({1.0, 2.0}),
    pluss::make_series(std::vector<bool>{false, false})
);
try {
  auto bad = all_missing.var();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread::var on an empty (all-null) selection
```

Example 4. The `StatisticOptions`-form returns `NULL` instead of throwing when
the observed count is below `min_count` — but note that streaming a
`ThreadValue` (or reading `.to_dbl()`) does **not** by itself tell you whether
the result is valid. Check validity through `.thread()` first.

```cpp
auto small = Thread(pluss::make_series({10.0, 20.0}));
auto out4 = small.var(StatisticOptions{.ddof = 1, .min_count = 5});
std::cout << out4.thread().notnull().as_series().to_vec<bool>()[0];
```

### Output

```text
false
```

Only two observations are present, below the required `min_count = 5`, so the
single-entry result thread is marked invalid at position `0` — no exception is
thrown, but `.to_dbl()` would still hand back whatever number the arithmetic
happened to produce underneath (`50`, in this case: `Series`/`ThreadValue`
printing has no concept of nullness, only the wrapping `Thread`'s own validity
mask does). Always check the scalar validity mask (or wrap the result and use
[`isnull()`](isnull.md)) before trusting a `StatisticOptions`-gated result.

## Notes

- Missing entries are excluded before computing, in both overloads.
- The `unbiased`-form throws on an empty/all-missing selection; the
  `options`-form returns `NULL` below `min_count` instead of throwing —
  choose whichever failure mode fits your code.
- `ddof = 1` (the `options`-form's default) matches `unbiased = true`; `ddof =
  0` matches `unbiased = false`.
- Restricted to `Series` payloads; anything else throws `"Thread::var is only
  valid for Series payload"`.

## See Also

- [`Thread::std`](std.md): the square root of this method's result.
- [`Thread::se`](se.md): the standard error of the mean, built on the same
  variance computation.
- [`Thread::mean`](mean.md): the center this method measures spread around.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="value.html">
    <span class="label">Previous</span>
    <span class="title">value</span>
  </a>
  <a class="next-link" href="view.html">
    <span class="label">Next</span>
    <span class="title">view</span>
  </a>
</div>
