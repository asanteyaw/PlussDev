<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8091708aee10 -->

# DataFrame::truediv

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::truediv(double other) const -> DataFrame
DataFrame::truediv(const DataFrame& other) const -> DataFrame
DataFrame::truediv(const DataFrame& other, const AlignOptions& options) const -> DataFrame
DataFrame::truediv(const Slate& other, Axis axis = Axis::Feature) const -> DataFrame
DataFrame::truediv(const Slate& other, Axis axis, const AlignOptions& options) const -> DataFrame
DataFrame::truediv(const Slate& other, const AlignOptions& options, Axis axis = Axis::Feature) const -> DataFrame
```

## What It Does

The named-method form of elementwise division — and, verified against the underlying
numeric kernel, **the same operation as [`div()`](div.md)**. `truediv()` shares the
exact same overload shape and alignment rules as [`add()`](add.md) and the rest of the
arithmetic family — see `add()`'s "What It Does" for the full explanation of the
scalar/frame/Slate operand shapes and the `axis`/`options` parameters.

```{important} truediv() and div() are the same operation
Both names route to the identical division kernel and produce identical results for
every input, including integer-typed features — neither one truncates or floors the
way [`floordiv()`](floordiv.md) does. `truediv()` exists as the more explicit name for
readers who want it clear this is ordinary (non-floor) division; `div()` is the
shorter alternative. There is no behavioral difference — pick whichever name reads
better at the call site.
```

## Parameters

**other**

The right-hand operand, dividing the current frame: `double` (broadcast scalar),
`const DataFrame&` (aligned frame-to-frame), or `const Slate&` (broadcast per
`axis`).

**axis** (`Axis`, default `Axis::Feature`, Slate overloads only)

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

auto out = counts.truediv(2.0);
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

Example 2. `div()` on the same data produces the identical result.

```cpp
auto out_div = counts.div(2.0);
std::cout << out_div;
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
- `truediv()` and `div()` are verified to be identical operations — see the note
  above.
- For integer-style division that rounds down to the nearest whole number instead, use
  [`floordiv()`](floordiv.md).
- Shares its full overload shape, alignment semantics, and `AlignOptions` behavior with
  [`add()`](add.md), [`sub()`](sub.md), [`mul()`](mul.md), [`floordiv()`](floordiv.md),
  [`mod()`](mod.md), and [`pow()`](pow.md).

## See Also

- [`DataFrame::add`](add.md): the full explanation of the shared operand shapes and
  `axis`/`options` parameters this family uses.
- [`DataFrame::div`](div.md): the verified-identical alternate name for this same
  operation.
- [`DataFrame::floordiv`](floordiv.md): the distinct floor-division operation.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="transpose.html">
    <span class="label">Previous</span>
    <span class="title">transpose</span>
  </a>
  <a class="next-link" href="ts.html">
    <span class="label">Next</span>
    <span class="title">ts</span>
  </a>
</div>
