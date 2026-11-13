<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d0b361079c35 -->

# Slate::eq

[Back to Slate API Reference](../../slate_api_reference)

## Signatures

```cpp
Slate Slate::eq(double other) const
Slate Slate::eq(std::string_view other) const
Slate Slate::eq(const char* other) const
Slate Slate::eq(const Slate& other) const
```

## What It Does

Elementwise equality, returning a `Bool`-typed `Series` slate. The `double`
overload compares every value against the same scalar; the `Slate` overload
compares two slates **positionally** (same length required, no case-label
alignment); the `std::string_view` overload (with a `const char*` convenience
wrapper that just forwards to it) behaves differently depending on the calling
slate's kind:

- On a `Note` (string) slate, it compares each string directly against `other`.
- On any other kind (a numeric `Series` slate, for example), it **stringifies**
  each value first, then does a plain string comparison against `other`. It does not
  parse `other` as a number and compare numerically.

```{important} eq() is the canonical comparison-family reference
`ne`, `lt`, `le`, `gt`, and `ge` all share this exact overload shape and
positional-matching rule. Their own pages focus on what's different about each
operator; this page covers the shared mechanics once. See [`lt()`](lt.md)
specifically for why the string-overload's stringify-then-compare behavior matters
more for ordering operators than it does for equality.
```

## Parameters

**other** (`double`, `std::string_view`, or `const Slate&`)

The value to compare against. For the `Slate` overload, must be the same length as
`*this`.

## Returns

`Slate`
    A new `Bool`-typed slate, same length as `*this`, holding the elementwise
    equality result.

## Examples

Example 1. Compare every value against a scalar.

```cpp
using namespace pluss::table;

auto scores = Slate(pluss::make_series({72.0, 85.0, 91.0}));
auto out = scores.eq(85.0);
std::cout << out;
```

### Output

```text
0  false
1  true
2  false

ptype: Series
```

Example 2. Compare two slates positionally.

```cpp
auto targets = Slate(pluss::make_series({72.0, 80.0, 95.0}));
auto out_pair = scores.eq(targets);
std::cout << out_pair;
```

### Output

```text
0  true
1  false
2  false

ptype: Series
```

Only position `0` matches (`72 == 72`); the others differ (`85 != 80`, `91 != 95`).

Example 3. String equality on a `Note` slate compares text directly.

```cpp
auto names = Slate(pluss::make_note({"Ada", "Bola", "Chen"}));
auto out_names = names.eq("Bola");
std::cout << out_names;
```

### Output

```text
0  false
1  true
2  false

ptype: Series
```

Example 4. String equality on a numeric slate stringifies first.

```cpp
auto values = Slate(pluss::make_series({9.0, 10.0, 11.0}));
auto out_values = values.eq("10");
std::cout << out_values;
```

### Output

```text
0  false
1  true
2  false

ptype: Series
```

`10.0` stringifies to `"10"`, which matches the literal `"10"` — for equality
specifically, stringify-then-compare and parse-then-compare-numerically happen to
agree here. That's not true for the ordering operators; see [`lt()`](lt.md) for a
case where it matters.

Example 5. A length mismatch throws rather than aligning or padding.

```cpp
auto too_short = Slate(pluss::make_series({1.0, 2.0}));
try {
  auto bad = scores.eq(too_short);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Slate::eq(Slate): positional comparison requires identical lengths
```

## Notes

- The comparison family also accepts `Timeline`/`TimeIndex`-kind slates on the
  `Slate` overload (comparing timestamps), and `Note`-vs-`Note` on the same
  overload (comparing strings) — this page focuses on the numeric case, which is
  the most common.
- Mixing incompatible payload kinds on the `Slate` overload (for example, a numeric
  slate against a `Note` slate) throws `"unsupported payload combination"`.
- Every method in this family reports its own name in error messages
  (`Slate::eq(Slate)`, `Slate::lt(Slate)`, and so on), not a shared generic one.

## See Also

- [`Slate::ne`](ne.md): the complementary inequality check.
- [`Slate::lt`](lt.md), [`Slate::le`](le.md), [`Slate::gt`](gt.md),
  [`Slate::ge`](ge.md): the ordering operators, sharing this overload shape but with
  a real gotcha in the string overload.
- [`DataFrame::eq`](../dataframe/eq.md): the multi-feature counterpart.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ends.html">
    <span class="label">Previous</span>
    <span class="title">ends</span>
  </a>
  <a class="next-link" href="exp.html">
    <span class="label">Next</span>
    <span class="title">exp</span>
  </a>
</div>
