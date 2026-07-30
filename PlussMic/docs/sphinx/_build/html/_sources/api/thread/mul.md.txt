<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e2f6ab7a4dc3 -->

# Thread::mul

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::mul(double other) const -> Thread
Thread::mul(const Thread& other) const -> Thread
```

## What It Does

Elementwise multiplication, following [`add()`](add.md)'s shared mechanics. `mul()`
has its own payload special case, parallel to `add()`'s `Note`-concatenation
exception: multiplying a `Note` (string) thread by a non-negative integral scalar
**repeats** each string that many times — `"ab".mul(3.0)` behaves like Python's
`"ab" * 3`, giving `"ababab"`. This only works for the scalar overload; there is no
`Note`-times-`Note` multiplication.

## Parameters

**other** (`double` or `const Thread&`)

The value(s) to multiply by. For a `Note` thread, `double` must be a non-negative
integral value (`3.0`, not `3.5`).

## Returns

`Thread`
    A new thread holding the elementwise product — or, for a `Note` thread multiplied
    by a scalar, each string repeated that many times.

## Examples

Example 1. Multiply every value by a scalar.

```cpp
using namespace pluss::table;

auto prices = Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto out = prices.mul(2.0);
std::cout << out;
```

### Output

```text
0  20
1  40
2  60

ptype: Series
```

Example 2. Multiply two threads positionally.

```cpp
auto boost = Thread(pluss::make_series({2.0, 4.0, 5.0}));
auto out_pair = prices.mul(boost);
std::cout << out_pair;
```

### Output

```text
0  20
1  80
2  150
ptype: Series
```

Example 3. A `Note` thread multiplied by a scalar repeats each string.

```cpp
auto word = Thread(pluss::make_note({"ab", "xy", "q"}));
auto out_repeat = word.mul(3.0);
std::cout << out_repeat;
```

### Output

```text
0  ababab
1  xyxyxy
2  qqq

ptype: Note
```

`"ab"` repeated `3` times is `"ababab"`; `"q"` repeated `3` times is `"qqq"`. Passing
a non-integral or negative scalar (like `2.5` or `-1.0`) throws
`"Thread::mul(double): Note repetition requires a non-negative integral scalar"`.

## Notes

- See [`add()`](add.md) for the family-wide mechanics: positional matching, the
  length-mismatch throw, and how display labels carry through.
- Note repetition only exists on the scalar overload — there is no `Note`-thread
  equivalent of `mul(const Thread&)`; multiplying two `Note` threads throws.
- [`rmul()`](rmul.md) produces identical results to `mul()` in every case, including
  `Note` repetition — multiplication is commutative for numbers, and the repetition
  special case doesn't check operand order at all.

## See Also

- [`Thread::rmul`](rmul.md): the reflected form, identical in every case.
- [`Thread::add`](add.md): the canonical reference for this family's shared
  mechanics.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mode.html">
    <span class="label">Previous</span>
    <span class="title">mode</span>
  </a>
  <a class="next-link" href="name.html">
    <span class="label">Next</span>
    <span class="title">name</span>
  </a>
</div>
