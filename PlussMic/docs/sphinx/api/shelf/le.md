<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: eb5165583aa0 -->

# Shelf::le

[Back to Shelf API Reference](../../shelf_api_reference)

## Signatures

```cpp
Shelf Shelf::le(double other) const
Shelf Shelf::le(std::string_view other) const
Shelf Shelf::le(const char* other) const
Shelf Shelf::le(const Shelf& other) const
```

## What It Does

Elementwise "less than or equal", following [`eq()`](eq.md)'s shared overload shape
and positional matching. Like [`lt()`](lt.md), the string overload stringifies
numeric values before comparing — see [`lt()`](lt.md) for why that can disagree with
numeric ordering.

## Parameters

**other** (`double`, `std::string_view`, or `const Shelf&`)

Same shape as [`eq()`](eq.md).

## Returns

`Shelf`
    A new `Bool`-typed shelf, same length as `*this`, holding the elementwise
    "less than or equal" result.

## Examples

Example 1. Compare every value against a scalar.

```cpp
using namespace pluss::table;

auto scores = Shelf(pluss::make_series({72.0, 85.0, 91.0}));
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

Example 2. Compare two shelves positionally.

```cpp
auto targets = Shelf(pluss::make_series({72.0, 80.0, 95.0}));
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

Example 3. String comparison on a `Note` shelf.

```cpp
auto names = Shelf(pluss::make_note({"Ada", "Bola", "Chen"}));
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

- [`Shelf::lt`](lt.md): the strict form.
- [`Shelf::ge`](ge.md): the opposite direction's inclusive form.

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
