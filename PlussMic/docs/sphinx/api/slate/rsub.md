<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 091797568172 -->

# Slate::rsub

[Back to Slate API Reference](../../slate_api_reference)

## Signatures

```cpp
Slate::rsub(double other) const -> Slate
Slate::rsub(const Slate& other) const -> Slate
```

## What It Does

The reflected form of [`sub()`](sub.md). Since subtraction is **not** commutative,
`rsub()` genuinely differs from `sub()` — it computes `other - (*this)` instead of
`(*this) - other`.

## Parameters

**other** (`double` or `const Slate&`)

Same shape as [`sub()`](sub.md).

## Returns

`Slate`
    A new slate holding `other - (*this)`, elementwise.

## Examples

Example 1. Subtract every value from a scalar.

```cpp
using namespace pluss::table;

auto prices = Slate(pluss::make_series({10.0, 20.0, 30.0}));
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

Example 2. Reflected against another slate — equivalent to calling `sub()` with the
operands swapped.

```cpp
auto boost = Slate(pluss::make_series({2.0, 4.0, 5.0}));
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

- `prices.rsub(x)` always equals `x - prices` (or, for the `Slate` overload,
  `boost.sub(prices)`) — the mirror image of [`sub()`](sub.md), not a synonym for it.
- See [`add()`](add.md) for the family-wide shared mechanics (positional matching,
  length-mismatch throw).

## See Also

- [`Slate::sub`](sub.md): the forward form.

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
