<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2ae35879ebfc -->

# Slate::ne

[Back to Slate API Reference](../../slate_api_reference)

## Signatures

```cpp
Slate Slate::ne(double other) const
Slate Slate::ne(std::string_view other) const
Slate Slate::ne(const char* other) const
Slate Slate::ne(const Slate& other) const
```

## What It Does

The complement of [`eq()`](eq.md) — `ne()` reports `true` at every position where
`eq()` would report `false`, and vice versa. Same overload shape, same positional
matching, same stringify-then-compare behavior on the string overload for non-`Note`
slates.

## Parameters

**other** (`double`, `std::string_view`, or `const Slate&`)

Same shape as [`eq()`](eq.md).

## Returns

`Slate`
    A new `Bool`-typed slate, same length as `*this`, holding the elementwise
    inequality result — exactly the logical negation of what `eq()` would return for
    the same arguments.

## Examples

Example 1. Compare every value against a scalar.

```cpp
using namespace pluss::table;

auto scores = Slate(pluss::make_series({72.0, 85.0, 91.0}));
auto out = scores.ne(85.0);
std::cout << out;
```

### Output

```text
0  true
1  false
2  true

ptype: Series
```

The exact negation of [`scores.eq(85.0)`](eq.md)'s `false, true, false`.

Example 2. Compare two slates positionally.

```cpp
auto targets = Slate(pluss::make_series({72.0, 80.0, 95.0}));
auto out_pair = scores.ne(targets);
std::cout << out_pair;
```

### Output

```text
0  false
1  true
2  true

ptype: Series
```

Example 3. String inequality on a `Note` slate.

```cpp
auto names = Slate(pluss::make_note({"Ada", "Bola", "Chen"}));
auto out_names = names.ne("Bola");
std::cout << out_names;
```

### Output

```text
0  true
1  false
2  true

ptype: Series
```

## Notes

- See [`eq()`](eq.md) for the family-wide mechanics: positional matching, the
  length-mismatch throw, and the string-overload's stringify-then-compare behavior
  on non-`Note` slates.

## See Also

- [`Slate::eq`](eq.md): the complementary equality check, and the canonical
  reference for this family's shared mechanics.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ndistinct.html">
    <span class="label">Previous</span>
    <span class="title">ndistinct</span>
  </a>
  <a class="next-link" href="normalize.html">
    <span class="label">Next</span>
    <span class="title">normalize</span>
  </a>
</div>
