<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8f899914c818 -->

# Shelf::rtruediv

[Back to Shelf API Reference](../../shelf_api_reference)

## Signatures

```cpp
Shelf Shelf::rtruediv(double other) const
Shelf Shelf::rtruediv(const Shelf& other) const
```

## What It Does

The reflected form of [`truediv()`](truediv.md) — computes `other / (*this)` with
both operands promoted to floating point first, the same guarantee `truediv()`
makes in the forward direction.

## Parameters

**other** (`double` or `const Shelf&`)

Same shape as [`truediv()`](truediv.md).

## Returns

`Shelf`
    A new `Float64` shelf holding `other / (*this)`, elementwise.

## Examples

Example 1. Divide a scalar by every value.

```cpp
using namespace pluss::table;

auto prices = Shelf(pluss::make_series({10.0, 20.0, 30.0}));
auto out = prices.rtruediv(4.0);
std::cout << out;
```

### Output

```text
0  0.4
1  0.2
2  0.1333333333

ptype: Series
```

Matches [`prices.rdiv(4.0)`](rdiv.md) exactly for this `Float64` data.

## Notes

- See [`rdiv()`](rdiv.md) for a second example against another shelf — the two
  methods agree in every case shown in this reference, differing only in their
  type-promotion guarantee for non-`Float64` source data.

## See Also

- [`Shelf::truediv`](truediv.md): the forward form.
- [`Shelf::rdiv`](rdiv.md): the more permissive reflected counterpart.

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
