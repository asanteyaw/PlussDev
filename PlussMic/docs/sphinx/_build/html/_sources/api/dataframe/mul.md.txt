<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 24800483050a -->

# DataFrame::mul

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::mul(double other) const -> DataFrame
DataFrame::mul(const DataFrame& other) const -> DataFrame
DataFrame::mul(const DataFrame& other, const AlignOptions& options) const -> DataFrame
DataFrame::mul(const Thread& other, Axis axis = Axis::Feature) const -> DataFrame
DataFrame::mul(const Thread& other, Axis axis, const AlignOptions& options) const -> DataFrame
DataFrame::mul(const Thread& other, const AlignOptions& options, Axis axis = Axis::Feature) const -> DataFrame
```

## What It Does

The named-method form of elementwise multiplication. `mul()` shares the exact same
overload shape and alignment rules as [`add()`](add.md) and the rest of the
arithmetic family — see `add()`'s "What It Does" for the full explanation of the
scalar/frame/Thread operand shapes and the `axis`/`options` parameters.

A common use is rescaling a feature (unit conversion, currency conversion, applying a
tax or discount rate) without hand-writing a loop over cells.

## Parameters

**other**

The right-hand operand, multiplied against the current frame: `double` (broadcast
scalar), `const DataFrame&` (aligned frame-to-frame), or `const Thread&` (broadcast
per `axis`).

**axis** (`Axis`, default `Axis::Feature`, Thread overloads only)

Same meaning as [`add()`](add.md)'s `axis`: `Axis::Feature` (default) supplies one
value per feature; `Axis::Case` supplies one value per case.

**options** (`AlignOptions`, optional)

Same meaning as [`add()`](add.md)'s `options`.

## Returns

`DataFrame`
    A new frame, same shape as the source, with every numeric feature multiplied by
    `other`. Non-numeric features are left unchanged.

## Examples

Example 1. Scale every numeric feature by a constant factor.

```cpp
using namespace pluss::table;

auto prices = DataFrame({
    {"units", pluss::make_series({2.0, 5.0, 1.0})},
    {"price", pluss::make_series({10.0, 20.0, 30.0})},
}, pluss::make_note({"a", "b", "c"}));

auto out = prices.mul(2.0);
std::cout << out;
```

### Output

```text
===================
     | units  price
-------------------
a    | 4      20   
b    | 10     40   
c    | 2      60   
-------------------
[3 cs x 2 feat]
```

Example 2. Multiply by a `Thread` with one value per feature (`Axis::Feature`, the
default) — apply a different scale factor to each column.

```cpp
auto per_feature_scale = Thread(pluss::make_series({10.0, 0.9})); // units x10, price x0.9
auto out = prices.mul(per_feature_scale);
std::cout << out;
```

### Output

```text
===================
     | units  price
-------------------
a    | 20     9    
b    | 50     18   
c    | 10     27   
-------------------
[3 cs x 2 feat]
```

## Notes

- Non-numeric features are always left unchanged.
- Multiplication is commutative, so `a.mul(b)` and `b.mul(a)` (or the reflected
  [`rmul()`](rmul.md)) produce the same result for a shared shape.
- Shares its full overload shape, alignment semantics, and `AlignOptions` behavior with
  [`add()`](add.md), [`sub()`](sub.md), [`div()`](div.md), [`truediv()`](truediv.md),
  [`floordiv()`](floordiv.md), [`mod()`](mod.md), and [`pow()`](pow.md).

## See Also

- [`DataFrame::add`](add.md): the full explanation of the shared operand shapes and
  `axis`/`options` parameters this family uses.
- [`DataFrame::div`](div.md) / [`DataFrame::truediv`](truediv.md): the corresponding
  division methods.
- [`DataFrame::assign`](assign.md): compute and attach a scaled feature as a new
  column instead of transforming existing ones in place.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mode.html">
    <span class="label">Previous</span>
    <span class="title">mode</span>
  </a>
  <a class="next-link" href="ncases.html">
    <span class="label">Next</span>
    <span class="title">ncases</span>
  </a>
</div>
