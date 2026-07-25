<!-- pluss-hand-authored -->

# Thread::ge

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::ge(double other) const -> Thread
Thread::ge(std::string_view other) const -> Thread
Thread::ge(const char* other) const -> Thread
Thread::ge(const Thread& other) const -> Thread
```

## What It Does

Elementwise "greater than or equal", the mirror image of [`le()`](le.md), following
[`eq()`](eq.md)'s shared overload shape and positional matching. Like `le()`, the
string overload stringifies numeric values before comparing lexicographically — see
[`lt()`](lt.md) for a concrete example of why that can disagree with numeric
ordering.

## Parameters

**other** (`double`, `std::string_view`, or `const Thread&`)

Same shape as [`eq()`](eq.md).

## Returns

`Thread`
    A new `Bool`-typed thread, same length as `*this`, holding the elementwise
    "greater than or equal" result.

## Examples

Example 1. Compare every value against a scalar.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({72.0, 85.0, 91.0}));
auto out = scores.ge(85.0);
std::cout << out;
```

### Output

```text
0  false
1  true
2  true

ptype: Series
```

Position `1` is `true` here (`85 >= 85`) where [`gt(85.0)`](gt.md) gave `false` —
the only difference between the two operators.

Example 2. Compare two threads positionally.

```cpp
auto targets = Thread(pluss::make_series({72.0, 80.0, 95.0}));
auto out_pair = scores.ge(targets);
std::cout << out_pair;
```

### Output

```text
0  true
1  true
2  false

ptype: Series
```

Example 3. String comparison on a `Note` thread.

```cpp
auto names = Thread(pluss::make_note({"Ada", "Bola", "Chen"}));
auto out_names = names.ge("Bola");
std::cout << out_names;
```

### Output

```text
0  false
1  true
2  true

ptype: Series
```

## Notes

- See [`eq()`](eq.md) for the family-wide mechanics.
- `ge(x)` is always the exact logical negation of `lt(x)` for the same argument.

## See Also

- [`Thread::gt`](gt.md): the strict form.
- [`Thread::le`](le.md): the opposite direction's inclusive form.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fullmatch.html">
    <span class="label">Previous</span>
    <span class="title">fullmatch</span>
  </a>
  <a class="next-link" href="gt.html">
    <span class="label">Next</span>
    <span class="title">gt</span>
  </a>
</div>
