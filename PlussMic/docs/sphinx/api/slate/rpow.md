<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 53c75ab7fe39 -->

# Slate::rpow

[Back to Slate API Reference](../../slate_api_reference)

## Signatures

```cpp
Slate::rpow(double other) const -> Slate
Slate::rpow(const Slate& other) const -> Slate
```

## What It Does

The reflected form of [`pow()`](pow.md) — computes `other ^ (*this)` instead of
`(*this) ^ other`.

## Parameters

**other** (`double` or `const Slate&`)

Same shape as [`pow()`](pow.md).

## Returns

`Slate`
    A new `Float64` slate holding `other ^ (*this)`, elementwise.

## Examples

Example 1. Raise a scalar to each value's power.

```cpp
using namespace pluss::table;

auto base = Slate(pluss::make_series({2.0, 3.0, 4.0}));
auto out = base.rpow(3.0);
std::cout << out;
```

### Output

```text
0  9
1  27
2  81
ptype: Series
```

`3^2 = 9`, `3^3 = 27`, `3^4 = 81` — the base and exponent roles are swapped from
[`base.pow(3.0)`](pow.md), which gave `8, 27, 64`.

Example 2. Reflected against another slate.

```cpp
auto exps = Slate(pluss::make_series({1.0, 2.0, 3.0}));
auto out_pair = base.rpow(exps);
std::cout << out_pair;
```

### Output

```text
0  1
1  8
2  81
ptype: Series
```

`base.rpow(exps)` computes exactly what `exps.pow(base)` would: `1^2 = 1`,
`2^3 = 8`, `3^4 = 81`.

## Notes

- `base.rpow(x)` always equals `x ^ base` — the mirror image of [`pow()`](pow.md).

## See Also

- [`Slate::pow`](pow.md): the forward form.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="round.html">
    <span class="label">Previous</span>
    <span class="title">round</span>
  </a>
  <a class="next-link" href="rstrip.html">
    <span class="label">Next</span>
    <span class="title">rstrip</span>
  </a>
</div>
