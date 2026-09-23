<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d92b1e11f192 -->

# Thread::rsub

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::rsub(double other) const -> Thread
Thread::rsub(const Thread& other) const -> Thread
```

## What It Does

The reflected form of [`sub()`](sub.md). Since subtraction is **not** commutative,
`rsub()` genuinely differs from `sub()` — it computes `other - (*this)` instead of
`(*this) - other`.

## Parameters

**other** (`double` or `const Thread&`)

Same shape as [`sub()`](sub.md).

## Returns

`Thread`
    A new thread holding `other - (*this)`, elementwise.

## Examples

Example 1. Subtract every value from a scalar.

```cpp
using namespace pluss::table;

auto prices = Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto out = prices.rsub(5.0);
std::cout << out;
```

### Output

```text
0  -5
1  -15
2  -25

ptype: Series
```

`5 - 10`, `5 - 20`, `5 - 30` — the scalar comes first this time, the opposite of
[`prices.sub(5.0)`](sub.md), which would give `5, 15, 25`.

Example 2. Reflected against another thread — equivalent to calling `sub()` with the
operands swapped.

```cpp
auto boost = Thread(pluss::make_series({2.0, 4.0, 5.0}));
auto out_pair = prices.rsub(boost);
std::cout << out_pair;
```

### Output

```text
0  -8
1  -16
2  -25

ptype: Series
```

`prices.rsub(boost)` computes exactly what `boost.sub(prices)` would: `2 - 10`,
`4 - 20`, `5 - 30`.

## Notes

- `prices.rsub(x)` always equals `x - prices` (or, for the `Thread` overload,
  `boost.sub(prices)`) — the mirror image of [`sub()`](sub.md), not a synonym for it.
- See [`add()`](add.md) for the family-wide shared mechanics (positional matching,
  length-mismatch throw).

## See Also

- [`Thread::sub`](sub.md): the forward form.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rstrip.html">
    <span class="label">Previous</span>
    <span class="title">rstrip</span>
  </a>
  <a class="next-link" href="rtruediv.html">
    <span class="label">Next</span>
    <span class="title">rtruediv</span>
  </a>
</div>
