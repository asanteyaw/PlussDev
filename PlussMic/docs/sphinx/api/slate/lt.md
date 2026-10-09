<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7ba6670ad6b3 -->

# Slate::lt

[Back to Slate API Reference](../../slate_api_reference)

## Signatures

```cpp
Slate::lt(double other) const -> Slate
Slate::lt(std::string_view other) const -> Slate
Slate::lt(const char* other) const -> Slate
Slate::lt(const Slate& other) const -> Slate
```

## What It Does

Elementwise "less than", following [`eq()`](eq.md)'s shared overload shape and
positional matching. Unlike a `strict` toggle you might expect from
[`DataFrame::lt`](../dataframe/lt.md), `Slate::lt()` has no `bool strict` parameter
at all — it's always strict (use [`le()`](le.md) for the inclusive form).

```{important} The string overload compares text, not numbers
On a numeric slate, `lt(std::string_view)` stringifies each value and does a plain
lexicographic string comparison against `other` — it does **not** parse `other` as a
number. This can disagree sharply with numeric ordering: `"10"` sorts *before* `"9"`
lexicographically (`'1' < '9'` as the first character), even though `10 > 9`
numerically. Example 3 below shows this concretely.
```

## Parameters

**other** (`double`, `std::string_view`, or `const Slate&`)

Same shape as [`eq()`](eq.md).

## Returns

`Slate`
    A new `Bool`-typed slate, same length as `*this`, holding the elementwise
    "less than" result.

## Examples

Example 1. Compare every value against a scalar.

```cpp
using namespace pluss::table;

auto scores = Slate(pluss::make_series({72.0, 85.0, 91.0}));
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

Example 2. Compare two slates positionally.

```cpp
auto targets = Slate(pluss::make_series({72.0, 80.0, 95.0}));
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

Example 3. The string-overload lexicographic gotcha on a numeric slate.

```cpp
auto values = Slate(pluss::make_series({2.0, 10.0, 21.0}));

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
  length-mismatch throw (`"Slate::lt(Slate): positional comparison requires
  identical lengths"`).
- There is no `strict`/inclusive toggle on `Slate`'s comparison family the way
  [`DataFrame::lt`](../dataframe/lt.md) has — `lt()` is always strict;
  [`le()`](le.md) is the separate, independently-implemented inclusive form.

## See Also

- [`Slate::le`](le.md): the inclusive ("less than or equal") form.
- [`Slate::gt`](gt.md): the opposite direction.
- [`Slate::eq`](eq.md): the canonical reference for this family's shared mechanics.

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
