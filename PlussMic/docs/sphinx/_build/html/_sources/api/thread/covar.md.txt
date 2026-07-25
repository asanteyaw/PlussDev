<!-- pluss-hand-authored -->

# Thread::covar

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::covar(const Thread& other, const StatisticOptions& options = {}) const -> ThreadValue
```

## What It Does

Computes the covariance between this thread and `other`, as a scalar
`ThreadValue` — the unnormalized counterpart to [`corr()`](corr.md) (`corr()`
is covariance divided by the product of the two standard deviations). Both
threads must be numeric `Series`. If either thread carries explicit case
labels, the two are aligned by matching label first — see
[`corr()`'s important callout](corr.md) on how much this can change the
result. `ddof` (in `options`) controls the divisor, exactly like
[`var()`](var.md)'s `StatisticOptions` overload: `ddof = 1` (the default)
gives the sample covariance (`n - 1` divisor); `ddof = 0` gives the
population covariance (`n` divisor). Below `min_count` valid pairs, the result
is marked *invalid* rather than thrown — see
[`Thread::sum`'s note](sum.md) on checking `.thread().is_valid(0)` rather than
trusting the printed number in that case.

Unlike `corr()`, this method requires both threads to already share the same
platform (CPU/CUDA/MPS) — there's no automatic host-side conversion here.

## Parameters

**other** (`const Thread&`)
    The numeric `Series` thread to compute covariance with.

**options** (`StatisticOptions`, default `{}`)
    `ddof` (default `1`): the divisor is `n - ddof` over the valid pairs.
    `min_count` (default `1`): the smallest number of valid pairs still
    considered valid; below it (and always below `ddof + 1`), the result is
    marked invalid.

## Returns

`ThreadValue`
    A scalar `Float64` (or `Float32` off-CPU) holding the covariance.

## Examples

Example 1. Sample covariance (the default) between two related threads.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0}));
auto other = Thread(pluss::make_series({10.0, 10.0, 35.0, 45.0}));
auto out = scores.covar(other);
std::cout << out.to_dbl();
```

### Output

```text
216.667
```

Example 2. Population covariance — `ddof = 0` instead of the default `1`.

```cpp
auto out2 = scores.covar(other, StatisticOptions{.ddof = 0});
std::cout << out2.to_dbl();
```

### Output

```text
162.5
```

Example 3. Below `min_count` valid pairs, the result is marked invalid rather
than thrown.

```cpp
auto small = Thread(pluss::make_series({1.0, 2.0}));
auto small_other = Thread(pluss::make_series({3.0, 4.0}));
auto out3 = small.covar(small_other, StatisticOptions{.ddof = 1, .min_count = 5});
std::cout << out3.thread().is_valid(0);
```

### Output

```text
false
```

## Notes

- If either thread carries explicit case labels, pairing is by matching
  label — see [`corr()`'s callout](corr.md).
- `ddof = 1` (the default) gives the sample covariance; `ddof = 0` gives the
  population covariance.
- Below `min_count` valid pairs, the result is marked invalid rather than
  thrown.
- Both threads must already share the same platform, or it throws
  `"Thread::covar: operands must share a platform"` — unlike `corr()`, this
  method doesn't convert to a common host representation first.
- Restricted to numeric `Series` payloads; anything else throws
  `"Thread::covar: expected a real numeric Series Thread"`.

## See Also

- [`Thread::corr`](corr.md): the normalized counterpart — a value in `[-1,
  1]` instead of covariance's unbounded scale.
- [`Thread::var`](var.md): the special case of `covar()` with `other` equal to
  the thread itself.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="count.html">
    <span class="label">Previous</span>
    <span class="title">count</span>
  </a>
  <a class="next-link" href="csmax.html">
    <span class="label">Next</span>
    <span class="title">csmax</span>
  </a>
</div>
