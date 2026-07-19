<!-- pluss-hand-authored -->

# DataFrame::ror

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::ror(const DataFrame& other) const -> DataFrame
DataFrame::ror(const Thread& other) const -> DataFrame
```

## What It Does

The reflected form of [`lor()`](lor.md). Since logical OR is commutative,
`frame.ror(other)` is indistinguishable from `frame.lor(other)` in every observable
respect — same cell values, and (for the `DataFrame` overload) the same case labels
and title, both drawn from `frame` itself, exactly as [`lor()`](lor.md) documents. See
[`rand()`](rand.md) for the full explanation, including a note on a previous revision
where the `DataFrame` overload's case labels came from `other` instead — that has
since been fixed, and `ror()` shares the fix.

## Parameters

**other**

`const DataFrame&` or `const Thread&` — identical shape requirements to
[`land()`](land.md).

## Returns

`DataFrame`
    A new frame holding the logical OR of each cell against the corresponding `other`
    value, with case labels and title drawn from the frame `ror()` was called on —
    identical in every respect to what [`lor()`](lor.md) would produce for the same
    arguments.

## Examples

Example 1. `ror()` and `lor()` produce identical results, values and case labels both.

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

auto out = left.ror(right);
std::cout << out;
```

### Output

```text
=================
     | a     b   
-----------------
p    | true  true
q    | true  true
r    | true  true
-----------------
[3 cs x 2 feat]
```

The values match [`left.lor(right)`](lor.md) exactly, and the case labels are `left`'s
own (`p, q, r`) — `right`'s labels (`x, y, z`) never appear in the result.

## Notes

- `ror()` and [`lor()`](lor.md) are fully interchangeable for `DataFrame` arguments —
  same values, same case labels, same title. The reflected name exists purely for
  symmetry with the rest of the reflected method family, not because it produces a
  different result here.
- Requires a genuine `Bool` scalar type on every participating `Series`, same as
  [`land()`](land.md).

## See Also

- [`DataFrame::lor`](lor.md): the forward form.
- [`DataFrame::rand`](rand.md): the fuller explanation of this reflected-family
  behavior, including a note on a previous case-label quirk that has since been fixed.
- [`DataFrame::rxor`](rxor.md): the XOR reflected form, sharing this same behavior.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rmul.html">
    <span class="label">Previous</span>
    <span class="title">rmul</span>
  </a>
  <a class="next-link" href="round.html">
    <span class="label">Next</span>
    <span class="title">round</span>
  </a>
</div>
