<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: fc02d23b3ee1 -->

# DataFrame::floordiv

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::floordiv(double other) const -> DataFrame
DataFrame::floordiv(const DataFrame& other) const -> DataFrame
DataFrame::floordiv(const DataFrame& other, const AlignOptions& options) const -> DataFrame
DataFrame::floordiv(const Thread& other, Axis axis = Axis::Feature) const -> DataFrame
DataFrame::floordiv(const Thread& other, Axis axis, const AlignOptions& options) const -> DataFrame
DataFrame::floordiv(const Thread& other, const AlignOptions& options, Axis axis = Axis::Feature) const -> DataFrame
```

## What It Does

The named-method form of floor division: divide, then round down to the nearest whole
number. `floordiv()` shares the exact same overload shape and alignment rules as
[`add()`](add.md) and the rest of the arithmetic family, but is the one member of the
family whose result is genuinely different from [`div()`](div.md) and
[`truediv()`](truediv.md) — those two perform ordinary division, while `floordiv()`
always floors the result afterward, even when the two operands would otherwise divide
evenly with a remainder.

## Parameters

**other**

The right-hand operand, floor-dividing the current frame: `double` (broadcast
scalar), `const DataFrame&` (aligned frame-to-frame), or `const Thread&` (broadcast
per `axis`).

**axis** (`Axis`, default `Axis::Feature`, Thread overloads only)

Same meaning as [`add()`](add.md)'s `axis`: `Axis::Feature` (default) supplies one
value per feature; `Axis::Case` supplies one value per case.

**options** (`AlignOptions`, optional)

Same meaning as [`add()`](add.md)'s `options`.

## Returns

`DataFrame`
    A new frame, same shape as the source, with every numeric feature floor-divided by
    `other`. Non-numeric features are left unchanged.

## Examples

Example 1. Floor-divide every numeric feature by a scalar.

```cpp
using namespace pluss::table;

auto counts = DataFrame({
    {"total", pluss::make_series({7.0, 9.0, 10.0})},
}, pluss::make_note({"a", "b", "c"}));

auto out = counts.floordiv(2.0);
std::cout << out;
```

### Output

```text
============
     | total
------------
a    | 3    
b    | 4    
c    | 5    
------------
[3 cs x 1 feat]
```

`7 / 2 = 3.5` floors to `3`, and `9 / 2 = 4.5` floors to `4` — compare this to
[`div(2.0)`](div.md) on the same frame, which would return `3.5` and `4.5` unchanged.
`10 / 2 = 5` divides evenly, so `floordiv()` and `div()` agree on that row.

## Notes

- Non-numeric features are always left unchanged.
- `floordiv()` is the one member of this arithmetic family that differs from
  `div()`/`truediv()` in its actual result, not just its name — it always rounds the
  quotient down to the nearest whole number.
- Shares its full overload shape, alignment semantics, and `AlignOptions` behavior with
  [`add()`](add.md), [`sub()`](sub.md), [`mul()`](mul.md), [`div()`](div.md),
  [`mod()`](mod.md), and [`pow()`](pow.md).

## See Also

- [`DataFrame::div`](div.md) / [`DataFrame::truediv`](truediv.md): ordinary division,
  without flooring.
- [`DataFrame::mod`](mod.md): the remainder left over after floor division.
- [`DataFrame::round`](round.md): round to a given decimal precision instead of always
  flooring.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="filter.html">
    <span class="label">Previous</span>
    <span class="title">filter</span>
  </a>
  <a class="next-link" href="flow.html">
    <span class="label">Next</span>
    <span class="title">flow</span>
  </a>
</div>
