<!-- pluss-hand-authored -->

# Series::lcm

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::lcm(const Series& other) const
Series& Series::lcm_(const Series& other)
Series& Series::lcm_(const Series& rhs) const
```

## What It Does

`lcm()` finds the first non-negative multiple shared by each aligned pair of
integers. If `a` describes a task that repeats every four ticks and `b` one that
repeats every six ticks, `lcm(a, b)` is twelve: the first tick at which those two
cycles meet again.

The operation is elementwise and broadcastable. It does not reduce a whole
series to one scheduling period. That distinction lets a row of reference
periods be compared with every row in a table of observed periods in one call.

For nonzero pairs, GCD and LCM divide the same information in opposite ways:

```
gcd(a, b) * lcm(a, b) = |a * b|
```

Conceptually, `lcm(a, b)` is `|a / gcd(a, b) * b|`; the divide-first form
explains which common factors are removed before the remaining factors are
combined. If either input is zero, the result is defined as zero.

Conventions, matching `std::lcm`:

- the result is always `>= 0`, regardless of the signs of the inputs
- `lcm(x, 0) == 0` for any `x`

Integer-only: a floating-point or complex input raises.

## Parameters

**other** (`const Series&`): the second integer operand, broadcast against the
receiver. The result uses the promoted integer dtype of both operands.

## Returns

`Series` or `Series&`
    A new series containing the non-negative pairwise multiples, with the
    broadcast shape and promoted integer dtype. `lcm_()` writes the result into
    the receiver while preserving its storage, shape, and dtype.

Raises if either operand is a floating-point or complex series.

## Gradient

`lcm()` is a discrete integer operation and is **not differentiable**.

## In-Place Form

`lcm_()` is the in-place sibling of [`lcm()`](lcm.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `lcm_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series(std::vector<std::int64_t>{4, 6}, {2});
auto y = pluss::make_series(std::vector<std::int64_t>{6, 9}, {2});

x.lcm_(y);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[12, 18]
```

## Examples

Example 1. `lcm(x, 0)` is `0`; signs are dropped.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({4, 6, 0}).lcm(pluss::make_series({6, 8, 5})));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[12, 24, 0]
```

`lcm(4, 6) = 12`, `lcm(6, 8) = 24`, `lcm(0, 5) = 0`.

Example 2. One row of cycle lengths can be broadcast across many schedules.

```cpp
auto observed = pluss::make_series({{-12, 18, 0}, {20, -27, 14}});
auto reference = pluss::make_series({{8, 6, 5}});

pluss::series::print(std::cout, observed.lcm(reference));
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=
⎡ 24 18  0 ⎤
⎣ 40 54 70 ⎦
```

Read the first column as two independent synchronization questions. Periods 12
and 8 next coincide at 24; periods 20 and 8 next coincide at 40. In the third
column, the zero period makes the first answer zero by convention, while 14 and
5 are coprime and therefore require the full product, 70.

Example 3. Use `gcd()` and `lcm()` together when both the shared unit and the
shared cycle matter.

```cpp
auto a = pluss::make_series({12, 18, 14});
auto b = pluss::make_series({8, 6, 5});

pluss::series::print(std::cout, a.gcd(b)) << '\n';
pluss::series::print(std::cout, a.lcm(b));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[4, 6, 1]
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[24, 18, 70]
```

The GCD gives the largest exact unit common to each pair. The LCM gives the
smallest complete span containing both periods. These are complementary
questions, so neither method substitutes for the other.

## Gotchas And Failure Modes

- Integer inputs only; a float operand raises.
- `lcm(x, 0)` is `0`, which is a valid multiple of everything.
- The result is never negative.
- Large valid answers can exceed the promoted integer dtype. Widen narrow
  operands with `to(ScalarType::Int64)` before the call, and remember that even
  `int64` has a finite range.
- Broadcasting may produce a shape larger than the receiver. That works for
  `lcm()`, but not for `lcm_()`, which must preserve receiver storage and shape.
- Not differentiable.

## Summary

`lcm(a, b)` is the elementwise least common multiple:

- `|a * b| / gcd(a, b)`; `lcm(x, 0) = 0`; result `>= 0`
- integer inputs only
- not differentiable
- `lcm_()` is the in-place form

## See Also

- [`Series::gcd`](gcd.md): the greatest common divisor this is built from.
- [`Series::factor`](factor.md): the prime factorization behind both.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="kurtosis.html">
    <span class="label">Previous</span>
    <span class="title">kurtosis</span>
  </a>
  <a class="next-link" href="ldexp.html">
    <span class="label">Next</span>
    <span class="title">ldexp</span>
  </a>
</div>
