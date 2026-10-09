<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: eaac62c03ffa -->

# DataFrame::rfloordiv

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::rfloordiv(double other) const -> DataFrame
```

## What It Does

The reflected (right-hand) form of [`floordiv()`](floordiv.md): computes
`floor(other / frame)` instead of `floor(frame / other)`. Like the forward pair, this
is the one reflected arithmetic method whose result genuinely differs from
[`rdiv()`](rdiv.md)/[`rtruediv()`](rtruediv.md) in value, not just in name — it always
rounds the quotient down after dividing.

## Parameters

**other** (`double`)

The scalar that the frame's values floor-divide.

## Returns

`DataFrame`
    A new frame, same shape as the source, holding `other` floor-divided by every
    numeric feature's value. Non-numeric features are left unchanged.

## Examples

Example 1. Floor-divide a scalar by every numeric feature.

```cpp
using namespace pluss::table;

auto values = DataFrame({
    {"x", pluss::make_series({2.0, 3.0, 4.0})},
}, pluss::make_note({"a", "b", "c"}));

auto out = values.rfloordiv(10.0);
std::cout << out;
```

### Output

```text
===========
     | x   
-----------
a    | 5   
b    | 3   
c    | 2   
-----------
[3 cs x 1 feat]
```

`10 / 2 = 5` floors to `5` exactly; `10 / 3 ≈ 3.333` floors to `3`; `10 / 4 = 2.5`
floors to `2` — compare this to [`rdiv(10.0)`](rdiv.md) on the same frame, which would
return the un-floored `5`, `3.333333333`, `2.5`.

```cpp
auto forward = values.floordiv(10.0);
std::cout << forward;
```

### Output

```text
===========
     | x   
-----------
a    | 0   
b    | 0   
c    | 0   
-----------
[3 cs x 1 feat]
```

The forward `floordiv(10.0)` computes `floor(x / 10)` instead: `2 / 10`, `3 / 10`, and
`4 / 10` are all less than `1`, so every case floors down to `0` — a very different
result from the reflected form's `5`, `3`, `2` above. This is a concrete illustration
of why operand order matters for non-commutative arithmetic: reversing which side is
the numerator can move results from "always zero" to "always at least two".

## Notes

- Non-numeric features are always left unchanged.
- `rfloordiv()` always rounds the quotient down to the nearest whole number, exactly
  like the forward [`floordiv()`](floordiv.md), just with the operand order reversed.
- Only accepts a scalar `double` — see [`radd()`](radd.md) for why there is no
  reflected `DataFrame` or `Slate` overload.

## See Also

- [`DataFrame::floordiv`](floordiv.md): the forward form.
- [`DataFrame::rdiv`](rdiv.md) / [`DataFrame::rtruediv`](rtruediv.md): the reflected
  ordinary division, without flooring.
- [`DataFrame::rmod`](rmod.md): the reflected remainder left over after this
  floor-division.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="reshuffle.html">
    <span class="label">Previous</span>
    <span class="title">reshuffle</span>
  </a>
  <a class="next-link" href="rmod.html">
    <span class="label">Next</span>
    <span class="title">rmod</span>
  </a>
</div>
