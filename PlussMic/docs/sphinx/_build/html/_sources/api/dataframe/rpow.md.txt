<!-- pluss-hand-authored -->

# DataFrame::rpow

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::rpow(double other) const -> DataFrame
```

## What It Does

The reflected (right-hand) form of [`pow()`](pow.md): computes `other ^ frame` (the
scalar raised to each feature's value as the exponent) instead of `frame ^ other`
(each feature's value raised to the scalar exponent). Exponentiation is emphatically
not commutative, so this is one of the reflected methods where the result looks
nothing like its forward counterpart.

## Parameters

**other** (`double`)

The scalar base; each numeric feature's value is used as the exponent.

## Returns

`DataFrame`
    A new frame, same shape as the source, holding `other` raised to the power of
    every numeric feature's value. Non-numeric features are left unchanged.

## Examples

Example 1. Compare `rpow()` against plain `pow()` on the same data.

```cpp
using namespace pluss::table;

auto values = DataFrame({
    {"x", pluss::make_series({2.0, 3.0, 4.0})},
}, pluss::make_note({"a", "b", "c"}));

auto reflected = values.rpow(2.0);
std::cout << reflected;
```

### Output

```text
===========
     | x   
-----------
a    | 4   
b    | 8   
c    | 16  
-----------
[3 cs x 1 feat]
```

`values.rpow(2.0)` computes `2 ^ x` for every case: `2^2 = 4`, `2^3 = 8`, `2^4 = 16`.

```cpp
auto forward = values.pow(2.0);
std::cout << forward;
```

### Output

```text
===========
     | x   
-----------
a    | 4   
b    | 9   
c    | 16  
-----------
[3 cs x 1 feat]
```

The forward `pow(2.0)` computes `x ^ 2` instead: `2^2 = 4` happens to coincide with the
reflected result on this one row, but `3^2 = 9` and `4^2 = 16` diverge from the
reflected form's `8` and `16` — the two agree only where base and exponent trade
places without changing the result (`2^2` either way), which is a coincidence of this
particular value, not a general property of exponentiation.

## Notes

- Non-numeric features are always left unchanged.
- Like [`pow()`](pow.md), `rpow()` always evaluates feature-by-feature rather than as a
  single batched matrix operation internally — an implementation detail with no
  observable effect on the result.
- Only accepts a scalar `double` — see [`radd()`](radd.md) for why there is no
  reflected `DataFrame` or `Thread` overload.

## See Also

- [`DataFrame::pow`](pow.md): the forward form.
- [`DataFrame::radd`](radd.md): the reflected addition, explaining the full reflected
  family's scalar-only shape.
- [`DataFrame::rmul`](rmul.md): the reflected multiplication, where (unlike here)
  operand order never changes the result.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="round.html">
    <span class="label">Previous</span>
    <span class="title">round</span>
  </a>
  <a class="next-link" href="rsub.html">
    <span class="label">Next</span>
    <span class="title">rsub</span>
  </a>
</div>
