<!-- pluss-hand-authored -->

# Thread::div

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::div(double other) const -> Thread
Thread::div(const Thread& other) const -> Thread
```

## What It Does

Elementwise division, following [`add()`](add.md)'s shared mechanics (positional
matching, length-mismatch throw, no `Note`-payload support). `div()` delegates to the
underlying `Series` library's own `div()` algebra, which is a distinct code path from
[`truediv()`](truediv.md) (which always promotes to floating point via
`true_divide()`). For the `Float64`-typed threads used throughout this reference, the
two produce identical results; the distinction can matter for integer-typed threads,
where `div()`'s exact type-preservation behavior comes from outside this repository
and isn't asserted here — prefer [`truediv()`](truediv.md) when you specifically want
guaranteed floating-point division regardless of input type.

## Parameters

**other** (`double` or `const Thread&`)

The divisor(s). For the `Thread` overload, must be the same length as `*this`.

## Returns

`Thread`
    A new thread, same length as `*this`, holding the elementwise quotient.

## Examples

Example 1. Divide every value by a scalar.

```cpp
using namespace pluss::table;

auto prices = Thread(pluss::make_series({10.0, 20.0, 30.0}));
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

Example 2. Divide two threads positionally.

```cpp
auto splits = Thread(pluss::make_series({4.0, 8.0, 9.0}));
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
(`3.333333333`), the same formatting every numeric thread uses.

## Notes

- See [`add()`](add.md) for the family-wide mechanics: positional matching and the
  length-mismatch throw (`"Thread::div(Thread): positional operation requires
  identical lengths"`).
- `div()` has no `Note`-payload special case, unlike `add()`/`mul()`.
- For `Float64` data, `div()` and [`truediv()`](truediv.md) always agree — reach for
  `truediv()` when the underlying type might not be floating-point and you need
  float division guaranteed.

## See Also

- [`Thread::truediv`](truediv.md): the always-floating-point counterpart.
- [`Thread::rdiv`](rdiv.md): the reflected form.
- [`Thread::floordiv`](floordiv.md), [`Thread::mod`](mod.md): related division-family
  operators with their own rounding behavior.
- [`Thread::add`](add.md): the canonical reference for this family's shared
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
