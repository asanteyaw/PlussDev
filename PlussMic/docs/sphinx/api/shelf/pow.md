<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 92569f579c26 -->

# Shelf::pow

[Back to Shelf API Reference](../../shelf_api_reference)

## Signatures

```cpp
Shelf Shelf::pow(double other) const
Shelf Shelf::pow(const Shelf& other) const
```

## What It Does

Elementwise exponentiation, computed via `std::pow` after promoting both operands to
`Float64`. Follows [`add()`](add.md)'s shared mechanics (positional matching,
length-mismatch throw, no `Note`-payload support). This reference sticks to positive
bases and small exponents throughout — `std::pow`'s behavior for negative bases with
fractional exponents (which can produce `NaN`) isn't exercised here.

## Parameters

**other** (`double` or `const Shelf&`)

The exponent(s). For the `Shelf` overload, must be the same length as `*this`.

## Returns

`Shelf`
    A new `Float64` shelf, same length as `*this`, holding the elementwise power.

## Examples

Example 1. Raise every value to the same scalar power.

```cpp
using namespace pluss::table;

auto base = Shelf(pluss::make_series({2.0, 3.0, 4.0}));
auto out = base.pow(3.0);
std::cout << out;
```

### Output

```text
0  8
1  27
2  64
ptype: Series
```

`2^3 = 8`, `3^3 = 27`, `4^3 = 64`.

Example 2. Raise each value to its own exponent, positionally.

```cpp
auto exps = Shelf(pluss::make_series({1.0, 2.0, 3.0}));
auto out_pair = base.pow(exps);
std::cout << out_pair;
```

### Output

```text
0  2
1  9
2  64
ptype: Series
```

`2^1 = 2`, `3^2 = 9`, `4^3 = 64` — each base raised to its own matching exponent.

## Notes

- See [`add()`](add.md) for the family-wide mechanics: positional matching and the
  length-mismatch throw (here, `"Shelf::pow: operand length mismatch"` for the
  `Shelf` overload's internal check, distinct from the generic
  `"positional operation requires identical lengths"` message the rest of the family
  uses).
- No `Note`-payload special case — `pow()` requires numeric shelves only.

## See Also

- [`Shelf::rpow`](rpow.md): the reflected form.
- [`Shelf::add`](add.md): the canonical reference for this family's shared
  mechanics.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="plot.html">
    <span class="label">Previous</span>
    <span class="title">plot</span>
  </a>
  <a class="next-link" href="prod.html">
    <span class="label">Next</span>
    <span class="title">prod</span>
  </a>
</div>
