<!-- pluss-hand-authored -->

# Series::random_

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series& Series::random_() const
Series& Series::random_(std::int64_t to) const
Series& Series::random_(std::int64_t from, std::int64_t to) const
```

## What It Does

Overwrites every element of the receiver with a uniformly chosen **integer**:

- `random_()` — from `[0, upper)` where `upper` depends on the receiver
  dtype (see the table below).
- `random_(to)` — from `[0, to)`.
- `random_(from, to)` — from `[from, to)`.

The interval is half-open: `from` is reachable, `to` is not. The draw is
always integer-valued; if the receiver is floating-point, the integers are
stored as floating values like `5.0` — `random_` never becomes continuous
sampling. This makes it the right primitive for IDs, offsets, bucket
indices, and categorical codes. For a continuous fill, use
[`uniform_`](uniform_.md).

The no-argument `upper` per dtype:

| receiver dtype | `random_()` range |
|---|---|
| `Bool` | `[0, 2)` — i.e. `{0, 1}` |
| `Int8` | `[0, 128)` |
| `UInt8` | `[0, 256)` |
| `Int16` / `UInt16` | `[0, 32768)` / `[0, 65536)` |
| `Int32` / `UInt32` | `[0, 2^31)` / `[0, 2^32)` |
| `Float16` / `BFloat16` / `Float32` | `[0, 2^24)` |
| `Int64` / `UInt64` / `Float64` | `[0, 2^53)` |

### Reproducibility

Deterministic given the process-wide RNG seed. Call `pluss::rng_seed(seed)`
before sampling to fix the values; two runs from the same seed produce
`equal` tensors. Each call advances a stream counter.

## Parameters

**from** (`std::int64_t`): the (inclusive) lower bound. Defaults to `0`.

**to** (`std::int64_t`): the (exclusive) upper bound. Must be `> from`, and
the whole `[from, to)` range must be representable in the receiver dtype.

## Returns

`Series&`
    A reference to the receiver, now holding the integer draws. The write is
    in-place and preserves storage identity — every alias sees the values.

## Raises

- if `to <= from`
- if `[from, to)` does not fit the receiver dtype
- if the receiver's dtype is complex

## Gradient

Not differentiable.

## Examples

Example 1. `random_(to)` — integers in `[0, to)`. Seeded, so fixed.

```cpp
pluss::rng_seed(42);
auto x = pluss::series::empty({12}, pluss::ScalarType::Int64);
x.random_(10);
pluss::series::print(std::cout, x);
```

### Output

```text
Series(shape=[12], strides=[1], dtype=int64, platform=cpu, numel=12, contiguous=true)
data=[8, 1, 2, 2, 5, 5, 7, 7, 9, 2, 0, 8]
```

Every value is in `0..9`; `10` never appears.

Example 2. `random_(from, to)` — a signed range, with `to` excluded.

```cpp
pluss::rng_seed(7);
auto x = pluss::series::empty({12}, pluss::ScalarType::Int64);
x.random_(-2, 3);
pluss::series::print(std::cout, x);
```

### Output

```text
Series(shape=[12], strides=[1], dtype=int64, platform=cpu, numel=12, contiguous=true)
data=[-1, 0, 0, 0, 0, 0, 1, 1, -2, -1, 2, 2]
```

The draws come from `{-2, -1, 0, 1, 2}` — `3` is out.

Example 3. On a floating receiver the draws are still integers, stored as
`X.0`.

```cpp
auto x = pluss::series::empty({8}, pluss::ScalarType::Float64);
x.random_(5, 6);
pluss::series::print(std::cout, x);
```

### Output

```text
Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[5.0000, 5.0000, 5.0000, 5.0000, 5.0000, 5.0000, 5.0000, 5.0000]
```

Only integer `5` is in `[5, 6)`. This is *not* the same as
`uniform_(5.0, 6.0)`, which would fill with continuous values.

Example 4. A large sample from `[0, 100)` is uniform over the 100 integers.

```cpp
pluss::rng_seed(3);
auto x = pluss::series::empty({200000}, pluss::ScalarType::Int32);
x.random_(0, 100);
auto v = x.to_vec<int>();
long lo = 1'000'000'000, hi = -1'000'000'000; double sum = 0;
for (int e : v) { lo = std::min<long>(lo, e); hi = std::max<long>(hi, e); sum += e; }
std::cout << "min=" << lo << "  max=" << hi << "  mean~" << sum / v.size() << "  (expect ~49.5)\n";
```

### Output

```text
min=0  max=99  mean~49.466
```

Example 5. The range must fit the dtype; complex receivers and a
non-increasing range are rejected.

```cpp
try {
  auto i8 = pluss::series::empty({3}, pluss::ScalarType::Int8);
  i8.random_(0, 300);              // 300 > INT8_MAX
} catch (const std::exception& e) {
  std::string m = e.what();
  std::cout << m.substr(0, m.find('\n')) << "\n";
}
try {
  auto c = pluss::series::empty({3}, pluss::ScalarType::Complex64);
  c.random_(0, 5);
} catch (const std::exception& e) {
  std::string m = e.what();
  std::cout << m.substr(0, m.find('\n')) << "\n";
}
try {
  auto i = pluss::series::empty({3}, pluss::ScalarType::Int64);
  i.random_(5, 5);
} catch (const std::exception& e) {
  std::string m = e.what();
  std::cout << m.substr(0, m.find('\n')) << "\n";
}
```

### Output

```text
random_: requested range is outside the receiver dtype
random_: complex receivers are not supported
random_: expected hi > lo
```

Example 6. Reproducible under a fixed seed.

```cpp
pluss::rng_seed(9);
auto a = pluss::series::empty({6}, pluss::ScalarType::Int64); a.random_(0, 1000);
pluss::rng_seed(9);
auto b = pluss::series::empty({6}, pluss::ScalarType::Int64); b.random_(0, 1000);
std::cout << std::boolalpha << "a == b: " << a.equal(b) << "\n";
```

### Output

```text
a == b: true
```

## Gotchas And Failure Modes

- `random_` is **integer**-valued, always. On a floating receiver the values
  are whole numbers stored as floats — it is not
  [`uniform_`](uniform_.md).
- `to` is excluded. `random_(n)` draws from `0..n-1`.
- The whole `[from, to)` range must be representable in the receiver dtype —
  a wide range into a small integer type, or into a low-precision float that
  cannot hold every distinct integer, is rejected.
- Complex receivers are rejected (no ordered integer interval for complex).
- Reproducible only after `pluss::rng_seed(seed)`; each call advances the
  stream.
- Not differentiable.

## Summary

`random_()` / `random_(to)` / `random_(from, to)` fills the receiver in place
with uniform integers from a half-open range:

- always integer-valued, even on a floating receiver (stored as `X.0`)
- default range depends on the receiver dtype (see the table)
- `to` excluded; range must fit the dtype; complex rejected
- reproducible under `pluss::rng_seed(seed)`; not differentiable; aliases
  see the write

## See Also

- [`Series::uniform_`](uniform_.md): the continuous counterpart —
  `[from, to)` over the reals, not the integers.
- [`Series::multinomial`](multinomial.md): non-uniform category selection.
- [`pluss::series::randint`](../series_factory/randint.md): create an
  integer tensor instead of filling one.
- [`Series::bernoulli_`](bernoulli_.md): the `{0, 1}` special case with a
  tunable probability.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="quantile.html">
    <span class="label">Previous</span>
    <span class="title">quantile</span>
  </a>
  <a class="next-link" href="rad2deg.html">
    <span class="label">Next</span>
    <span class="title">rad2deg</span>
  </a>
</div>
