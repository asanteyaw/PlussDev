<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 417519f4eff3 -->

# DataFrame::rand

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame DataFrame::rand(const DataFrame& other) const
DataFrame DataFrame::rand(const Slate& other) const
```

## What It Does

The reflected form of [`land()`](land.md). Since logical AND is commutative,
`frame.rand(other)` is indistinguishable from `frame.land(other)` in every observable
respect — same cell values, and (for the `DataFrame` overload) the same case labels
and title, both drawn from `frame` itself, exactly as [`land()`](land.md) documents.

```{note} Previously, the DataFrame overload inherited case labels from other
An earlier revision of `rand(const DataFrame&)` computed the reflection as
`other.land(*this)`, which meant the result's case labels and title came from
`other` instead of from the frame the method was called on — a real, verified quirk
that was documented here as a caution. That has since been fixed: `rand()` now zips
`*this` and `other` directly (with `Slate::rand()` handling the per-cell operand
order internally), so the result's structure always comes from `*this`, matching
[`land()`](land.md) and every other method in this family. If you're working against
an older build where this fix isn't present, verify which behavior you're actually
seeing before relying on either.
```

## Parameters

**other**

`const DataFrame&` or `const Slate&` — identical shape requirements to
[`land()`](land.md): same feature names in order and same case count for the
`DataFrame` overload; one boolean slate matching `ncases()` for the `Slate` overload.

## Returns

`DataFrame`
    A new frame holding the logical AND of each cell against the corresponding `other`
    value, with case labels and title drawn from the frame `rand()` was called on —
    identical in every respect to what [`land()`](land.md) would produce for the same
    arguments.

## Examples

Example 1. `rand()` and `land()` produce identical results, values and case labels
both.

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

auto forward = left.land(right);
std::cout << forward;
```

### Output (land)

```text
===================
     | a      b    
-------------------
p    | true   false
q    | false  true 
r    | false  false
-------------------
[3 cs x 2 feat]
```

```cpp
auto reflected = left.rand(right);
std::cout << reflected;
```

### Output (rand)

```text
===================
     | a      b    
-------------------
p    | true   false
q    | false  true 
r    | false  false
-------------------
[3 cs x 2 feat]
```

Both calls produce the exact same table — same values, same case labels (`p, q, r`,
from `left`, the frame both methods were called on), even though `right` uses a
completely different set of case labels (`x, y, z`) that never appear in either
result.

Example 2. The `Slate` overload behaves identically, as it always has.

```cpp
auto mask = Slate(pluss::make_series({true, false, true}, pluss::ScalarType::Bool));
auto out = left.rand(mask);
std::cout << out;
```

### Output

```text
===================
     | a      b    
-------------------
p    | true   true 
q    | false  false
r    | true   false
-------------------
[3 cs x 2 feat]
```

Case labels `p, q, r` come from `left`, exactly as [`land(mask)`](land.md) would
produce.

## Notes

- `rand()` and [`land()`](land.md) are fully interchangeable for `DataFrame` arguments
  — same values, same case labels, same title. The reflected name exists purely for
  symmetry with the rest of the reflected method family ([`ror()`](ror.md),
  [`rxor()`](rxor.md), [`radd()`](radd.md), and so on), not because it produces a
  different result here.
- Requires a genuine `Bool` scalar type on every participating `Series`, same as
  [`land()`](land.md).

## See Also

- [`DataFrame::land`](land.md): the forward form, and the full explanation of the
  logical family's overload shape and `Bool`-only requirement.
- [`DataFrame::ror`](ror.md) / [`DataFrame::rxor`](rxor.md): the OR and XOR reflected
  forms, sharing this same now-fully-interchangeable behavior.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="radd.html">
    <span class="label">Previous</span>
    <span class="title">radd</span>
  </a>
  <a class="next-link" href="rank.html">
    <span class="label">Next</span>
    <span class="title">rank</span>
  </a>
</div>
