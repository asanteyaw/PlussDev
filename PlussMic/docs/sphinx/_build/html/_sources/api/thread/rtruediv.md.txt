<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0aa756587271 -->

# Thread::rtruediv

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::rtruediv(double other) const -> Thread
Thread::rtruediv(const Thread& other) const -> Thread
```

## What It Does

The reflected form of [`truediv()`](truediv.md) — computes `other / (*this)` with
both operands promoted to floating point first, the same guarantee `truediv()`
makes in the forward direction.

## Parameters

**other** (`double` or `const Thread&`)

Same shape as [`truediv()`](truediv.md).

## Returns

`Thread`
    A new `Float64` thread holding `other / (*this)`, elementwise.

## Examples

Example 1. Divide a scalar by every value.

```cpp
using namespace pluss::table;

auto prices = Thread(pluss::make_series({10.0, 20.0, 30.0}));
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

- See [`rdiv()`](rdiv.md) for a second example against another thread — the two
  methods agree in every case shown in this reference, differing only in their
  type-promotion guarantee for non-`Float64` source data.

## See Also

- [`Thread::truediv`](truediv.md): the forward form.
- [`Thread::rdiv`](rdiv.md): the more permissive reflected counterpart.

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
