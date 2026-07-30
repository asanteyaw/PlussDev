<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 027ab6b58d74 -->

# Thread::truediv

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::truediv(double other) const -> Thread
Thread::truediv(const Thread& other) const -> Thread
```

## What It Does

Elementwise division that always promotes to floating point first — internally,
both operands are converted to `Float64` and divided via the library's
`true_divide()` primitive, regardless of what numeric type the source thread holds.
This is a stricter guarantee than [`div()`](div.md), which delegates to the raw
`Series::div()` algebra and may not promote integer types the same way. For the
`Float64`-typed examples used throughout this reference, `truediv()` and `div()`
produce identical results.

## Parameters

**other** (`double` or `const Thread&`)

The divisor(s). For the `Thread` overload, must be the same length as `*this`.

## Returns

`Thread`
    A new `Float64` thread, same length as `*this`, holding the elementwise quotient.

## Examples

Example 1. Divide every value by a scalar.

```cpp
using namespace pluss::table;

auto prices = Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto out = prices.truediv(4.0);
std::cout << out;
```

### Output

```text
0  2.5
1  5
2  7.5

ptype: Series
```

Example 2. Divide two threads positionally.

```cpp
auto splits = Thread(pluss::make_series({4.0, 8.0, 9.0}));
auto out_pair = prices.truediv(splits);
std::cout << out_pair;
```

### Output

```text
0  2.5
1  2.5
2  3.333333333

ptype: Series
```

Both examples match [`div()`](div.md)'s results exactly for this `Float64` data — the
distinction between the two methods only shows up on integer-typed threads.

## Notes

- See [`add()`](add.md) for the family-wide mechanics: positional matching and the
  length-mismatch throw.
- Prefer `truediv()` over [`div()`](div.md) when you want a guaranteed
  floating-point result regardless of the source thread's underlying numeric type.

## See Also

- [`Thread::div`](div.md): the more permissive counterpart, delegating to the raw
  `Series::div()` algebra instead of always promoting to float.
- [`Thread::rtruediv`](rtruediv.md): the reflected form.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="transform.html">
    <span class="label">Previous</span>
    <span class="title">transform</span>
  </a>
  <a class="next-link" href="ts.html">
    <span class="label">Next</span>
    <span class="title">ts</span>
  </a>
</div>
