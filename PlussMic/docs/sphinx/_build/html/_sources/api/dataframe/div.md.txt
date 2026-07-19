<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 754bd4a44854 -->

# DataFrame::div

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::div(double other) const -> DataFrame
DataFrame::div(const DataFrame& other) const -> DataFrame
DataFrame::div(const DataFrame& other, const AlignOptions& options) const -> DataFrame
DataFrame::div(const Thread& other, Axis axis = Axis::Feature) const -> DataFrame
DataFrame::div(const Thread& other, Axis axis, const AlignOptions& options) const -> DataFrame
DataFrame::div(const Thread& other, const AlignOptions& options, Axis axis = Axis::Feature) const -> DataFrame
```

## What It Does

The named-method form of elementwise division. `div()` shares the exact same overload
shape and alignment rules as [`add()`](add.md) and the rest of the arithmetic family —
see `add()`'s "What It Does" for the full explanation of the scalar/frame/Thread
operand shapes and the `axis`/`options` parameters.

```{important} div() and truediv() are the same operation
`div()` and [`truediv()`](truediv.md) both perform ordinary floating-point division —
verified against the underlying numeric kernel, they compute identical results for
every input, including integer-typed features. Neither one truncates or floors the
result the way [`floordiv()`](floordiv.md) does. Both names are provided so code
reading like `df.div(2)` and code reading like `df.truediv(2)` (for readers coming
from a background where "true division" is a meaningful distinction from floor
division) are both available — pick whichever reads better at the call site, since
there is no behavioral reason to prefer one over the other.
```

## Parameters

**other**

The right-hand operand, dividing the current frame: `double` (broadcast scalar),
`const DataFrame&` (aligned frame-to-frame), or `const Thread&` (broadcast per
`axis`).

**axis** (`Axis`, default `Axis::Feature`, Thread overloads only)

Same meaning as [`add()`](add.md)'s `axis`: `Axis::Feature` (default) supplies one
value per feature; `Axis::Case` supplies one value per case.

**options** (`AlignOptions`, optional)

Same meaning as [`add()`](add.md)'s `options`.

## Returns

`DataFrame`
    A new frame, same shape as the source, with every numeric feature divided by
    `other`. Non-numeric features are left unchanged.

## Examples

Example 1. Divide every numeric feature by a scalar.

```cpp
using namespace pluss::table;

auto counts = DataFrame({
    {"total", pluss::make_series({7.0, 9.0, 10.0})},
}, pluss::make_note({"a", "b", "c"}));

auto out = counts.div(2.0);
std::cout << out;
```

### Output

```text
============
     | total
------------
a    | 3.5  
b    | 4.5  
c    | 5    
------------
[3 cs x 1 feat]
```

Example 2. `truediv()` on the same data produces the identical result.

```cpp
auto out_truediv = counts.truediv(2.0);
std::cout << out_truediv;
```

### Output

```text
============
     | total
------------
a    | 3.5  
b    | 4.5  
c    | 5    
------------
[3 cs x 1 feat]
```

## Notes

- Non-numeric features are always left unchanged.
- `div()` and `truediv()` are verified to be identical operations — see the note
  above.
- For integer-style division that rounds down to the nearest whole number instead, use
  [`floordiv()`](floordiv.md).
- Shares its full overload shape, alignment semantics, and `AlignOptions` behavior with
  [`add()`](add.md), [`sub()`](sub.md), [`mul()`](mul.md), [`floordiv()`](floordiv.md),
  [`mod()`](mod.md), and [`pow()`](pow.md).

## See Also

- [`DataFrame::add`](add.md): the full explanation of the shared operand shapes and
  `axis`/`options` parameters this family uses.
- [`DataFrame::truediv`](truediv.md): the verified-identical alternate name for this
  same operation.
- [`DataFrame::floordiv`](floordiv.md): the distinct floor-division operation.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="describe.html">
    <span class="label">Previous</span>
    <span class="title">describe</span>
  </a>
  <a class="next-link" href="drop.html">
    <span class="label">Next</span>
    <span class="title">drop</span>
  </a>
</div>
