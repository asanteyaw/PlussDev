<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: baee923d137d -->

# Thread::mod

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::mod(double other) const -> Thread
Thread::mod(const Thread& other) const -> Thread
```

## What It Does

Elementwise remainder — the complement to [`floordiv()`](floordiv.md): for each
position, `divisor * floordiv_result + mod_result` reconstructs the original
dividend. Follows [`add()`](add.md)'s shared mechanics (positional matching,
length-mismatch throw, no `Note`-payload support).

```{important} Negative operands are deliberately not covered
`mod()` delegates to the underlying `Series` library's own `remainder()` primitive,
which is not implemented in this repository — its exact sign convention for negative
dividends or divisors can't be verified here. Every example on this page uses
positive operands only, where the convention is unambiguous.
```

## Parameters

**other** (`double` or `const Thread&`)

The divisor(s). For the `Thread` overload, must be the same length as `*this`.

## Returns

`Thread`
    A new thread, same length as `*this`, holding the elementwise remainder.

## Examples

Example 1. Remainder against a scalar divisor.

```cpp
using namespace pluss::table;

auto amounts = Thread(pluss::make_series({17.0, 26.0, 33.0}));
auto out = amounts.mod(5.0);
std::cout << out;
```

### Output

```text
0  2
1  1
2  3
ptype: Series
```

`17 = 5*3 + 2`, `26 = 5*5 + 1`, `33 = 5*6 + 3` — matching
[`amounts.floordiv(5.0)`](floordiv.md)'s `3, 5, 6` from the same dividends.

Example 2. Remainder against another thread, positionally.

```cpp
auto divisors = Thread(pluss::make_series({4.0, 7.0, 9.0}));
auto out_pair = amounts.mod(divisors);
std::cout << out_pair;
```

### Output

```text
0  1
1  5
2  6
ptype: Series
```

`17 = 4*4 + 1`, `26 = 7*3 + 5`, `33 = 9*3 + 6`.

## Notes

- See [`add()`](add.md) for the family-wide mechanics: positional matching and the
  length-mismatch throw.
- See [`floordiv()`](floordiv.md) for the complementary quotient — the two methods
  are computed independently (not derived from each other), but always satisfy
  `divisor * floordiv_result + mod_result == dividend` for the positive operands
  shown here.

## See Also

- [`Thread::rmod`](rmod.md): the reflected form.
- [`Thread::floordiv`](floordiv.md): the complementary floored quotient.
- [`Thread::add`](add.md): the canonical reference for this family's shared
  mechanics.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="min.html">
    <span class="label">Previous</span>
    <span class="title">min</span>
  </a>
  <a class="next-link" href="mode.html">
    <span class="label">Next</span>
    <span class="title">mode</span>
  </a>
</div>
