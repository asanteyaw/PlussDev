<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 91b4cb330f2e -->

# DataFrame::rsub

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::rsub(double other) const -> DataFrame
```

## What It Does

The reflected (right-hand) form of [`sub()`](sub.md): computes `other - frame` instead
of `frame - other`. Unlike [`radd()`](radd.md), the reflected and forward forms
genuinely disagree here, since subtraction is not commutative.

## Parameters

**other** (`double`)

The scalar that the frame's values are subtracted from.

## Returns

`DataFrame`
    A new frame, same shape as the source, holding `other` minus every numeric
    feature's value. Non-numeric features are left unchanged.

## Examples

Example 1. Compare `rsub()` against plain `sub()` on the same data.

```cpp
using namespace pluss::table;

auto values = DataFrame({
    {"x", pluss::make_series({2.0, 3.0, 4.0})},
}, pluss::make_note({"a", "b", "c"}));

auto reflected = values.rsub(10.0);
std::cout << reflected;
```

### Output

```text
===========
     | x   
-----------
a    | 8   
b    | 7   
c    | 6   
-----------
[3 cs x 1 feat]
```

`values.rsub(10.0)` computes `10 - x` for every case: `10 - 2 = 8`, `10 - 3 = 7`,
`10 - 4 = 6`.

```cpp
auto forward = values.sub(10.0);
std::cout << forward;
```

### Output

```text
===========
     | x   
-----------
a    | -8  
b    | -7  
c    | -6  
-----------
[3 cs x 1 feat]
```

`values.sub(10.0)` computes `x - 10` instead — the same magnitudes, but with the
opposite sign, since `frame - other` and `other - frame` are not the same operation.

## Notes

- `rsub(other)` computes `other - frame`; [`sub(other)`](sub.md) computes
  `frame - other`. The two are only equal when the frame's values are `0`.
- Like all reflected methods, `rsub()` only accepts a scalar `double` — see
  [`radd()`](radd.md) for why there is no reflected `DataFrame` or `Thread` overload.

## See Also

- [`DataFrame::sub`](sub.md): the forward form.
- [`DataFrame::radd`](radd.md): the reflected addition, where operand order does not
  change the result.
- [`DataFrame::rtruediv`](rtruediv.md): the reflected division, another
  non-commutative case worth double-checking operand order for.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rpow.html">
    <span class="label">Previous</span>
    <span class="title">rpow</span>
  </a>
  <a class="next-link" href="rtruediv.html">
    <span class="label">Next</span>
    <span class="title">rtruediv</span>
  </a>
</div>
