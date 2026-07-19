<!-- pluss-hand-authored -->

# DataFrame::rmul

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::rmul(double other) const -> DataFrame
```

## What It Does

The reflected (right-hand) form of [`mul()`](mul.md): computes `other * frame` instead
of `frame * other`. Like [`radd()`](radd.md), this exists mainly for symmetry with the
rest of the reflected family — multiplication is commutative, so `rmul()` and `mul()`
always agree on the same operand.

## Parameters

**other** (`double`)

The scalar multiplied against every numeric feature on the frame.

## Returns

`DataFrame`
    A new frame, same shape as the source, with every numeric feature multiplied by
    `other`. Non-numeric features are left unchanged.

## Examples

Example 1. `rmul()` and `mul()` produce the same result.

```cpp
using namespace pluss::table;

auto values = DataFrame({
    {"x", pluss::make_series({2.0, 3.0, 4.0})},
}, pluss::make_note({"a", "b", "c"}));

auto out = values.rmul(10.0);
std::cout << out;
```

### Output

```text
===========
     | x   
-----------
a    | 20  
b    | 30  
c    | 40  
-----------
[3 cs x 1 feat]
```

`values.mul(10.0)` would produce this identical result — `10 * x` and `x * 10` are the
same number.

## Notes

- Only accepts a scalar `double`; there is no reflected `DataFrame` or `Thread`
  overload — see [`radd()`](radd.md) for why.
- Multiplication is commutative, so `rmul()` never differs from `mul()` in result,
  unlike [`rdiv()`](rdiv.md), [`rsub()`](rsub.md), and the rest of the non-commutative
  reflected forms.

## See Also

- [`DataFrame::mul`](mul.md): the forward form.
- [`DataFrame::radd`](radd.md): the reflected addition, explaining the full reflected
  family's scalar-only shape.
- [`DataFrame::rdiv`](rdiv.md): the reflected division, where operand order genuinely
  changes the result.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rmod.html">
    <span class="label">Previous</span>
    <span class="title">rmod</span>
  </a>
  <a class="next-link" href="ror.html">
    <span class="label">Next</span>
    <span class="title">ror</span>
  </a>
</div>
