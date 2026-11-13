<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a5b58000ec8c -->

# Slate::div

[Back to Slate API Reference](../../slate_api_reference)

## Signatures

```cpp
Slate Slate::div(double other) const
Slate Slate::div(const Slate& other) const
```

## What It Does

Elementwise division, following [`add()`](add.md)'s shared mechanics (positional
matching, length-mismatch throw, no `Note`-payload support). `div()` delegates to the
underlying `Series` library's own `div()` algebra, which is a distinct code path from
[`truediv()`](truediv.md) (which always promotes to floating point via
`true_divide()`). For the `Float64`-typed slates used throughout this reference, the
two produce identical results; the distinction can matter for integer-typed slates,
where `div()`'s exact type-preservation behavior comes from outside this repository
and isn't asserted here — prefer [`truediv()`](truediv.md) when you specifically want
guaranteed floating-point division regardless of input type.

## Parameters

**other** (`double` or `const Slate&`)

The divisor(s). For the `Slate` overload, must be the same length as `*this`.

## Returns

`Slate`
    A new slate, same length as `*this`, holding the elementwise quotient.

## Examples

Example 1. Divide every value by a scalar.

```cpp
using namespace pluss::table;

auto prices = Slate(pluss::make_series({10.0, 20.0, 30.0}));
auto out = prices.div(4.0);
std::cout << out;
```

### Output

```text
0  2.5
1  5
2  7.5

ptype: Series
```

Example 2. Divide two slates positionally.

```cpp
auto splits = Slate(pluss::make_series({4.0, 8.0, 9.0}));
auto out_pair = prices.div(splits);
std::cout << out_pair;
```

### Output

```text
0  2.5
1  2.5
2  3.333333333

ptype: Series
```

`30 / 9` doesn't divide evenly — the result is displayed to 10 significant digits
(`3.333333333`), the same formatting every numeric slate uses.

## Notes

- See [`add()`](add.md) for the family-wide mechanics: positional matching and the
  length-mismatch throw (`"Slate::div(Slate): positional operation requires
  identical lengths"`).
- `div()` has no `Note`-payload special case, unlike `add()`/`mul()`.
- For `Float64` data, `div()` and [`truediv()`](truediv.md) always agree — reach for
  `truediv()` when the underlying type might not be floating-point and you need
  float division guaranteed.

## See Also

- [`Slate::truediv`](truediv.md): the always-floating-point counterpart.
- [`Slate::rdiv`](rdiv.md): the reflected form.
- [`Slate::floordiv`](floordiv.md), [`Slate::mod`](mod.md): related division-family
  operators with their own rounding behavior.
- [`Slate::add`](add.md): the canonical reference for this family's shared
  mechanics.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="distinct.html">
    <span class="label">Previous</span>
    <span class="title">distinct</span>
  </a>
  <a class="next-link" href="dropnull.html">
    <span class="label">Next</span>
    <span class="title">dropnull</span>
  </a>
</div>
