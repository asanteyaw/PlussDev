<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 732fbb8a27f0 -->

# DataFrame::matmul

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::matmul(const DataFrame& other) const -> DataFrame
```

## What It Does

Treats both frames as plain numeric matrices — cases as rows, features as columns —
and computes the standard matrix product between them: `this` (shape `ncases() x
nfeatures()`) times `other` (shape `other.ncases() x other.nfeatures()`). This means
`other`'s **case count** must equal this frame's **feature count**, exactly like
ordinary matrix multiplication requires the left matrix's column count to match the
right matrix's row count.

## Parameters

**other** (`const DataFrame&`)

The right-hand matrix. Every feature on both frames must be numeric; `other.ncases()`
must equal `this->nfeatures()`.

## Returns

`DataFrame`
    A new frame of shape `(this->ncases(), other.nfeatures())`: `other`'s feature
    names, and this frame's case labels.

## Examples

Example 1. Multiply a 2x2 numeric frame by another 2x2 numeric frame.

```cpp
using namespace pluss::table;

auto a = DataFrame({
    {"x", Slate(pluss::make_series({1.0, 3.0}))},
    {"y", Slate(pluss::make_series({2.0, 4.0}))},
});
std::cout << a;
```

### Output (a)

```text
=================
     | x     y   
-----------------
0    | 1     2   
1    | 3     4   
-----------------
[2 cs x 2 feat]
```

```cpp
auto b = DataFrame({
    {"p", Slate(pluss::make_series({5.0, 6.0}))},
    {"q", Slate(pluss::make_series({7.0, 8.0}))},
});
std::cout << b;
```

### Output (b)

```text
=================
     | p     q   
-----------------
0    | 5     7   
1    | 6     8   
-----------------
[2 cs x 2 feat]
```

```cpp
auto product = a.matmul(b);
std::cout << product;
```

### Output

```text
=================
     | p     q   
-----------------
0    | 17    23  
1    | 39    53  
-----------------
[2 cs x 2 feat]
```

`a` as a matrix is `[[1, 2], [3, 4]]` (case `0`'s row is `[1, 2]`, case `1`'s row is
`[3, 4]`); `b` is `[[5, 7], [6, 8]]`. The result's case `0`, feature `p` is
`1*5 + 2*6 = 17`; case `0`, feature `q` is `1*7 + 2*8 = 23`; and so on for case `1`.
The result carries `b`'s feature names (`p`, `q`) and `a`'s case labels (`0`, `1`).

## Notes

- `other.ncases()` must equal `this->nfeatures()`; a mismatch throws
  `"DataFrame::matmul: left feature count must equal right case count"`.
- Neither operand may contain any missing values — `matmul()` throws
  `"DataFrame::matmul: null values are not supported"` if either frame has a `NULL`
  cell anywhere.
- Both operands must be entirely numeric; a `Note`, `Timeline`, or `TimeIndex` feature
  on either side is not a supported combination for matrix multiplication.
- The result's feature names always come from `other`, and its case labels always
  come from `this` — the same convention as ordinary matrix multiplication, where the
  result's row labels come from the left operand and its column labels come from the
  right.

## See Also

- [`DataFrame::matrix`](matrix.md): the lower-level accessor for the raw numeric
  matrix behind a frame, without the multiplication.
- [`DataFrame::mul`](mul.md): elementwise (not matrix) multiplication between two
  frames or a frame and a scalar.
- [`DataFrame::corr`](corr.md): another numeric-matrix-wide computation, for
  pairwise correlation instead of a matrix product.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="match.html">
    <span class="label">Previous</span>
    <span class="title">match</span>
  </a>
  <a class="next-link" href="matrix.html">
    <span class="label">Next</span>
    <span class="title">matrix</span>
  </a>
</div>
