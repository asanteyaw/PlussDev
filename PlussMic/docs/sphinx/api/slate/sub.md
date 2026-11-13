<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 72085fa5e049 -->

# Slate::sub

[Back to Slate API Reference](../../slate_api_reference)

## Signatures

```cpp
Slate Slate::sub(double other) const
Slate Slate::sub(const Slate& other) const
```

## What It Does

Elementwise subtraction, following the same shared mechanics documented on
[`add()`](add.md): the `double` overload subtracts the same scalar from every value;
the `Slate` overload subtracts positionally and requires matching lengths.
Subtraction is not commutative, so — unlike `add()` — `sub()` and its reflected form
[`rsub()`](rsub.md) genuinely produce different results, even for plain numbers.

Unlike `add()`, there is no `Note`-payload special case for `sub()` — subtracting one
string slate from another always throws.

## Parameters

**other** (`double` or `const Slate&`)

The value(s) to subtract. For the `Slate` overload, must be the same length as
`*this`.

## Returns

`Slate`
    A new slate, same length as `*this`, holding the elementwise difference.

## Examples

Example 1. Subtract a scalar from every value.

```cpp
using namespace pluss::table;

auto prices = Slate(pluss::make_series({10.0, 20.0, 30.0}));
auto out = prices.sub(5.0);
std::cout << out;
```

### Output

```text
0  5
1  15
2  25

ptype: Series
```

Example 2. Subtract two slates positionally.

```cpp
auto boost = Slate(pluss::make_series({2.0, 4.0, 5.0}));
auto out_pair = prices.sub(boost);
std::cout << out_pair;
```

### Output

```text
0  8
1  16
2  25

ptype: Series
```

`10 - 2`, `20 - 4`, `30 - 5` — `prices` is always the left-hand (minuend) side when
calling `prices.sub(boost)`. See [`rsub()`](rsub.md) for the flipped order.

## Notes

- See [`add()`](add.md) for the family-wide mechanics: positional matching, the
  length-mismatch throw (`"Slate::sub(Slate): positional operation requires
  identical lengths"`), and how display labels carry through.
- `sub()` has no `Note`-payload special case — subtraction between two `Note`
  slates throws `"unsupported payload combination"`, unlike `add()`'s
  concatenation exception.

## See Also

- [`Slate::rsub`](rsub.md): the reflected form — genuinely different results here,
  unlike the commutative `add()`/`radd()` pair.
- [`Slate::add`](add.md): the canonical reference for this family's shared
  mechanics.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="strip.html">
    <span class="label">Previous</span>
    <span class="title">strip</span>
  </a>
  <a class="next-link" href="sum.html">
    <span class="label">Next</span>
    <span class="title">sum</span>
  </a>
</div>
