<!-- pluss-hand-authored -->

# Thread::lt

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::lt(double other) const -> Thread
Thread::lt(std::string_view other) const -> Thread
Thread::lt(const char* other) const -> Thread
Thread::lt(const Thread& other) const -> Thread
```

## What It Does

Elementwise "less than", following [`eq()`](eq.md)'s shared overload shape and
positional matching. Unlike a `strict` toggle you might expect from
[`DataFrame::lt`](../dataframe/lt.md), `Thread::lt()` has no `bool strict` parameter
at all — it's always strict (use [`le()`](le.md) for the inclusive form).

```{important} The string overload compares text, not numbers
On a numeric thread, `lt(std::string_view)` stringifies each value and does a plain
lexicographic string comparison against `other` — it does **not** parse `other` as a
number. This can disagree sharply with numeric ordering: `"10"` sorts *before* `"9"`
lexicographically (`'1' < '9'` as the first character), even though `10 > 9`
numerically. Example 3 below shows this concretely.
```

## Parameters

**other** (`double`, `std::string_view`, or `const Thread&`)

Same shape as [`eq()`](eq.md).

## Returns

`Thread`
    A new `Bool`-typed thread, same length as `*this`, holding the elementwise
    "less than" result.

## Examples

Example 1. Compare every value against a scalar.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({72.0, 85.0, 91.0}));
auto out = scores.lt(85.0);
std::cout << out;
```

### Output

```text
0  true
1  false
2  false

ptype: Series
```

Example 2. Compare two threads positionally.

```cpp
auto targets = Thread(pluss::make_series({72.0, 80.0, 95.0}));
auto out_pair = scores.lt(targets);
std::cout << out_pair;
```

### Output

```text
0  false
1  false
2  true

ptype: Series
```

Example 3. The string-overload lexicographic gotcha on a numeric thread.

```cpp
auto values = Thread(pluss::make_series({2.0, 10.0, 21.0}));

auto lexicographic = values.lt("9");
auto numeric = values.lt(9.0);
std::cout << lexicographic << "\n" << numeric;
```

### Output

```text
0  true
1  true
2  true

ptype: Series
0  true
1  false
2  false

ptype: Series
```

Calling `lt("9")` stringifies `2.0`, `10.0`, `21.0` to `"2"`, `"10"`, `"21"` and
compares each lexicographically against `"9"` — since every one of those strings
starts with a digit less than `'9'`, **all three** come back `true`, even though only
`2` is actually less than `9`. Calling `lt(9.0)` (the numeric overload) gives the
mathematically correct `true, false, false`. Passing a string when you mean a number
silently changes what gets compared — always prefer the `double` overload for
numeric thresholds.

## Notes

- See [`eq()`](eq.md) for the family-wide mechanics: positional matching and the
  length-mismatch throw (`"Thread::lt(Thread): positional comparison requires
  identical lengths"`).
- There is no `strict`/inclusive toggle on `Thread`'s comparison family the way
  [`DataFrame::lt`](../dataframe/lt.md) has — `lt()` is always strict;
  [`le()`](le.md) is the separate, independently-implemented inclusive form.

## See Also

- [`Thread::le`](le.md): the inclusive ("less than or equal") form.
- [`Thread::gt`](gt.md): the opposite direction.
- [`Thread::eq`](eq.md): the canonical reference for this family's shared mechanics.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lstrip.html">
    <span class="label">Previous</span>
    <span class="title">lstrip</span>
  </a>
  <a class="next-link" href="lxor.html">
    <span class="label">Next</span>
    <span class="title">lxor</span>
  </a>
</div>
