<!-- pluss-hand-authored -->

# DataFrame::lxor

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::lxor(const DataFrame& other) const -> DataFrame
DataFrame::lxor(const Thread& other) const -> DataFrame
```

## What It Does

Elementwise logical XOR — the same overload shape, `Bool`-only requirement, and
positional (not case-label) matching as [`land()`](land.md). See `land()`'s "What It
Does" for the full explanation of the two overloads and the constraints on `other`.
Unlike [`land()`](land.md)/[`lor()`](lor.md), XOR reports `true` exactly where the two
operands *disagree* — a useful "exactly one of these flags is set" check.

## Parameters

**other**

`const DataFrame&` (same feature names in order, same case count, matched
positionally) or `const Thread&` (one boolean thread applied identically to every
feature). See [`land()`](land.md) for the full breakdown.

## Returns

`DataFrame`
    A new frame, same shape as the source, with every feature holding the logical XOR
    of the source cell and the corresponding `other` value.

## Examples

Example 1. XOR two boolean frames with identical feature names, case-by-case.

```cpp
using namespace pluss::table;

auto left = DataFrame({
    {"a", pluss::make_series({true, false, true}, pluss::ScalarType::Bool)},
    {"b", pluss::make_series({true, true, false}, pluss::ScalarType::Bool)},
}, pluss::make_note({"p", "q", "r"}));

auto right = DataFrame({
    {"a", pluss::make_series({true, true, false}, pluss::ScalarType::Bool)},
    {"b", pluss::make_series({false, true, true}, pluss::ScalarType::Bool)},
}, pluss::make_note({"x", "y", "z"}));

auto out = left.lxor(right);
std::cout << out;
```

### Output

```text
===================
     | a      b    
-------------------
p    | false  true 
q    | true   false
r    | true   true 
-------------------
[3 cs x 2 feat]
```

Position `0` (`left`'s `a=true` against `right`'s `a=true`) agrees, so XOR reports
`false`; position `1` (`left`'s `a=false` against `right`'s `a=true`) disagrees, so XOR
reports `true` — and so on for `b`.

## Notes

- Requires a genuine `Bool` scalar type on every participating `Series` — see
  [`land()`](land.md)'s note on casting first with
  [`astype(..., pluss::ScalarType::Bool)`](astype.md).
- `lxor(const DataFrame&)` matches positionally by feature name and case count only —
  it does not align by case label. See [`land()`](land.md) for the full explanation.
- The reflected form [`rxor()`](rxor.md) computes the same cell values (XOR is
  commutative) and the same case labels and title too — `rxor()` and `lxor()` are
  fully interchangeable. See [`rand()`](rand.md) for the fuller explanation, including
  a note on an earlier revision where this wasn't the case.

## See Also

- [`DataFrame::land`](land.md): the AND counterpart, and the full explanation of this
  family's shared overload shape.
- [`DataFrame::lor`](lor.md): the OR counterpart.
- [`DataFrame::rxor`](rxor.md): the reflected form.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lt.html">
    <span class="label">Previous</span>
    <span class="title">lt</span>
  </a>
  <a class="next-link" href="map.html">
    <span class="label">Next</span>
    <span class="title">map</span>
  </a>
</div>
