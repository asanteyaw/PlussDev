<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d20d5b3f2eca -->

# DataFrame::pow

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::pow(double other) const -> DataFrame
DataFrame::pow(const DataFrame& other) const -> DataFrame
DataFrame::pow(const DataFrame& other, const AlignOptions& options) const -> DataFrame
DataFrame::pow(const Thread& other, Axis axis = Axis::Feature) const -> DataFrame
DataFrame::pow(const Thread& other, Axis axis, const AlignOptions& options) const -> DataFrame
DataFrame::pow(const Thread& other, const AlignOptions& options, Axis axis = Axis::Feature) const -> DataFrame
```

## What It Does

The named-method form of elementwise exponentiation. `pow()` shares the exact same
overload shape and alignment rules as [`add()`](add.md) and the rest of the arithmetic
family — see `add()`'s "What It Does" for the full explanation of the scalar/frame/
Thread operand shapes and the `axis`/`options` parameters.

## Parameters

**other**

The right-hand operand (the exponent): `double` (broadcast scalar), `const
DataFrame&` (aligned frame-to-frame), or `const Thread&` (broadcast per `axis`).

**axis** (`Axis`, default `Axis::Feature`, Thread overloads only)

Same meaning as [`add()`](add.md)'s `axis`: `Axis::Feature` (default) supplies one
value per feature; `Axis::Case` supplies one value per case.

**options** (`AlignOptions`, optional)

Same meaning as [`add()`](add.md)'s `options`.

## Returns

`DataFrame`
    A new frame, same shape as the source, with every numeric feature raised to the
    power of `other`. Non-numeric features are left unchanged.

## Examples

Example 1. Raise every numeric feature to a scalar power.

```cpp
using namespace pluss::table;

auto values = DataFrame({
    {"x", pluss::make_series({2.0, 3.0, 4.0})},
}, pluss::make_note({"a", "b", "c"}));

auto out = values.pow(2.0);
std::cout << out;
```

### Output

```text
===========
     | x   
-----------
a    | 4   
b    | 9   
c    | 16  
-----------
[3 cs x 1 feat]
```

## Notes

- Non-numeric features are always left unchanged.
- Unlike the other arithmetic methods in this family, `pow()` always evaluates
  feature-by-feature rather than as a single batched matrix operation internally —
  this is an implementation detail with no observable effect on the result.
- Shares its full overload shape, alignment semantics, and `AlignOptions` behavior with
  [`add()`](add.md), [`sub()`](sub.md), [`mul()`](mul.md), [`div()`](div.md),
  [`floordiv()`](floordiv.md), and [`mod()`](mod.md).

## See Also

- [`DataFrame::add`](add.md): the full explanation of the shared operand shapes and
  `axis`/`options` parameters this family uses.
- [`DataFrame::rpow`](rpow.md): the reflected form, computing `other ^ frame`.
- [`DataFrame::mul`](mul.md): repeated addition's multiplicative cousin, for scaling
  rather than exponentiating.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="plot3d.html">
    <span class="label">Previous</span>
    <span class="title">plot3d</span>
  </a>
  <a class="next-link" href="prep_sql.html">
    <span class="label">Next</span>
    <span class="title">prep_sql</span>
  </a>
</div>
