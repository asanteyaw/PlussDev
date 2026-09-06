<!-- pluss-hand-authored -->

# Series::is_signed

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
bool Series::is_signed() const
```

## What It Does

Returns `true` when the `Series`' dtype can represent negative values —
signed integers (`Int8` … `Int64`), all real floating formats, and both
complex formats — and `false` for the unsigned integers (`UInt8` …
`UInt64`) and `Bool`.

It is a dtype test, not a value test: it reports whether the *format* has a
sign, not whether the current data contains a negative number. A signed
`Int32` tensor holding only positive values is still `is_signed() == true`;
an unsigned `UInt8` tensor is `is_signed() == false` even if every value is
`0`.

The reason to care is that arithmetic on an unsigned dtype **wraps** instead
of going negative. `neg()`, `sub()`, and anything that can produce a value
below zero silently roll around modulo `2^bits` on an unsigned `Series`.
`is_signed()` is the guard that says "this subtraction is safe to interpret
literally" — where it returns `false`, a result near zero may actually be a
huge positive number.

Across every scalar type, the three category predicates behave as:

| dtype | `is_complex` | `is_floating_point` | `is_signed` |
|---|---|---|---|
| `Bool` | false | false | **false** |
| `UInt8` … `UInt64` | false | false | **false** |
| `Int8` … `Int64` | false | false | **true** |
| `Float16`, `BFloat16`, `Float32`, `Float64`, the 8-bit floats | false | true | **true** |
| `Complex64`, `Complex128` | true | false | **true** |

Note `Bool` and the unsigned integers land in the same `false` column for
`is_signed()`, but for different reasons — `Bool` has no numeric sign concept
at all, the unsigned integers deliberately trade the sign bit for range.

## Parameters

None.

## Returns

`bool`
    `true` for signed integer, real floating, and complex dtypes; `false`
    for unsigned integer and `Bool`.

## Gradient

Not applicable — `is_signed()` returns a plain `bool`.

## Examples

Example 1. Signed integers, floats, and complex answer `true`; unsigned
integers and `Bool` answer `false`.

```cpp
for (auto st : {pluss::ScalarType::Bool, pluss::ScalarType::UInt8,
                pluss::ScalarType::UInt64, pluss::ScalarType::Int8,
                pluss::ScalarType::Int64, pluss::ScalarType::Float32,
                pluss::ScalarType::Complex64}) {
  auto x = pluss::series::zeros({1}, st);
  std::cout << x.dtype() << " -> is_signed=" << std::boolalpha
            << x.is_signed() << "\n";
}
```

### Output

```text
bool -> is_signed=false
uint8 -> is_signed=false
uint64 -> is_signed=false
int8 -> is_signed=true
int64 -> is_signed=true
float32 -> is_signed=true
complex64 -> is_signed=true
```

Example 2. `neg()` on an unsigned `Series` wraps around — this is the hazard
`is_signed() == false` is warning about.

```cpp
auto u = pluss::make_series({1, 2, 3}).to(pluss::ScalarType::UInt8);
std::cout << "u.is_signed()=" << std::boolalpha << u.is_signed() << "\n";
pluss::series::print(std::cout, u.neg());
```

### Output

```text
u.is_signed()=false
Series(shape=[3], strides=[1], dtype=uint8, platform=cpu, numel=3, contiguous=true)
data=[255, 254, 253]
```

`-1` in `UInt8` is `255`, `-2` is `254`, and so on — two's-complement
wraparound. On a signed dtype the same call would give `[-1, -2, -3]`.

Example 3. Subtraction that would go negative underflow-wraps on an unsigned
dtype.

```cpp
auto a = pluss::make_series({0, 1, 2}).to(pluss::ScalarType::UInt8);
auto b = pluss::make_series({1, 1, 1}).to(pluss::ScalarType::UInt8);
std::cout << "a.is_signed()=" << std::boolalpha << a.is_signed() << "\n";
pluss::series::print(std::cout, a.sub(b));
```

### Output

```text
a.is_signed()=false
Series(shape=[3], strides=[1], dtype=uint8, platform=cpu, numel=3, contiguous=true)
data=[255, 0, 1]
```

`0 - 1` becomes `255`, not `-1`. If differences can be negative, convert to a
signed dtype first (`a.to(ScalarType::Int32).sub(...)`), or check
`is_signed()` and branch.

Example 4. `is_signed()` is a *dtype* question; [`signbit`](signbit.md) is
the per-element one.

```cpp
auto x = pluss::make_series({-2.0, 0.0, 3.0});
std::cout << "x.is_signed()=" << std::boolalpha << x.is_signed()
          << " (the dtype can hold negatives)\n";
pluss::series::print(std::cout, x.signbit());
```

### Output

```text
x.is_signed()=true (the dtype can hold negatives)
Series(shape=[3], strides=[1], dtype=bool, platform=cpu, numel=3, contiguous=true)
data=[true, false, false]
```

`is_signed()` is one `bool` for the whole tensor — "could any value be
negative"; [`signbit`](signbit.md) is one `bool` per element — "is *this*
value negative" (and it catches `-0.0`, which compares equal to `0`).

## Gotchas And Failure Modes

- `is_signed() == false` covers **two** dtypes with different rationales:
  the unsigned integers (range-for-sign trade) and `Bool` (no numeric sign
  at all).
- On an unsigned `Series`, `neg()`, `sub()`, and any op that can dip below
  zero wrap modulo `2^bits` — a "small" result can be a huge positive
  number. Convert to a signed dtype before such arithmetic if negatives are
  meaningful.
- The answer is about the format, not the data. A signed tensor of all-
  positive values is still `is_signed() == true`.
- For the elementwise "is this value negative" question use
  [`signbit`](signbit.md), which also distinguishes `-0.0` from `0.0`.

## Summary

`is_signed()` reports whether the dtype can represent negatives:

- `true` for signed integers, all real floats, and complex
- `false` for unsigned integers **and** `Bool`
- format-only — a signed tensor of positives is still `true`
- the guard before subtraction / negation whose result could go below zero
  on an unsigned dtype (where it wraps)

## See Also

- [`Series::dtype`](dtype.md): the exact representation and the same category
  queries on the `DType` struct.
- [`Series::is_complex`](is_complex.md) /
  [`Series::is_floating_point`](is_floating_point.md): the other two category
  predicates.
- [`Series::signbit`](signbit.md): the per-element "is this value negative"
  question, `-0.0`-aware.
- [`Series::to`](to.md): convert to a signed dtype before sign-sensitive
  arithmetic.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_set_to.html">
    <span class="label">Previous</span>
    <span class="title">is_set_to</span>
  </a>
  <a class="next-link" href="isclose.html">
    <span class="label">Next</span>
    <span class="title">isclose</span>
  </a>
</div>
