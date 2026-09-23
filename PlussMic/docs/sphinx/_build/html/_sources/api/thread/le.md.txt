<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c1b849208be2 -->

# Thread::le

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::le(double other) const -> Thread
Thread::le(std::string_view other) const -> Thread
Thread::le(const char* other) const -> Thread
Thread::le(const Thread& other) const -> Thread
```

## What It Does

Elementwise "less than or equal", following [`eq()`](eq.md)'s shared overload shape
and positional matching. Like [`lt()`](lt.md), the string overload stringifies
numeric values before comparing — see [`lt()`](lt.md) for why that can disagree with
numeric ordering.

## Parameters

**other** (`double`, `std::string_view`, or `const Thread&`)

Same shape as [`eq()`](eq.md).

## Returns

`Thread`
    A new `Bool`-typed thread, same length as `*this`, holding the elementwise
    "less than or equal" result.

## Examples

Example 1. Compare every value against a scalar.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({72.0, 85.0, 91.0}));
auto out = scores.le(85.0);
std::cout << out;
```

### Output

```text
0  true
1  true
2  false

ptype: Series
```

Position `1` is `true` here (`85 <= 85`) where [`lt(85.0)`](lt.md) gave `false` —
the only difference between the two operators.

Example 2. Compare two threads positionally.

```cpp
auto targets = Thread(pluss::make_series({72.0, 80.0, 95.0}));
auto out_pair = scores.le(targets);
std::cout << out_pair;
```

### Output

```text
0  true
1  false
2  true

ptype: Series
```

Example 3. String comparison on a `Note` thread.

```cpp
auto names = Thread(pluss::make_note({"Ada", "Bola", "Chen"}));
auto out_names = names.le("Bola");
std::cout << out_names;
```

### Output

```text
0  true
1  true
2  false

ptype: Series
```

`"Ada" <= "Bola"` and `"Bola" <= "Bola"` are both true lexicographically;
`"Chen" <= "Bola"` is false.

## Notes

- See [`eq()`](eq.md) for the family-wide mechanics.
- See [`lt()`](lt.md) for the lexicographic-vs-numeric gotcha on the string
  overload — it applies here too.

## See Also

- [`Thread::lt`](lt.md): the strict form.
- [`Thread::ge`](ge.md): the opposite direction's inclusive form.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="land.html">
    <span class="label">Previous</span>
    <span class="title">land</span>
  </a>
  <a class="next-link" href="length.html">
    <span class="label">Next</span>
    <span class="title">length</span>
  </a>
</div>
