<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 197cb83c3720 -->

# Thread::floordiv

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::floordiv(double other) const -> Thread
Thread::floordiv(const Thread& other) const -> Thread
```

## What It Does

Floor division: computes `floor((*this) / other)`, elementwise — always the
mathematical floor of the true (floating-point) quotient, not a truncation toward
zero. Follows [`add()`](add.md)'s shared mechanics (positional matching,
length-mismatch throw, no `Note`-payload support).

```{important} Floor, not truncation
`floordiv()` is defined as `floor(true_divide(a, b))`. For positive operands this
matches ordinary integer division; the distinction only shows up with negative
operands, which this reference deliberately avoids — the exact rounding convention
for negative divisors depends on the underlying `Series` library and isn't asserted
here.
```

## Parameters

**other** (`double` or `const Thread&`)

The divisor(s). For the `Thread` overload, must be the same length as `*this`.

## Returns

`Thread`
    A new thread, same length as `*this`, holding the elementwise floored quotient.

## Examples

Example 1. Divide every value by a scalar and floor the result.

```cpp
using namespace pluss::table;

auto amounts = Thread(pluss::make_series({17.0, 26.0, 33.0}));
auto out = amounts.floordiv(5.0);
std::cout << out;
```

### Output

```text
0  3
1  5
2  6
ptype: Series
```

`floor(17 / 5) = floor(3.4) = 3`; `floor(26 / 5) = floor(5.2) = 5`;
`floor(33 / 5) = floor(6.6) = 6` — how many whole groups of `5` fit into each amount.

Example 2. Floor-divide two threads positionally.

```cpp
auto divisors = Thread(pluss::make_series({4.0, 7.0, 9.0}));
auto out_pair = amounts.floordiv(divisors);
std::cout << out_pair;
```

### Output

```text
0  4
1  3
2  3
ptype: Series
```

`floor(17 / 4) = 4`, `floor(26 / 7) = 3`, `floor(33 / 9) = 3`.

## Notes

- See [`add()`](add.md) for the family-wide mechanics: positional matching and the
  length-mismatch throw.
- See [`mod()`](mod.md) for the complementary remainder: for each position,
  `divisor * floordiv_result + mod_result` reconstructs the original dividend.
- Internally computed as `floor(true_divide(a, b))`, always promoting to
  floating point first — the same underlying `true_divide()` primitive
  [`truediv()`](truediv.md) uses.

## See Also

- [`Thread::rfloordiv`](rfloordiv.md): the reflected form.
- [`Thread::mod`](mod.md): the complementary remainder operator.
- [`Thread::truediv`](truediv.md): the unfloored quotient this method floors.
- [`Thread::add`](add.md): the canonical reference for this family's shared
  mechanics.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="filter.html">
    <span class="label">Previous</span>
    <span class="title">filter</span>
  </a>
  <a class="next-link" href="flow.html">
    <span class="label">Next</span>
    <span class="title">flow</span>
  </a>
</div>
