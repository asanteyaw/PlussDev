<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 19b7dcbb9b91 -->

# Thread::sub

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::sub(double other) const -> Thread
Thread::sub(const Thread& other) const -> Thread
```

## What It Does

Elementwise subtraction, following the same shared mechanics documented on
[`add()`](add.md): the `double` overload subtracts the same scalar from every value;
the `Thread` overload subtracts positionally and requires matching lengths.
Subtraction is not commutative, so — unlike `add()` — `sub()` and its reflected form
[`rsub()`](rsub.md) genuinely produce different results, even for plain numbers.

Unlike `add()`, there is no `Note`-payload special case for `sub()` — subtracting one
string thread from another always throws.

## Parameters

**other** (`double` or `const Thread&`)

The value(s) to subtract. For the `Thread` overload, must be the same length as
`*this`.

## Returns

`Thread`
    A new thread, same length as `*this`, holding the elementwise difference.

## Examples

Example 1. Subtract a scalar from every value.

```cpp
using namespace pluss::table;

auto prices = Thread(pluss::make_series({10.0, 20.0, 30.0}));
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

Example 2. Subtract two threads positionally.

```cpp
auto boost = Thread(pluss::make_series({2.0, 4.0, 5.0}));
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
  length-mismatch throw (`"Thread::sub(Thread): positional operation requires
  identical lengths"`), and how display labels carry through.
- `sub()` has no `Note`-payload special case — subtraction between two `Note`
  threads throws `"unsupported payload combination"`, unlike `add()`'s
  concatenation exception.

## See Also

- [`Thread::rsub`](rsub.md): the reflected form — genuinely different results here,
  unlike the commutative `add()`/`radd()` pair.
- [`Thread::add`](add.md): the canonical reference for this family's shared
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
