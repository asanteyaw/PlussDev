<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f18031bde7e5 -->

# DataFrame::rmod

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame DataFrame::rmod(double other) const
```

## What It Does

The reflected (right-hand) form of [`mod()`](mod.md): computes `other mod frame`
instead of `frame mod other`. Like the rest of the non-commutative reflected family,
this genuinely differs from the forward form's result, not just its name.

## Parameters

**other** (`double`)

The scalar dividend; each numeric feature's value is used as the divisor.

## Returns

`DataFrame`
    A new frame, same shape as the source, holding the remainder of `other` divided by
    every numeric feature's value. Non-numeric features are left unchanged.

## Examples

Example 1. Compare `rmod()` against plain `mod()` on the same data.

```cpp
using namespace pluss::table;

auto values = DataFrame({
    {"x", pluss::make_series({2.0, 3.0, 4.0})},
}, pluss::make_note({"a", "b", "c"}));

auto reflected = values.rmod(10.0);
std::cout << reflected;
```

### Output

```text
===========
     | x   
-----------
a    | 0   
b    | 1   
c    | 2   
-----------
[3 cs x 1 feat]
```

`values.rmod(10.0)` computes `10 mod x` for every case: `10 mod 2 = 0` (divides
evenly), `10 mod 3 = 1`, `10 mod 4 = 2`.

```cpp
auto forward = values.mod(10.0);
std::cout << forward;
```

### Output

```text
===========
     | x   
-----------
a    | 2   
b    | 3   
c    | 4   
-----------
[3 cs x 1 feat]
```

The forward `mod(10.0)` computes `x mod 10` instead: since every value in `x` is
smaller than `10`, each remainder is just the original value unchanged — a
noticeably different result from the reflected form.

## Notes

- Non-numeric features are always left unchanged.
- `rmod(other)` computes `other mod frame`; [`mod(other)`](mod.md) computes
  `frame mod other`. These only coincide in special cases (for example, when
  `frame`'s values evenly divide `other` and vice versa).
- Only accepts a scalar `double` — see [`radd()`](radd.md) for why there is no
  reflected `DataFrame` or `Slate` overload.

## See Also

- [`DataFrame::mod`](mod.md): the forward form.
- [`DataFrame::rfloordiv`](rfloordiv.md): the matching reflected floor-division
  quotient.
- [`DataFrame::rdiv`](rdiv.md) / [`DataFrame::rtruediv`](rtruediv.md): the reflected
  ordinary division, without a remainder.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rfloordiv.html">
    <span class="label">Previous</span>
    <span class="title">rfloordiv</span>
  </a>
  <a class="next-link" href="rmul.html">
    <span class="label">Next</span>
    <span class="title">rmul</span>
  </a>
</div>
