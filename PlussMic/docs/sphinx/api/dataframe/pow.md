<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2da1aa1e4f13 -->

# DataFrame::pow

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame DataFrame::pow(double other) const
DataFrame DataFrame::pow(const DataFrame& other) const
DataFrame DataFrame::pow(const DataFrame& other, const AlignOptions& options) const
DataFrame DataFrame::pow(const Slate& other, Axis axis = Axis::Feature) const
DataFrame DataFrame::pow(const Slate& other, Axis axis, const AlignOptions& options) const
DataFrame DataFrame::pow(const Slate& other, const AlignOptions& options, Axis axis = Axis::Feature) const
```

## What It Does

The named-method form of elementwise exponentiation: raises every numeric feature to
the power of `other`. `pow()` shares the exact same six-overload shape and alignment
rules as [`add()`](add.md) and the rest of the arithmetic family
(`add`, [`sub`](sub.md), [`mul`](mul.md), [`div`](div.md), [`truediv`](truediv.md),
[`floordiv`](floordiv.md), [`mod`](mod.md)) — only the operation itself differs. See
`add()`'s "What It Does" for the general explanation of why each shape exists; this
page demonstrates every one of them with `pow()`'s own numbers.

## Parameters

**other**

The right-hand operand (the exponent), in one of three shapes:

- `double`: one scalar exponent, applied to every numeric feature.
- `const DataFrame&`: another frame supplying one exponent per cell, after the two
  frames are aligned.
- `const Slate&`: one column of exponents, broadcast according to `axis`.

**axis** (`Axis`, default `Axis::Feature`, Slate overloads only)

- `Axis::Feature` (the default): `other` supplies **one exponent per feature** — its
  length must match `nfeatures()`. Feature `i`'s exponent is applied to every case in
  column `i`.
- `Axis::Case`: `other` supplies **one exponent per case** — its length must match
  `ncases()`. Case `i`'s exponent is applied across every feature in row `i`.

**options** (`AlignOptions`, optional)

Controls how the `DataFrame` or `Slate` right-hand operand is aligned to the current
frame before the exponentiation runs (by position, case label, or feature name), and
how missing values are filled during alignment. Defaults to strict positional
alignment (`AlignBy::Pos`).

## Returns

`DataFrame`
    A new frame, same shape as the source, with every numeric feature raised to the
    power of `other`. Non-numeric features are left unchanged.

## Examples

Example 1. Raise every numeric feature to a scalar power.

```cpp
using namespace pluss::table;

auto values = DataFrame({
    {"base",  pluss::make_series({2.0, 3.0, 4.0})},
    {"scale", pluss::make_series({10.0, 20.0, 30.0})},
}, pluss::make_note({"a", "b", "c"}));

std::cout << values;
```

### Output (before)

```text
===================
      | base  scale
-------------------
a     | 2     10   
b     | 3     20   
c     | 4     30   
-------------------
[3 cs x 2 feat]
```

```cpp
auto out = values.pow(2.0);
std::cout << out;
```

### Output (after)

```text
===================
      | base  scale
-------------------
a     | 4     100  
b     | 9     400  
c     | 16    900  
-------------------
[3 cs x 2 feat]
```

Example 2. Raise each feature to its own exponent via a `Slate` with one value per
feature (`Axis::Feature`, the default) — `base` gets `^2`, `scale` gets `^3`.

```cpp
auto per_feature_exp = Slate(pluss::make_series({2.0, 3.0}));
auto out = values.pow(per_feature_exp);
std::cout << out;
```

### Output

```text
===================
      | base  scale
-------------------
a     | 4     1000 
b     | 9     8000 
c     | 16    27000
-------------------
[3 cs x 2 feat]
```

Example 3. Raise each *case* to its own exponent instead (`Axis::Case`) — row `a`
gets `^1`, row `b` gets `^2`, row `c` gets `^3`, applied across every feature in that
row.

```cpp
auto per_case_exp = Slate(pluss::make_series({1.0, 2.0, 3.0}));
auto out = values.pow(per_case_exp, Axis::Case);
std::cout << out;
```

### Output

```text
===================
      | base  scale
-------------------
a     | 2     10   
b     | 9     400  
c     | 64    27000
-------------------
[3 cs x 2 feat]
```

Example 4. Raise by another `DataFrame` using the default (`const DataFrame&`)
overload — a plain positional pairing that ignores both frames' case labels.

```cpp
auto exponents = DataFrame({
    {"col1", Slate(pluss::make_series({3.0, 1.0, 2.0}))},
    {"col2", Slate(pluss::make_series({1.0, 2.0, 1.0}))},
}, Slate(pluss::make_note({"x", "y", "z"})));
std::cout << exponents;

auto out = values.pow(exponents);
std::cout << out;
```

### Output (exponents)

```text
==================
      | col1  col2
------------------
x     | 3     1   
y     | 1     2   
z     | 2     1   
------------------
[3 cs x 2 feat]
```

### Output (values.pow(exponents))

```text
===================
      | base  scale
-------------------
a     | 8     10   
b     | 3     400  
c     | 16    30   
-------------------
[3 cs x 2 feat]
```

`exponents`'s case labels (`x`, `y`, `z`) share nothing with `values`'s (`a`, `b`,
`c`), and the default overload doesn't care — it pairs row 0 with row 0 (`base` = `2`
raised to `col1`'s row-0 exponent `3`, giving `8`), row 1 with row 1, and so on, purely
by physical position.

Example 5. The exact same `exponents` values as Example 4, but with the case labels
shuffled to `c`, `a`, `b` instead of `x`, `y`, `z` — now using
`AlignOptions{.align = AlignBy::Caselbl}` to pair rows by case label instead of
physical position.

```cpp
auto exponents_by_label = DataFrame({
    {"col1", Slate(pluss::make_series({3.0, 1.0, 2.0}))},
    {"col2", Slate(pluss::make_series({1.0, 2.0, 1.0}))},
}, Slate(pluss::make_note({"c", "a", "b"})));
std::cout << exponents_by_label;

auto out = values.pow(exponents_by_label, AlignOptions{.align = AlignBy::Caselbl});
std::cout << out;
```

### Output (exponents_by_label)

```text
==================
      | col1  col2
------------------
c     | 3     1   
a     | 1     2   
b     | 2     1   
------------------
[3 cs x 2 feat]
```

### Output (values.pow(exponents_by_label, Caselbl))

```text
===================
      | base  scale
-------------------
a     | 2     100  
b     | 9     20   
c     | 64    30   
-------------------
[3 cs x 2 feat]
```

With `AlignBy::Caselbl`, case `"a"` in `values` is matched to case `"a"` in
`exponents_by_label` — which sits in the *second* physical row here (`col1 = 1,
col2 = 2`) — giving `base_a = 2^1 = 2` and `scale_a = 10^2 = 100`. Compare this to
Example 4's positional pairing, which matched `values`'s row 0 (`"a"`) to
`exponents`'s row 0 (`"x"`, `col1 = 3`) and got a different answer, `base_a = 2^3 =
8`. Same underlying numbers, different pairing rule, different result — exactly the
contrast [`add()`](add.md)'s own Examples 4 and 5 make with addition instead of
exponentiation.

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
