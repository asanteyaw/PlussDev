<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0162c5cfd718 -->

# DataFrame::mod

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::mod(double other) const -> DataFrame
DataFrame::mod(const DataFrame& other) const -> DataFrame
DataFrame::mod(const DataFrame& other, const AlignOptions& options) const -> DataFrame
DataFrame::mod(const Thread& other, Axis axis = Axis::Feature) const -> DataFrame
DataFrame::mod(const Thread& other, Axis axis, const AlignOptions& options) const -> DataFrame
DataFrame::mod(const Thread& other, const AlignOptions& options, Axis axis = Axis::Feature) const -> DataFrame
```

## What It Does

The named-method form of elementwise remainder (modulo). `mod()` shares the exact same
overload shape and alignment rules as [`add()`](add.md) and the rest of the arithmetic
family — see `add()`'s "What It Does" for the full explanation of the scalar/frame/
Thread operand shapes and the `axis`/`options` parameters. `mod()` is the natural
companion to [`floordiv()`](floordiv.md): together they satisfy
`x == y * x.floordiv(y) + x.mod(y)`.

## Parameters

**other**

The right-hand operand (the divisor): `double` (broadcast scalar), `const DataFrame&`
(aligned frame-to-frame), or `const Thread&` (broadcast per `axis`).

**axis** (`Axis`, default `Axis::Feature`, Thread overloads only)

Same meaning as [`add()`](add.md)'s `axis`: `Axis::Feature` (default) supplies one
value per feature; `Axis::Case` supplies one value per case.

**options** (`AlignOptions`, optional)

Same meaning as [`add()`](add.md)'s `options`.

## Returns

`DataFrame`
    A new frame, same shape as the source, with every numeric feature replaced by its
    remainder after dividing by `other`. Non-numeric features are left unchanged.

## Examples

Example 1. Take the remainder of every numeric feature against a scalar divisor.

```cpp
using namespace pluss::table;

auto counts = DataFrame({
    {"total", pluss::make_series({7.0, 9.0, 10.0})},
}, pluss::make_note({"a", "b", "c"}));

auto out = counts.mod(2.0);
std::cout << out;
```

### Output

```text
============
     | total
------------
a    | 1    
b    | 1    
c    | 0    
------------
[3 cs x 1 feat]
```

`7 mod 2 = 1`, `9 mod 2 = 1`, and `10 mod 2 = 0` — `10` divides evenly by `2`, leaving
no remainder, consistent with [`floordiv(2.0)`](floordiv.md) also landing on a whole
number for that same row.

## Notes

- Non-numeric features are always left unchanged.
- `mod()` and [`floordiv()`](floordiv.md) are complementary: the floor-divided
  quotient times the divisor, plus the remainder from `mod()`, reconstructs the
  original value.
- Shares its full overload shape, alignment semantics, and `AlignOptions` behavior with
  [`add()`](add.md), [`sub()`](sub.md), [`mul()`](mul.md), [`div()`](div.md),
  [`floordiv()`](floordiv.md), and [`pow()`](pow.md).

## See Also

- [`DataFrame::floordiv`](floordiv.md): the matching floor-division quotient.
- [`DataFrame::div`](div.md) / [`DataFrame::truediv`](truediv.md): ordinary division,
  without a remainder.
- [`DataFrame::rmod`](rmod.md): the reflected form, computing `other mod frame`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="min.html">
    <span class="label">Previous</span>
    <span class="title">min</span>
  </a>
  <a class="next-link" href="mode.html">
    <span class="label">Next</span>
    <span class="title">mode</span>
  </a>
</div>
