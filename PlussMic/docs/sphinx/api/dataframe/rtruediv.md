<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c3206775926c -->

# DataFrame::rtruediv

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame DataFrame::rtruediv(double other) const
```

## What It Does

The reflected (right-hand) form of [`truediv()`](truediv.md): computes `other / frame`
instead of `frame / other` — and, verified against the underlying numeric kernel,
**the same operation as [`rdiv()`](rdiv.md)**. Like the forward pair
`div()`/`truediv()`, the two reflected names route to the identical division kernel.

## Parameters

**other** (`double`)

The scalar that the frame's values divide.

## Returns

`DataFrame`
    A new frame, same shape as the source, holding `other` divided by every numeric
    feature's value. Non-numeric features are left unchanged.

## Examples

Example 1. `rtruediv()` and `rdiv()` produce the identical result.

```cpp
using namespace pluss::table;

auto values = DataFrame({
    {"x", pluss::make_series({2.0, 3.0, 4.0})},
}, pluss::make_note({"a", "b", "c"}));

auto out = values.rtruediv(10.0);
std::cout << out;
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

```cpp
auto out_rdiv = values.rdiv(10.0);
std::cout << out_rdiv;
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

Both compute `10 / x` for every case — identical numbers, since `rtruediv()` and
`rdiv()` share the same division kernel underneath.

## Notes

- Non-numeric features are always left unchanged.
- `rtruediv()` and `rdiv()` are verified to be identical operations.
- Only accepts a scalar `double` — see [`radd()`](radd.md) for why there is no
  reflected `DataFrame` or `Slate` overload.
- For the reflected floor-division variant that rounds `other / frame` down to a whole
  number instead, use [`rfloordiv()`](rfloordiv.md).

## See Also

- [`DataFrame::truediv`](truediv.md): the forward form.
- [`DataFrame::rdiv`](rdiv.md): the verified-identical alternate name for this same
  reflected operation.
- [`DataFrame::rfloordiv`](rfloordiv.md): the distinct reflected floor-division.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rsub.html">
    <span class="label">Previous</span>
    <span class="title">rsub</span>
  </a>
  <a class="next-link" href="rxor.html">
    <span class="label">Next</span>
    <span class="title">rxor</span>
  </a>
</div>
