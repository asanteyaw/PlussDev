<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 129fc895a1a1 -->

# DataFrame::rdiv

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::rdiv(double other) const -> DataFrame
```

## What It Does

The reflected (right-hand) form of [`div()`](div.md): computes `other / frame` instead
of `frame / other`. Unlike [`rmul()`](rmul.md), the reflected and forward forms
genuinely disagree here, since division is not commutative.

```{important} rdiv() and rtruediv() are the same operation
Just like [`div()`](div.md) and [`truediv()`](truediv.md) are verified-identical at the
forward level, `rdiv()` and [`rtruediv()`](rtruediv.md) are the same reflected
operation under two names — see `rtruediv()` for the side-by-side confirmation.
```

## Parameters

**other** (`double`)

The scalar that the frame's values divide.

## Returns

`DataFrame`
    A new frame, same shape as the source, holding `other` divided by every numeric
    feature's value. Non-numeric features are left unchanged.

## Examples

Example 1. Compare `rdiv()` against plain `div()` on the same data.

```cpp
using namespace pluss::table;

auto values = DataFrame({
    {"x", pluss::make_series({2.0, 3.0, 4.0})},
}, pluss::make_note({"a", "b", "c"}));

auto reflected = values.rdiv(10.0);
std::cout << reflected;
```

### Output

```text
==================
     | x          
------------------
a    | 5          
b    | 3.333333333
c    | 2.5        
------------------
[3 cs x 1 feat]
```

`values.rdiv(10.0)` computes `10 / x` for every case: `10 / 2 = 5`, `10 / 3 ≈
3.333333333`, `10 / 4 = 2.5`.

```cpp
auto forward = values.div(10.0);
std::cout << forward;
```

### Output

```text
===========
     | x   
-----------
a    | 0.2 
b    | 0.3 
c    | 0.4 
-----------
[3 cs x 1 feat]
```

`values.div(10.0)` computes `x / 10` instead — an entirely different set of numbers,
since `frame / other` and `other / frame` are reciprocal operations, not the same one.

## Notes

- `rdiv(other)` computes `other / frame`; [`div(other)`](div.md) computes
  `frame / other`. The two only coincide when `frame`'s values and `other` are equal.
- `rdiv()` and `rtruediv()` are verified to be the same operation, mirroring the
  forward `div()`/`truediv()` identity.
- Only accepts a scalar `double` — see [`radd()`](radd.md) for why there is no
  reflected `DataFrame` or `Thread` overload.
- Dividing by a frame value of `0` produces the usual floating-point division-by-zero
  result (`inf` or `NaN`, depending on the numerator), not an exception.

## See Also

- [`DataFrame::div`](div.md): the forward form.
- [`DataFrame::rtruediv`](rtruediv.md): the verified-identical alternate name for this
  same reflected operation.
- [`DataFrame::rfloordiv`](rfloordiv.md): the reflected floor-division, which rounds
  `other / frame` down to a whole number.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rank.html">
    <span class="label">Previous</span>
    <span class="title">rank</span>
  </a>
  <a class="next-link" href="rename.html">
    <span class="label">Next</span>
    <span class="title">rename</span>
  </a>
</div>
