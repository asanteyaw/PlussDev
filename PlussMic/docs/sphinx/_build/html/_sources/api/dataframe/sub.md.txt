<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 799308ff4bc4 -->

# DataFrame::sub

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::sub(double other) const -> DataFrame
DataFrame::sub(const DataFrame& other) const -> DataFrame
DataFrame::sub(const DataFrame& other, const AlignOptions& options) const -> DataFrame
DataFrame::sub(const Slate& other, Axis axis = Axis::Feature) const -> DataFrame
DataFrame::sub(const Slate& other, Axis axis, const AlignOptions& options) const -> DataFrame
DataFrame::sub(const Slate& other, const AlignOptions& options, Axis axis = Axis::Feature) const -> DataFrame
```

## What It Does

The named-method form of elementwise subtraction. `sub()` shares the exact same
overload shape and alignment rules as [`add()`](add.md), [`mul()`](mul.md),
[`div()`](div.md), and the rest of the arithmetic family — see `add()`'s
"What It Does" for the full explanation of the scalar/frame/Slate operand shapes and
the `axis`/`options` parameters. Unlike addition, subtraction is not commutative, so
operand order matters: `a.sub(b)` computes `a - b`, not `b - a`. Use
[`rsub()`](rsub.md) when you specifically want `other - a` with `a`'s shape driving
the result.

## Parameters

**other**

The right-hand operand, subtracted from the current frame: `double` (broadcast
scalar), `const DataFrame&` (aligned frame-to-frame), or `const Slate&` (broadcast
per `axis`).

**axis** (`Axis`, default `Axis::Feature`, Slate overloads only)

Same meaning as [`add()`](add.md)'s `axis`: `Axis::Feature` (default) supplies one
value per feature; `Axis::Case` supplies one value per case.

**options** (`AlignOptions`, optional)

Same meaning as [`add()`](add.md)'s `options`: controls alignment and missing-value
handling for the `DataFrame`/`Slate` overloads.

## Returns

`DataFrame`
    A new frame, same shape as the source, with every numeric feature reduced by
    `other`. Non-numeric features are left unchanged.

## Examples

Example 1. Subtract a scalar from every numeric feature.

```cpp
using namespace pluss::table;

auto prices = DataFrame({
    {"units", pluss::make_series({2.0, 5.0, 1.0})},
    {"price", pluss::make_series({10.0, 20.0, 30.0})},
}, pluss::make_note({"a", "b", "c"}));

auto out = prices.sub(1.0);
std::cout << out;
```

### Output

```text
===================
     | units  price
-------------------
a    | 1      9    
b    | 4      19   
c    | 0      29   
-------------------
[3 cs x 2 feat]
```

Example 2. Subtract another aligned frame, feature-by-feature.

```cpp
auto fees = DataFrame({
    {"units", pluss::make_series({1.0, 1.0, 0.0})},
    {"price", pluss::make_series({2.0, 2.0, 2.0})},
}, pluss::make_note({"a", "b", "c"}));

auto out = prices.sub(fees);
std::cout << out;
```

### Output

```text
===================
     | units  price
-------------------
a    | 1      8    
b    | 4      18   
c    | 1      28   
-------------------
[3 cs x 2 feat]
```

Example 3. Subtract a `Slate` with one value per case (`Axis::Case`) — a per-row
adjustment applied across every feature in that row.

```cpp
auto per_case_discount = Slate(pluss::make_series({1.0, 2.0, 3.0}));
auto out = prices.sub(per_case_discount, Axis::Case);
std::cout << out;
```

### Output

```text
===================
     | units  price
-------------------
a    | 1      9    
b    | 3      18   
c    | -2     27   
-------------------
[3 cs x 2 feat]
```

## Notes

- Non-numeric features are always left unchanged.
- Subtraction is not commutative: `a.sub(b)` computes `a - b`. For `b - a` with `a`
  driving the result shape, use [`rsub()`](rsub.md).
- Shares its full overload shape, alignment semantics, and `AlignOptions` behavior with
  [`add()`](add.md), [`mul()`](mul.md), [`div()`](div.md), [`truediv()`](truediv.md),
  [`floordiv()`](floordiv.md), [`mod()`](mod.md), and [`pow()`](pow.md).

## See Also

- [`DataFrame::add`](add.md): the full explanation of the shared operand shapes and
  `axis`/`options` parameters this family uses.
- [`DataFrame::rsub`](rsub.md): the reflected form, computing `other - frame`.
- [`DataFrame::compare`](compare.md): cell-by-cell comparison instead of arithmetic
  combination.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="std.html">
    <span class="label">Previous</span>
    <span class="title">std</span>
  </a>
  <a class="next-link" href="sum.html">
    <span class="label">Next</span>
    <span class="title">sum</span>
  </a>
</div>
