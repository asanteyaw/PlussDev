<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5d4864f2c1cb -->

# Slate::gt

[Back to Slate API Reference](../../slate_api_reference)

## Signatures

```cpp
Slate Slate::gt(double other) const
Slate Slate::gt(std::string_view other) const
Slate Slate::gt(const char* other) const
Slate Slate::gt(const Slate& other) const
```

## What It Does

Elementwise "greater than", the mirror image of [`lt()`](lt.md), following
[`eq()`](eq.md)'s shared overload shape and positional matching. Like `lt()`, the
string overload stringifies numeric values before comparing lexicographically — see
[`lt()`](lt.md) for a concrete example of why that can disagree with numeric
ordering.

## Parameters

**other** (`double`, `std::string_view`, or `const Slate&`)

Same shape as [`eq()`](eq.md).

## Returns

`Slate`
    A new `Bool`-typed slate, same length as `*this`, holding the elementwise
    "greater than" result.

## Examples

Example 1. Compare every value against a scalar.

```cpp
using namespace pluss::table;

auto scores = Slate(pluss::make_series({72.0, 85.0, 91.0}));
auto out = scores.gt(85.0);
std::cout << out;
```

### Output

```text
0  false
1  false
2  true

ptype: Series
```

The exact negation of [`scores.le(85.0)`](le.md)'s `true, true, false`.

Example 2. Compare two slates positionally.

```cpp
auto targets = Slate(pluss::make_series({72.0, 80.0, 95.0}));
auto out_pair = scores.gt(targets);
std::cout << out_pair;
```

### Output

```text
0  false
1  true
2  false

ptype: Series
```

Example 3. String comparison on a `Note` slate.

```cpp
auto names = Slate(pluss::make_note({"Ada", "Bola", "Chen"}));
auto out_names = names.gt("Bola");
std::cout << out_names;
```

### Output

```text
0  false
1  false
2  true

ptype: Series
```

## Notes

- See [`eq()`](eq.md) for the family-wide mechanics.
- `gt(x)` is always the exact logical negation of `le(x)` for the same argument —
  there is no `NULL`-handling subtlety that would break that relationship here.

## See Also

- [`Slate::ge`](ge.md): the inclusive form.
- [`Slate::lt`](lt.md): the opposite direction, including the lexicographic gotcha
  that also applies to `gt()`'s string overload.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="group.html">
    <span class="label">Previous</span>
    <span class="title">group</span>
  </a>
  <a class="next-link" href="has.html">
    <span class="label">Next</span>
    <span class="title">has</span>
  </a>
</div>
