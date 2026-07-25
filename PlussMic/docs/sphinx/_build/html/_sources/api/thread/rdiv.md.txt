<!-- pluss-hand-authored -->

# Thread::rdiv

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::rdiv(double other) const -> Thread
Thread::rdiv(const Thread& other) const -> Thread
```

## What It Does

The reflected form of [`div()`](div.md). Division is not commutative, so `rdiv()`
computes `other / (*this)` instead of `(*this) / other`.

## Parameters

**other** (`double` or `const Thread&`)

Same shape as [`div()`](div.md).

## Returns

`Thread`
    A new thread holding `other / (*this)`, elementwise.

## Examples

Example 1. Divide a scalar by every value.

```cpp
using namespace pluss::table;

auto prices = Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto out = prices.rdiv(4.0);
std::cout << out;
```

### Output

```text
0  0.4
1  0.2
2  0.1333333333

ptype: Series
```

`4 / 10`, `4 / 20`, `4 / 30` — the opposite of [`prices.div(4.0)`](div.md), which
would give `2.5, 5, 7.5`.

Example 2. Reflected against another thread.

```cpp
auto splits = Thread(pluss::make_series({4.0, 8.0, 9.0}));
auto out_pair = prices.rdiv(splits);
std::cout << out_pair;
```

### Output

```text
0  0.4
1  0.4
2  0.3

ptype: Series
```

`prices.rdiv(splits)` computes exactly what `splits.div(prices)` would: `4 / 10`,
`8 / 20`, `9 / 30`.

## Notes

- `prices.rdiv(x)` always equals `x / prices` — the mirror image of
  [`div()`](div.md), not a synonym for it.
- See [`add()`](add.md) for the family-wide shared mechanics.

## See Also

- [`Thread::div`](div.md): the forward form.
- [`Thread::rtruediv`](rtruediv.md): the always-floating-point reflected counterpart.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rank.html">
    <span class="label">Previous</span>
    <span class="title">rank</span>
  </a>
  <a class="next-link" href="repeated.html">
    <span class="label">Next</span>
    <span class="title">repeated</span>
  </a>
</div>
