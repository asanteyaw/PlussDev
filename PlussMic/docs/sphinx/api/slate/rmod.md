<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d5e2fa8be635 -->

# Slate::rmod

[Back to Slate API Reference](../../slate_api_reference)

## Signatures

```cpp
Slate::rmod(double other) const -> Slate
Slate::rmod(const Slate& other) const -> Slate
```

## What It Does

The reflected form of [`mod()`](mod.md) — computes `other % (*this)` instead of
`(*this) % other`. Same positive-operands-only caveat as `mod()` applies: the
underlying remainder algorithm's negative-operand convention isn't verified in this
repository.

## Parameters

**other** (`double` or `const Slate&`)

Same shape as [`mod()`](mod.md).

## Returns

`Slate`
    A new slate holding `other % (*this)`, elementwise.

## Examples

Example 1. Remainder of a scalar against every value.

```cpp
using namespace pluss::table;

auto amounts = Slate(pluss::make_series({17.0, 26.0, 33.0}));
auto out = amounts.rmod(200.0);
std::cout << out;
```

### Output

```text
0  13
1  18
2  2
ptype: Series
```

`200 = 17*11 + 13`, `200 = 26*7 + 18`, `200 = 33*6 + 2` — the opposite framing from
[`amounts.mod(...)`](mod.md), where `amounts` supplied the dividends.

Example 2. Reflected against another slate.

```cpp
auto multiples = Slate(pluss::make_series({50.0, 80.0, 90.0}));
auto out_pair = amounts.rmod(multiples);
std::cout << out_pair;
```

### Output

```text
0  16
1  2
2  24
ptype: Series
```

`amounts.rmod(multiples)` computes exactly what `multiples.mod(amounts)` would:
`50 % 17 = 16`, `80 % 26 = 2`, `90 % 33 = 24`.

## Notes

- `amounts.rmod(x)` always equals `x % amounts` — the mirror image of
  [`mod()`](mod.md).
- If the scalar or `other` value at a position is smaller than the corresponding
  value in `*this`, the remainder is just that smaller value itself (dividing `0`
  whole times) — for example, `amounts.rmod(5.0)` would give `5, 5, 5` here, since
  `5` is smaller than every value in `amounts`.

## See Also

- [`Slate::mod`](mod.md): the forward form.
- [`Slate::rfloordiv`](rfloordiv.md): the complementary reflected quotient.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rfloordiv.html">
    <span class="label">Previous</span>
    <span class="title">rfloordiv</span>
  </a>
  <a class="next-link" href="rmul.html">
    <span class="label">Next</span>
    <span class="title">rmul</span>
  </a>
</div>
