<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 242181ddd853 -->

# DataFrame::transpose

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::transpose(bool render = false) const -> DataFrameView
```

## What It Does

Flips rows and features: each original **case** becomes a new **feature** (named
after that case's original label), and each original **feature name** becomes a new
**case label**. Every value in the result is converted to its text representation —
transposing mixes together whatever types were in each row (numbers, strings, and so
on), and a single output column can no longer be a single-typed `Series` once it holds
values that used to live in different feature columns of different types. This makes
`transpose()` best suited for quick visual inspection of a small, wide-vs-tall
swap — not as a step in a numeric pipeline, since every value becomes text and a
missing value becomes the literal string `"NULL"`.

## Parameters

**render** (`bool`, default `false`)

Display toggle for the returned `DataFrameView`, identical in behavior to
[`head()`](head.md)'s `render` parameter.

## Returns

`DataFrameView`
    A new frame where the original feature names are the case labels, and the
    original case labels are the feature names, with every cell converted to text.

## Examples

Example 1. Transpose a small numeric frame.

```cpp
using namespace pluss::table;

auto orders_small = DataFrame({
    {"units",      Slate(pluss::make_series({2, 5, 1}))},
    {"unit_price", Slate(pluss::make_series({1200.0, 45.5, std::numeric_limits<double>::quiet_NaN()}))},
});

auto flipped = orders_small.transpose();
std::cout << flipped;
```

### Output

```text
=============================
           | 0     1     2   
-----------------------------
units      | 2     5     1   
unit_price | 1200  45.5  NULL
-----------------------------
[2 cs x 3 feat]
```

The original two features (`units`, `unit_price`) are now the row labels; the original
three cases (default labels `0`, `1`, `2`) are now the feature names. `unit_price`'s
missing third value became the literal text `"NULL"` — a text cell reading the word
"NULL", not a typed missing value — because the result is text throughout. Note also
that `1200.0` prints as `1200`: whole-valued floats are rendered without a trailing
`.0` in the transposed text form.

## Notes

- Every cell in the result is text, regardless of the original column's type — a
  transposed frame is not meant for further numeric computation, only inspection.
- A missing source cell becomes the literal text `"NULL"` (the word, as a text cell),
  not a typed missing value — this is different from every other reshaping method in
  this reference, which preserve an actual missing cell instead of a text
  representation of one.
- The new feature names come from the original case labels; if those labels would
  collide as feature names (for example, duplicate case labels), they are
  disambiguated automatically.
- `transpose()` composes with `render` exactly like `head()` and `tail()` do.

## See Also

- [`DataFrame::stack`](stack.md) / [`DataFrame::unstack`](unstack.md): reshape between
  wide and long form while preserving each feature's original dtype, unlike
  `transpose()`.
- [`DataFrame::to_string`](to_string.md): a plain-text rendering of the frame without
  swapping rows and features.
- [`DataFrame::head`](head.md): another `DataFrameView`-returning method sharing the
  same `render` toggle.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="transform.html">
    <span class="label">Previous</span>
    <span class="title">transform</span>
  </a>
  <a class="next-link" href="truediv.html">
    <span class="label">Next</span>
    <span class="title">truediv</span>
  </a>
</div>
