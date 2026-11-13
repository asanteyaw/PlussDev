<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f1868f5719e8 -->

# DataFrame::lor

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame DataFrame::lor(const DataFrame& other) const
DataFrame DataFrame::lor(const Slate& other) const
```

## What It Does

Elementwise logical OR — the same overload shape, `Bool`-only requirement, and
positional (not case-label) matching as [`land()`](land.md). See `land()`'s "What It
Does" for the full explanation of the two overloads and the constraints on `other`.

## Parameters

**other**

`const DataFrame&` (same feature names in order, same case count, matched
positionally) or `const Slate&` (one boolean slate applied identically to every
feature). See [`land()`](land.md) for the full breakdown.

## Returns

`DataFrame`
    A new frame, same shape as the source, with every feature holding the logical OR
    of the source cell and the corresponding `other` value.

## Examples

Example 1. OR two boolean frames with identical feature names, case-by-case.

```cpp
using namespace pluss::table;

auto left = DataFrame({
    {"a", pluss::make_series({true, false, true}, pluss::ScalarType::Bool)},
    {"b", pluss::make_series({true, true, false}, pluss::ScalarType::Bool)},
}, pluss::make_note({"p", "q", "r"}));

auto right = DataFrame({
    {"a", pluss::make_series({false, false, false}, pluss::ScalarType::Bool)},
    {"b", pluss::make_series({false, false, true}, pluss::ScalarType::Bool)},
}, pluss::make_note({"x", "y", "z"}));

auto out = left.lor(right);
std::cout << out;
```

### Output

```text
===================
     | a      b    
-------------------
p    | true   true 
q    | false  true 
r    | true   true 
-------------------
[3 cs x 2 feat]
```

Position `1` (`q`) is the only cell where OR reports `false` for feature `a` — that's
the one position where **both** operands (`left`'s `false` and `right`'s `false`) are
false at once. Every other cell has at least one `true` operand on either side, so OR
reports `true`.

Example 2. OR a frame against a single shared `Slate`, applied identically to every
feature.

```cpp
auto mask = Slate(pluss::make_series({false, true, false}, pluss::ScalarType::Bool));
auto out_slate = left.lor(mask);
std::cout << out_slate;
```

### Output

```text
===================
     | a     b    
-------------------
p    | true  true 
q    | true  true 
r    | true  false
-------------------
[3 cs x 2 feat]
```

The same three-value `mask` is combined against **both** `a` and `b` at matching
positions — there is no way to supply a different mask per feature in one call,
unlike [`add()`](add.md)'s `Axis::Feature` `Slate` overload.

## Notes

- Requires a genuine `Bool` scalar type on every participating `Series` — see
  [`land()`](land.md)'s note on casting first with
  [`astype(..., pluss::ScalarType::Bool)`](astype.md).
- `lor(const DataFrame&)` matches positionally by feature name and case count only —
  it does not align by case label. See [`land()`](land.md) for the full explanation.
- The reflected form [`ror()`](ror.md) computes the same cell values (OR is
  commutative) and the same case labels and title too — `ror()` and `lor()` are fully
  interchangeable. See [`rand()`](rand.md) for the fuller explanation, including a
  note on an earlier revision where this wasn't the case.

## See Also

- [`DataFrame::land`](land.md): the AND counterpart, and the full explanation of this
  family's shared overload shape.
- [`DataFrame::lxor`](lxor.md): the XOR counterpart.
- [`DataFrame::ror`](ror.md): the reflected form.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="log.html">
    <span class="label">Previous</span>
    <span class="title">log</span>
  </a>
  <a class="next-link" href="lt.html">
    <span class="label">Next</span>
    <span class="title">lt</span>
  </a>
</div>
