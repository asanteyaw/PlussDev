<!-- pluss-hand-authored -->

# Series::is_complex

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
bool Series::is_complex() const
```

## What It Does

Returns `true` when the `Series`' dtype is one of the two complex formats —
`Complex64` or `Complex128` — and `false` for every real dtype (floating,
signed and unsigned integer, and `Bool`).

It is a **dtype** test, not a value test. It reads
[`dtype`](dtype.md)`().is_complex()` and nothing else: it does not scan the
data for nonzero imaginary parts, and it costs nothing. A `Complex64` tensor
whose imaginary lane is entirely zero is still `is_complex() == true` — the
storage layout reserves two float lanes per element regardless of what is in
them.

Use it to branch before operations whose *contract* differs by
representation rather than by content: `abs()` returns a magnitude for
complex input and a plain absolute value for real input; [`sgn`](sgn.md)
projects onto the unit circle for complex and returns `-1/0/+1` for real;
[`sort`](sort.md) and the ordering comparisons reject complex entirely.
Where "does this tensor carry an imaginary component at all" is the question,
`is_complex()` answers it; where "is this particular value purely real"
is the question, inspect [`imag`](imag.md) instead.

`is_complex()` sits alongside [`is_floating_point`](is_floating_point.md) and
[`is_signed`](is_signed.md) as the three category predicates on `Series`.
Their full behavior across every scalar type:

| dtype | `is_complex` | `is_floating_point` | `is_signed` |
|---|---|---|---|
| `Bool` | false | false | false |
| `UInt8` … `UInt64` | false | false | false |
| `Int8` … `Int64` | false | false | true |
| `Float16`, `BFloat16`, `Float32`, `Float64`, the 8-bit floats | false | true | true |
| `Complex64`, `Complex128` | **true** | **false** | true |

The row that trips people up is the last one: a complex dtype is **not**
floating-point, even though its components are floats. See
[`is_floating_point`](is_floating_point.md).

## Parameters

None.

## Returns

`bool`
    `true` for `Complex64` / `Complex128`, `false` for every other dtype.

## Gradient

Not applicable — `is_complex()` returns a plain `bool`.

## Examples

Example 1. Across the whole scalar-type surface, only the two complex
formats answer `true`.

```cpp
for (auto st : {pluss::ScalarType::Bool, pluss::ScalarType::Int64,
                pluss::ScalarType::Float64, pluss::ScalarType::Complex64,
                pluss::ScalarType::Complex128}) {
  auto x = pluss::series::zeros({1}, st);
  std::cout << x.dtype() << " -> is_complex=" << std::boolalpha
            << x.is_complex() << "\n";
}
```

### Output

```text
bool -> is_complex=false
int64 -> is_complex=false
float64 -> is_complex=false
complex64 -> is_complex=true
complex128 -> is_complex=true
```

Example 2. A complex `Series` whose imaginary part is all zeros is *still*
complex — the answer is about representation, not content.

```cpp
auto z = pluss::series::complex(pluss::make_series({1.0f, 2.0f}),
                                pluss::make_series({0.0f, 0.0f}));
std::cout << "dtype=" << z.dtype() << " is_complex=" << std::boolalpha
          << z.is_complex() << "\n";
std::cout << "imag part: ";
pluss::series::print(std::cout, z.imag());
```

### Output

```text
dtype=complex64 is_complex=true
imag part: Series(shape=[2], strides=[1], dtype=float32, platform=cpu, numel=2, contiguous=true)
data=[0.0000, 0.0000]
```

If the real question is "is every value purely real", test the imaginary
part (`z.imag().eq(0)`), not `is_complex()`.

Example 3. Pulling the real (or imaginary) part *out* of a complex `Series`
gives a real floating `Series` — `is_complex()` flips to `false`.

```cpp
auto z = pluss::series::complex(pluss::make_series({1.0f, 2.0f}),
                                pluss::make_series({3.0f, 4.0f}));
std::cout << std::boolalpha
          << "z.is_complex()           = " << z.is_complex() << "\n"
          << "z.real().is_complex()    = " << z.real().is_complex() << "\n"
          << "z.real().is_floating_point() = " << z.real().is_floating_point() << "\n";
```

### Output

```text
z.is_complex()           = true
z.real().is_complex()    = false
z.real().is_floating_point() = true
```

[`real`](real.md) and [`imag`](imag.md) drop the imaginary lane and hand back
a tensor in the corresponding real float dtype (`Complex64` → `Float32`,
`Complex128` → `Float64`).

Example 4. A `std::complex` literal fed to [`make_series`](../series_factory/make.md)
produces a complex `Series` directly.

```cpp
auto z = pluss::make_series({std::complex<float>{1, 2}, {3, 4}});
std::cout << "dtype=" << z.dtype() << " is_complex=" << std::boolalpha
          << z.is_complex() << "\n";
```

### Output

```text
dtype=complex64 is_complex=true
```

## Gotchas And Failure Modes

- `is_complex()` never inspects values. A complex tensor with a zero
  imaginary part still returns `true`; a real tensor holding results that
  "should" be complex still returns `false`.
- A complex dtype is **not** floating-point.
  `x.is_floating_point() || x.is_complex()` is the test for "any continuous
  numeric data"; `is_floating_point()` alone silently excludes complex.
- The ordering operations ([`sort`](sort.md), `lt`/`gt`, [`argsort`](argsort.md))
  and reductions that depend on order reject complex input — guard with
  `is_complex()` before calling them on data of unknown dtype.
- Converting with [`to`](to.md) is what changes the category;
  `is_complex()` only reports it.

## Summary

`is_complex()` is a dtype-category predicate:

- `true` only for `Complex64` / `Complex128`
- metadata-only — never scans for nonzero imaginary parts
- a complex dtype is signed but **not** floating-point
- pair with [`is_floating_point`](is_floating_point.md) via `||` for an
  "any continuous data" check

## See Also

- [`Series::dtype`](dtype.md): the exact representation, plus `itemsize` and
  the same category queries on the `DType` struct.
- [`Series::is_floating_point`](is_floating_point.md) /
  [`Series::is_signed`](is_signed.md): the other two category predicates.
- [`Series::real`](real.md) / [`Series::imag`](imag.md): extract a real
  component (which is then no longer `is_complex()`).
- [`pluss::series::complex`](../series_factory/complex.md): build a complex
  `Series` from real and imaginary parts.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="irfft.html">
    <span class="label">Previous</span>
    <span class="title">irfft</span>
  </a>
  <a class="next-link" href="is_contiguous.html">
    <span class="label">Next</span>
    <span class="title">is_contiguous</span>
  </a>
</div>
