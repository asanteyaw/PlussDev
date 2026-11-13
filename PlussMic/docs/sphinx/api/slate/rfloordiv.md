<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7b9c3a40d08a -->

# Slate::rfloordiv

[Back to Slate API Reference](../../slate_api_reference)

## Signatures

```cpp
Slate Slate::rfloordiv(double other) const
Slate Slate::rfloordiv(const Slate& other) const
```

## What It Does

The reflected form of [`floordiv()`](floordiv.md) — computes
`floor(other / (*this))` instead of `floor((*this) / other)`.

## Parameters

**other** (`double` or `const Slate&`)

Same shape as [`floordiv()`](floordiv.md).

## Returns

`Slate`
    A new slate holding `floor(other / (*this))`, elementwise.

## Examples

Example 1. Floor-divide a scalar by every value.

```cpp
using namespace pluss::table;

auto amounts = Slate(pluss::make_series({17.0, 26.0, 33.0}));
auto out = amounts.rfloordiv(100.0);
std::cout << out;
```

### Output

```text
0  5
1  3
2  3
ptype: Series
```

`floor(100 / 17) = floor(5.88) = 5`; `floor(100 / 26) = floor(3.84) = 3`;
`floor(100 / 33) = floor(3.03) = 3` — how many whole `amounts`-worth fit into a
budget of `100`, the opposite framing from [`amounts.floordiv(...)`](floordiv.md).

Example 2. Reflected against another slate.

```cpp
auto multiples = Slate(pluss::make_series({50.0, 80.0, 90.0}));
auto out_pair = amounts.rfloordiv(multiples);
std::cout << out_pair;
```

### Output

```text
0  2
1  3
2  2
ptype: Series
```

`amounts.rfloordiv(multiples)` computes exactly what `multiples.floordiv(amounts)`
would: `floor(50 / 17) = 2`, `floor(80 / 26) = 3`, `floor(90 / 33) = 2`.

## Notes

- `amounts.rfloordiv(x)` always equals `floor(x / amounts)` — the mirror image of
  [`floordiv()`](floordiv.md).
- See [`rmod()`](rmod.md) for the complementary reflected remainder.

## See Also

- [`Slate::floordiv`](floordiv.md): the forward form.
- [`Slate::rmod`](rmod.md): the complementary reflected remainder operator.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="replace.html">
    <span class="label">Previous</span>
    <span class="title">replace</span>
  </a>
  <a class="next-link" href="rmod.html">
    <span class="label">Next</span>
    <span class="title">rmod</span>
  </a>
</div>
