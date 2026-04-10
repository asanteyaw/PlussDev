# Series API Reference

# Complete Series Method Reference: Signatures + Real Calls



## API Reference Style

This section follows a professional reference layout: signatures, parameters, returns, notes, examples, and overload links.

For task-first, output-backed usage patterns of `stat`, `linearalg`, and `traverse`, see `series_query_user_guide`.

Type note for signatures below: `ExtractSpec` refers to `pluss::codem::ExtractSpec`.

## Indexing and Slicing Additions

### `pos`

#### Signatures

```cpp
Series pos(std::int64_t index) const
Series pos(std::int64_t row, std::int64_t col) const
Series pos(std::int64_t i, std::int64_t j, std::int64_t k) const
```

#### Notes

- Bounds-checked scalar selection helpers.
- `pos(index)` requires rank-1 input.
- `pos(row, col)` requires rank-2 input.
- `pos(i, j, k)` requires rank-3 input.
- Returns a rank-0 scalar `Series`.

#### Example

```cpp
auto v = pluss::make_series({10.0, 20.0, 30.0});
auto m = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {2, 2});
auto t = pluss::make_series({1.0,2.0,3.0,4.0,5.0,6.0,7.0,8.0}, {2,2,2});

auto a = v.pos(1);      // 20.0
auto b = m.pos(1, 0);   // 3.0
auto c = t.pos(1, 0, 1);// 6.0
```

### `size` and `sizes`

#### Signatures

```cpp
std::int64_t size(std::int64_t dim) const
std::int64_t size(int dim) const
pluss::msharp::RangeView::Int sizes() const
```

#### Notes

- `size(dim)` returns a single dimension extent (`int64_t`), matching libtorch-style usage.
- Negative dims are allowed (for example `size(-1)`).
- `sizes()` returns a lightweight shape view (`pluss::msharp::RangeView::Int`).
- Alias: `MSharp::RangeView::Int`.

#### Example

```cpp
auto x = pluss::make_series({1.0,2.0,3.0,4.0,5.0,6.0}, {1,2,3});

std::int64_t d0 = x.size(0);   // 1
std::int64_t d2 = x.size(-1);  // 3
auto shape = x.sizes();        // [1,2,3]
```

### `extract`

#### Signature

```cpp
Series extract(pluss::msharp::RangeView::Of<ExtractSpec> specs) const
```

#### Notes

- Slicing tokens use the concise `pluss::Slicing::*` function style:
  - `pluss::Slicing::SliceAll()`
  - `pluss::Slicing::SliceFrom(...)`
  - `pluss::Slicing::SliceTo(...)`
- `pluss::Slicing::SliceRange(...)`
- `pluss::Slicing::Ellipsis()`
- `pluss::Slicing::NewAxis()`
- Supports mixed scalar/range specs, ellipsis expansion, new-axis insertion, and bool/int advanced indexing specs.

#### Example

```cpp
auto x = pluss::make_series({1.0,2.0,3.0,4.0,5.0,6.0}, {2,3});
using pluss::codem::ExtractSpec;

// torch-like intent: x.slice(1, 1, x.size(1))
std::vector<ExtractSpec> right_specs{
  pluss::Slicing::SliceAll(),
  pluss::Slicing::SliceRange(1, x.size(1), 1)
};
auto right = x.extract(pluss::msharp::RangeView::Of<ExtractSpec>(right_specs));

// ellipsis + scalar index
std::vector<ExtractSpec> one_col_specs{
  pluss::Slicing::Ellipsis(),
  std::int64_t{1}
};
auto y = x.extract(pluss::msharp::RangeView::Of<ExtractSpec>(one_col_specs));
```

### `extract_put_`

#### Signature

```cpp
Series& extract_put_(pluss::msharp::RangeView::Of<ExtractSpec> specs, const Series& values)
```

#### Notes

- In-place update on selection defined by `specs`.
- This mutates `self` and returns `Series&`.

#### Example

```cpp
using pluss::codem::ExtractSpec;
auto w = pluss::zeros({2,3});
std::vector<ExtractSpec> write_specs{
  std::int64_t{1},
  pluss::Slicing::SliceRange(1, 3, 1)
};
w.extract_put_(pluss::msharp::RangeView::Of<ExtractSpec>(write_specs),
               pluss::make_series({9.0,8.0}, {2}));
```

### `extract_put`

#### Signature

```cpp
Series extract_put(pluss::msharp::RangeView::Of<ExtractSpec> specs, const Series& values) const
```

#### Notes

- Non-mutating form of `extract_put_`.
- Returns a copied/updated result.

#### Example

```cpp
using pluss::codem::ExtractSpec;
auto w = pluss::zeros({2,3});
std::vector<ExtractSpec> write_specs{
  std::int64_t{1},
  pluss::Slicing::SliceRange(1, 3, 1)
};
auto out = w.extract_put(pluss::msharp::RangeView::Of<ExtractSpec>(write_specs),
                         pluss::make_series({9.0,8.0}, {2}));
```

### `block`

#### Signature

```cpp
Series block(std::int64_t row_start, std::int64_t row_count,
             std::int64_t col_start, std::int64_t col_count) const
```

#### Notes

- Fast 2D-only rectangular view extraction.
- Throws `pluss::Error` if input is not rank-2.
- `row_start` and `col_start` accept negative indices (from end).
- `row_count` and `col_count` must be non-negative.
- Bounds are strict: requested rectangle must stay inside matrix bounds.

#### Examples

```cpp
auto mm = pluss::make_series({
  {1.0, 2.0, 3.0, 4.0},
  {5.0, 6.0, 7.0, 8.0},
  {9.0,10.0,11.0,12.0}
});

auto b1 = mm.block(1, 2, 1, 3);    // [[6,7,8],[10,11,12]]
auto b2 = mm.block(-2, 1, -3, 2);  // [[6,7]]
```

### `index`

#### Signature

```cpp
Series index(pluss::msharp::RangeView::Of<ExtractSpec> specs) const
```

#### Notes

- Canonical dynamic indexing entrypoint for `Series`.
- Accepts mixed scalar/slice/ellipsis/new-axis/advanced-index specs through `ExtractSpec`.
- Public policy is `RangeView::*` for array-like inputs (not `std::vector` overloads on the API surface).

#### Example

```cpp
auto x = pluss::make_series({
  {10.0, 11.0, 12.0, 13.0},
  {20.0, 21.0, 22.0, 23.0}
});

std::vector<pluss::codem::ExtractSpec> specs{
  pluss::Slicing::SliceAll(),
  pluss::Slicing::SliceRange(1, 4, 2)
};

auto out = x.index(pluss::msharp::RangeView::Of<pluss::codem::ExtractSpec>(specs));
// [[11,13],[21,23]]
```

### `slice`

#### Signatures

```cpp
Series slice(std::int64_t dim, std::int64_t start, std::int64_t end, std::int64_t step = 1) const
Series slice(std::int64_t dim, std::int64_t start, std::int64_t end) const
Series slice(std::int64_t dim, std::int64_t start) const
Series slice(std::int64_t dim) const
```

#### Notes

- View-oriented slicing helper (Torch-like semantics).
- Use positive `step` for cheap view traversal paths.
- Convenience overloads delegate to the full `(dim, start, end, step)` form.

#### Example

```cpp
auto x = pluss::make_series({
  {10.0, 11.0, 12.0, 13.0},
  {20.0, 21.0, 22.0, 23.0}
});

auto out = x.slice(1, 1, 4, 2); // [[11,13],[21,23]]
```

### `traverse_slice`

#### Signature

```cpp
Series traverse_slice(pluss::msharp::RangeView::Of<ExtractSpec> specs) const
```

#### Notes

- `traverse_slice` is strictly view-only selector traversal.
- Non-view-representable selectors fail fast (for example advanced index specs or non-positive slice step).
- Use `extract`/`extract_put_` when materialization-capable indexing is required.

#### Example

```cpp
auto x = pluss::make_series({
  {10.0, 11.0, 12.0, 13.0},
  {20.0, 21.0, 22.0, 23.0}
});

std::vector<pluss::codem::ExtractSpec> specs{
  pluss::Slicing::SliceAll(),
  pluss::Slicing::SliceRange(1, 3, 1)
};

auto view = x.traverse_slice(pluss::msharp::RangeView::Of<pluss::codem::ExtractSpec>(specs));
```

### `traverse_put_`

#### Signature

```cpp
Series& traverse_put_(pluss::msharp::RangeView::Of<ExtractSpec> specs, const Series& values)
```

#### Notes

- In-place write on a `traverse_slice(...)`-compatible selector.
- Follows the same view-only selector constraints as `traverse_slice`.

#### Example

```cpp
auto x = pluss::make_series({
  {10.0, 11.0, 12.0, 13.0},
  {20.0, 21.0, 22.0, 23.0}
});

std::vector<pluss::codem::ExtractSpec> specs{
  pluss::Slicing::SliceAll(),
  pluss::Slicing::SliceRange(1, 3, 1)
};

auto y = x.clone();
y.traverse_put_(pluss::msharp::RangeView::Of<pluss::codem::ExtractSpec>(specs),
                pluss::full({2,2}, -1.0));
```

### `traverse_put`

#### Signature

```cpp
Series traverse_put(pluss::msharp::RangeView::Of<ExtractSpec> specs, const Series& values) const
```

#### Notes

- Non-mutating form of `traverse_put_`.
- Returns copied/updated result.

#### Example

```cpp
auto x = pluss::make_series({
  {10.0, 11.0, 12.0, 13.0},
  {20.0, 21.0, 22.0, 23.0}
});
std::vector<pluss::codem::ExtractSpec> specs{
  pluss::Slicing::SliceAll(),
  pluss::Slicing::SliceRange(1, 3, 1)
};
auto out = x.traverse_put(pluss::msharp::RangeView::Of<pluss::codem::ExtractSpec>(specs),
                          pluss::full({2,2}, -1.0));
```

### `cube`

#### Signature

```cpp
Series cube(self)
```

#### Notes

- Convenience power helper equivalent to `pow(3)`.

#### Example

```cpp
auto x = pluss::make_series({-2.0, -1.0, 0.0, 1.0, 2.0});

auto out = x.cube(); // [-8,-1,0,1,8]
```

### `abs`

#### Signature

```cpp
Series abs(self)
```

#### Notes

- Elementwise absolute value.

#### Example

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});
auto out = x.abs();
```

### `abs2`

#### Signature

```cpp
Series abs2(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `abs2` to `self`. No additional arguments.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.hilbert().abs2();
```

### `acos`

#### Signature

```cpp
Series acos(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `acos` computation to `self`. No additional arguments.

#### Examples

```cpp
auto unit = pluss::make_series({-0.9,-0.5,-0.1,0.0,0.1,0.5,0.9});

auto out = unit.acos();
```

### `acosh`

#### Signature

```cpp
Series acosh(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `acosh` computation to `self`. No additional arguments.

#### Examples

```cpp
auto pos = pluss::make_series({0.2,0.5,1.0,2.0,3.0,4.0,5.0});

auto out = pos.add(1.0).acosh();
```

### `add`

#### Signature

```cpp
Series add(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `add` computation to `self`. Arguments: `const Series&`.

#### See also

- `Series add(self, scalar)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});
auto y = pluss::make_series({3,2,1,0,-1,-2,-3});

auto out = x.add(y);
```

### `add`

#### Signature

```cpp
Series add(self, scalar)
```

#### Parameters

`arg1` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `add` computation to `self`. Arguments: `scalar`. The current C++ overload takes `double`, but integer literals like `1` are accepted via implicit conversion.

#### See also

- `Series add(self, const Series&)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.add(1);
```

### `addbmm`

#### Signature

```cpp
Series addbmm(self, const Series& batch1, const Series& batch2)
```

#### Parameters

`batch1` : `const Series&`
    Required method argument.

`batch2` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes `beta * self + alpha * batch_matmul(batch1, batch2)` using batched matrix products. Arguments: `const Series& batch1, const Series& batch2`.

#### See also

- `Series addbmm(self, const Series& batch1, const Series& batch2, scalar beta, scalar alpha)`

#### Examples

```cpp
auto s1 = pluss::zeros({2,2});
auto batch1 = pluss::ones({3,2,4});
auto batch2 = pluss::ones({3,4,2});

auto out = s1.addbmm(batch1, batch2);
```

### `addbmm`

#### Signature

```cpp
Series addbmm(self, const Series& batch1, const Series& batch2, scalar beta, scalar alpha)
```

#### Parameters

`batch1` : `const Series&`
    Required method argument.

`batch2` : `const Series&`
    Required method argument.

`beta` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

`alpha` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes `beta * self + alpha * batch_matmul(batch1, batch2)` using batched matrix products. Arguments: `const Series& batch1, const Series& batch2, scalar beta, scalar alpha`.

#### See also

- `Series addbmm(self, const Series& batch1, const Series& batch2)`

#### Examples

```cpp
auto s1 = pluss::zeros({2,2});
auto batch1 = pluss::ones({3,2,4});
auto batch2 = pluss::ones({3,4,2});

auto out = s1.addbmm(batch1, batch2, 0.5, 2.0);
```

### `airy`

#### Signature

```cpp
Series airy(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `airy` computation to `self`. No additional arguments.

#### Examples

```cpp
auto pos = pluss::make_series({0.2,0.5,1.0,2.0,3.0,4.0,5.0});

auto out = pos.airy();
```

### `all`

#### Signature

```cpp
Series all(self, std::int64_t, bool)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `bool`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. Arguments: `std::int64_t, bool`.

#### Examples

```cpp
auto mm = pluss::make_series({
  {1,2,3},
  {4,5,6},
  {7,8,9}
});

auto out = mm.gt(0.0).all(1, false);
```

### `amax`

#### Signature

```cpp
Series amax(self, std::int64_t, bool)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `bool`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. Arguments: `std::int64_t, bool`.

#### Examples

```cpp
auto mm = pluss::make_series({
  {1,2,3},
  {4,5,6},
  {7,8,9}
});

auto out = mm.amax(1, false);
```

### `amin`

#### Signature

```cpp
Series amin(self, std::int64_t, bool)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `bool`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. Arguments: `std::int64_t, bool`.

#### Examples

```cpp
auto mm = pluss::make_series({
  {1,2,3},
  {4,5,6},
  {7,8,9}
});

auto out = mm.amin(1, false);
```

### `angle`

#### Signature

```cpp
Series angle(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `angle` to `self`. No additional arguments.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.hilbert().angle();
```

### `any`

#### Signature

```cpp
Series any(self, std::int64_t, bool)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `bool`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. Arguments: `std::int64_t, bool`.

#### Examples

```cpp
auto mm = pluss::make_series({
  {1,2,3},
  {4,5,6},
  {7,8,9}
});

auto out = mm.gt(7.0).any(1, false);
```

### `asin`

#### Signature

```cpp
Series asin(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `asin` computation to `self`. No additional arguments.

#### Examples

```cpp
auto unit = pluss::make_series({-0.9,-0.5,-0.1,0.0,0.1,0.5,0.9});

auto out = unit.asin();
```

### `asinh`

#### Signature

```cpp
Series asinh(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `asinh` computation to `self`. No additional arguments.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.asinh();
```

### `atan`

#### Signature

```cpp
Series atan(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `atan` computation to `self`. No additional arguments.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.atan();
```

### `atanh`

#### Signature

```cpp
Series atanh(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `atanh` computation to `self`. No additional arguments.

#### Examples

```cpp
auto unit = pluss::make_series({-0.9,-0.5,-0.1,0.0,0.1,0.5,0.9});

auto out = unit.atanh();
```

### `besseli`

#### Signature

```cpp
Series besseli(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `besseli` computation to `self`. No additional arguments.

#### Examples

```cpp
auto pos = pluss::make_series({0.2,0.5,1.0,2.0,3.0,4.0,5.0});

auto out = pos.besseli();
```

### `besselj`

#### Signature

```cpp
Series besselj(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `besselj` computation to `self`. No additional arguments.

#### Examples

```cpp
auto pos = pluss::make_series({0.2,0.5,1.0,2.0,3.0,4.0,5.0});

auto out = pos.besselj();
```

### `besselk`

#### Signature

```cpp
Series besselk(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `besselk` computation to `self`. No additional arguments.

#### Examples

```cpp
auto pos = pluss::make_series({0.2,0.5,1.0,2.0,3.0,4.0,5.0});

auto out = pos.besselk();
```

### `bessely`

#### Signature

```cpp
Series bessely(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `bessely` computation to `self`. No additional arguments.

#### Examples

```cpp
auto pos = pluss::make_series({0.2,0.5,1.0,2.0,3.0,4.0,5.0});

auto out = pos.bessely();
```

### `beta`

#### Signature

```cpp
Series beta(self, scalar)
```

#### Parameters

`arg1` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `beta` computation to `self`. Arguments: `scalar`. The current C++ overload takes `double`, but integer literals like `1` are accepted via implicit conversion.

#### Examples

```cpp
auto pos = pluss::make_series({0.2,0.5,1.0,2.0,3.0,4.0,5.0});

auto out = pos.beta(2.0);
```

### `betaln`

#### Signature

```cpp
Series betaln(self, scalar)
```

#### Parameters

`arg1` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `betaln` computation to `self`. Arguments: `scalar`. The current C++ overload takes `double`, but integer literals like `1` are accepted via implicit conversion.

#### Examples

```cpp
auto pos = pluss::make_series({0.2,0.5,1.0,2.0,3.0,4.0,5.0});

auto out = pos.betaln(2.0);
```

### `bincount`

#### Signature

```cpp
Series bincount(self, std::int64_t minlength)
```

#### Parameters

`minlength` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. Arguments: `std::int64_t minlength`.

#### Examples

```cpp
auto i = pluss::make_series({1,2,3,2,1,0,3});

auto out = i.sub(1.0).to(ScalarType::Int64).bincount(0);
```

### `bitwise_and`

#### Signature

```cpp
Series bitwise_and(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `bitwise_and` computation to `self`. Arguments: `const Series&`.

#### Examples

```cpp
auto bi = pluss::make_series({1,2,3,4,5,6,7});

auto out = bi.bitwise_and(bi.add(1));
```

### `bitwise_not`

#### Signature

```cpp
Series bitwise_not(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `bitwise_not` computation to `self`. No additional arguments.

#### Examples

```cpp
auto bi = pluss::make_series({1,2,3,4,5,6,7});

auto out = bi.bitwise_not();
```

### `bitwise_or`

#### Signature

```cpp
Series bitwise_or(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `bitwise_or` computation to `self`. Arguments: `const Series&`.

#### Examples

```cpp
auto bi = pluss::make_series({1,2,3,4,5,6,7});

auto out = bi.bitwise_or(bi.add(1));
```

### `bitwise_xor`

#### Signature

```cpp
Series bitwise_xor(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `bitwise_xor` computation to `self`. Arguments: `const Series&`.

#### Examples

```cpp
auto bi = pluss::make_series({1,2,3,4,5,6,7});

auto out = bi.bitwise_xor(bi.add(1));
```

### `cdf`

#### Signature

```cpp
Series cdf(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes Gaussian cumulative distribution values for `self` using the provided mean/stddev parameters. No additional arguments.

#### See also

- `Series cdf(self, scalar stddev)`
- `Series cdf(self, scalar mean, scalar stddev)`

#### Examples

```cpp
auto pos = pluss::make_series({0.2,0.5,1.0,2.0,3.0,4.0,5.0});

auto out = pos.stat.cdf();
```

### `cdf`

#### Signature

```cpp
Series cdf(self, scalar mean, scalar stddev)
```

#### Parameters

`mean` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

`stddev` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes Gaussian cumulative distribution values for `self` using the provided mean/stddev parameters. Arguments: `scalar mean, scalar stddev`.

#### See also

- `Series cdf(self)`
- `Series cdf(self, scalar stddev)`

#### Examples

```cpp
auto pos = pluss::make_series({0.2,0.5,1.0,2.0,3.0,4.0,5.0});

auto out = pos.stat.cdf(0.0, 1.5);
```

### `cdf`

#### Signature

```cpp
Series cdf(self, scalar stddev)
```

#### Parameters

`stddev` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes Gaussian cumulative distribution values for `self` using the provided mean/stddev parameters. Arguments: `scalar stddev`.

#### See also

- `Series cdf(self)`
- `Series cdf(self, scalar mean, scalar stddev)`

#### Examples

```cpp
auto pos = pluss::make_series({0.2,0.5,1.0,2.0,3.0,4.0,5.0});

auto out = pos.stat.cdf(1.5);
```

### `cdist`

#### Signature

```cpp
Series cdist(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `cdist` computation to `self`. Arguments: `const Series&`.

#### Examples

```cpp
auto m = pluss::make_series({
  {1,2,3},
  {4,5,6}
});
auto n = pluss::make_series({
  {6,5,4},
  {3,2,1}
});

auto out = m.cdist(n);
```

### `ceil`

#### Signature

```cpp
Series ceil(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `ceil` computation to `self`. No additional arguments.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.div(2).ceil();
```

### `center`

#### Signature

```cpp
Series center(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `center` computation to `self`. No additional arguments.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.center();
```

### `clamp_max`

#### Signature

```cpp
Series clamp_max(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `clamp_max` computation to `self`. Arguments: `const Series&`.

#### See also

- `Series clamp_max(self, scalar)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});
auto y = pluss::make_series({3,2,1,0,-1,-2,-3});

auto out = x.clamp_max(y);
```

### `clamp_max`

#### Signature

```cpp
Series clamp_max(self, scalar)
```

#### Parameters

`arg1` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `clamp_max` computation to `self`. Arguments: `scalar`. The current C++ overload takes `double`, but integer literals like `1` are accepted via implicit conversion.

#### See also

- `Series clamp_max(self, const Series&)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.clamp_max(0.0);
```

### `clamp_min`

#### Signature

```cpp
Series clamp_min(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `clamp_min` computation to `self`. Arguments: `const Series&`.

#### See also

- `Series clamp_min(self, scalar)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});
auto y = pluss::make_series({3,2,1,0,-1,-2,-3});

auto out = x.clamp_min(y);
```

### `clamp_min`

#### Signature

```cpp
Series clamp_min(self, scalar)
```

#### Parameters

`arg1` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `clamp_min` computation to `self`. Arguments: `scalar`. The current C++ overload takes `double`, but integer literals like `1` are accepted via implicit conversion.

#### See also

- `Series clamp_min(self, const Series&)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.clamp_min(0.0);
```

### `cos`

#### Signature

```cpp
Series cos(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `cos` computation to `self`. No additional arguments.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.cos();
```

### `cosh`

#### Signature

```cpp
Series cosh(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `cosh` computation to `self`. No additional arguments.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.cosh();
```

### `cplxpair`

#### Signature

```cpp
Series cplxpair(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `cplxpair` computation to `self`. No additional arguments.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.hilbert().cplxpair();
```

### `cumprod`

#### Signature

```cpp
Series cumprod(self, std::int64_t, bool)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `bool`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. Arguments: `std::int64_t, bool`.

#### Examples

```cpp
auto mm = pluss::make_series({
  {1,2,3},
  {4,5,6},
  {7,8,9}
});

auto out = mm.cumprod(1, false);
```

### `cumprod_`

#### Signature

```cpp
Series& cumprod_(self, std::int64_t, bool)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `bool`
    Required method argument.

#### Returns

`Series&`
    Return value produced by this method call.

#### Notes

In-place variant of `cumprod`; it mutates `self` and returns `self`. Arguments: `std::int64_t, bool`.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto t = x.clone(); t.cumprod_(1, false);
```

### `cumsum`

#### Signature

```cpp
Series cumsum(self, std::int64_t, bool)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `bool`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. Arguments: `std::int64_t, bool`.

#### Examples

```cpp
auto mm = pluss::make_series({
  {1,2,3},
  {4,5,6},
  {7,8,9}
});

auto out = mm.cumsum(1, false);
```

### `cumsum_`

#### Signature

```cpp
Series& cumsum_(self, std::int64_t, bool)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `bool`
    Required method argument.

#### Returns

`Series&`
    Return value produced by this method call.

#### Notes

In-place variant of `cumsum`; it mutates `self` and returns `self`. Arguments: `std::int64_t, bool`.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto t = x.clone(); t.cumsum_(1, false);
```

### `dct`

#### Signature

```cpp
Series dct(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `dct` computation to `self`. No additional arguments.

#### See also

- `Series dct(self, std::int64_t dim)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.dct();
```

### `dct`

#### Signature

```cpp
Series dct(self, std::int64_t dim)
```

#### Parameters

`dim` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `dct` computation to `self`. Arguments: `std::int64_t dim`.

#### See also

- `Series dct(self)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.dct(0);
```

### `div`

#### Signature

```cpp
Series div(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `div` computation to `self`. Arguments: `const Series&`.

#### See also

- `Series div(self, scalar)`

#### Examples

```cpp
auto pos = pluss::make_series({0.2,0.5,1.0,2.0,3.0,4.0,5.0});

auto out = pos.div(pos.add(1.0));
```

### `div`

#### Signature

```cpp
Series div(self, scalar)
```

#### Parameters

`arg1` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `div` computation to `self`. Arguments: `scalar`. The current C++ overload takes `double`, but integer literals like `1` are accepted via implicit conversion.

#### See also

- `Series div(self, const Series&)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.div(2);
```

### `dst`

#### Signature

```cpp
Series dst(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `dst` computation to `self`. No additional arguments.

#### See also

- `Series dst(self, std::int64_t dim)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.dst();
```

### `dst`

#### Signature

```cpp
Series dst(self, std::int64_t dim)
```

#### Parameters

`dim` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `dst` computation to `self`. Arguments: `std::int64_t dim`.

#### See also

- `Series dst(self)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.dst(0);
```

### `ediff1d`

#### Signature

```cpp
Series ediff1d(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `ediff1d` computation to `self`. No additional arguments.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.ediff1d();
```

### `eq`

#### Signature

```cpp
Series eq(self, scalar)
```

#### Parameters

`arg1` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `eq` to `self`. Arguments: `scalar`. The current C++ overload takes `double`, but integer literals like `1` are accepted via implicit conversion.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.eq(0.0);
```

### `erf`

#### Signature

```cpp
Series erf(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `erf` computation to `self`. No additional arguments.

#### Examples

```cpp
auto unit = pluss::make_series({-0.9,-0.5,-0.1,0.0,0.1,0.5,0.9});

auto out = unit.erf();
```

### `erfc`

#### Signature

```cpp
Series erfc(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `erfc` computation to `self`. No additional arguments.

#### Examples

```cpp
auto unit = pluss::make_series({-0.9,-0.5,-0.1,0.0,0.1,0.5,0.9});

auto out = unit.erfc();
```

### `erfcinv`

#### Signature

```cpp
Series erfcinv(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `erfcinv` computation to `self`. No additional arguments.

#### Examples

```cpp
auto unit = pluss::make_series({-0.9,-0.5,-0.1,0.0,0.1,0.5,0.9});

auto out = unit.abs().erfcinv();
```

### `exp`

#### Signature

```cpp
Series exp(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `exp` computation to `self`. No additional arguments.

#### Examples

```cpp
auto pos = pluss::make_series({0.2,0.5,1.0,2.0,3.0,4.0,5.0});

auto out = pos.exp();
```

### `expm1`

#### Signature

```cpp
Series expm1(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `expm1` computation to `self`. No additional arguments.

#### Examples

```cpp
auto unit = pluss::make_series({-0.9,-0.5,-0.1,0.0,0.1,0.5,0.9});

auto out = unit.expm1();
```

### `fftshift`

#### Signature

```cpp
Series fftshift(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. No additional arguments.

#### See also

- `Series fftshift(self, const std::vector<std::int64_t>& dims)`

#### Examples

```cpp
auto mm = pluss::make_series({
  {1,2,3},
  {4,5,6},
  {7,8,9}
});

auto out = mm.fftshift();
```

### `fftshift`

#### Signature

```cpp
Series fftshift(self, const std::vector<std::int64_t>& dims)
```

#### Parameters

`dims` : `const std::vector<std::int64_t>&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `const std::vector<std::int64_t>& dims`.

#### See also

- `Series fftshift(self)`

#### Examples

```cpp
auto mm = pluss::make_series({
  {1,2,3},
  {4,5,6},
  {7,8,9}
});

auto out = mm.fftshift(std::vector<std::int64_t>{0,1});
```

### `flatten`

#### Signature

```cpp
Series flatten(self, std::int64_t, std::int64_t)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `std::int64_t, std::int64_t`.

#### Examples

```cpp
auto mm = pluss::make_series({
  {1,2,3},
  {4,5,6},
  {7,8,9}
});

auto out = mm.flatten(0, 1);
```

### `floor`

#### Signature

```cpp
Series floor(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `floor` computation to `self`. No additional arguments.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.div(2).floor();
```

### `gather`

#### Signature

```cpp
Series gather(self, std::int64_t, const Series&)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `gather` computation to `self`. Arguments: `std::int64_t, const Series&`.

#### Examples

```cpp
auto gather_src = pluss::make_series({
  {10,20,30},
  {40,50,60}
});
auto gather_idx = pluss::make_series({
  {2,1,0},
  {0,1,2}
});

auto out = gather_src.gather(1, gather_idx);
```

### `ge`

#### Signature

```cpp
Series ge(self, scalar)
```

#### Parameters

`arg1` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `ge` to `self`. Arguments: `scalar`. The current C++ overload takes `double`, but integer literals like `1` are accepted via implicit conversion.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.ge(0.0);
```

### `gt`

#### Signature

```cpp
Series gt(self, scalar)
```

#### Parameters

`arg1` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `gt` to `self`. Arguments: `scalar`. The current C++ overload takes `double`, but integer literals like `1` are accepted via implicit conversion.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.gt(0.0);
```

### `hilbert`

#### Signature

```cpp
Series hilbert(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `hilbert` computation to `self`. No additional arguments.

#### See also

- `Series hilbert(self, std::int64_t dim)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.hilbert();
```

### `hilbert`

#### Signature

```cpp
Series hilbert(self, std::int64_t dim)
```

#### Parameters

`dim` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `hilbert` computation to `self`. Arguments: `std::int64_t dim`.

#### See also

- `Series hilbert(self)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.hilbert(0);
```

### `histc`

#### Signature

```cpp
Series histc(self, std::int64_t, scalar, scalar)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

`arg3` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. Arguments: `std::int64_t, scalar, scalar`.

#### Examples

```cpp
auto pos = pluss::make_series({0.2,0.5,1.0,2.0,3.0,4.0,5.0});

auto out = pos.histc(5, 0.0, 5.0);
```

### `histcounts`

#### Signature

```cpp
Series histcounts(self, std::int64_t, scalar, scalar)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

`arg3` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. Arguments: `std::int64_t, scalar, scalar`.

#### Examples

```cpp
auto pos = pluss::make_series({0.2,0.5,1.0,2.0,3.0,4.0,5.0});

auto out = pos.histcounts(5, 0.0, 5.0);
```

### `idct`

#### Signature

```cpp
Series idct(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `idct` computation to `self`. No additional arguments.

#### See also

- `Series idct(self, std::int64_t dim)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.dct().idct();
```

### `idct`

#### Signature

```cpp
Series idct(self, std::int64_t dim)
```

#### Parameters

`dim` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `idct` computation to `self`. Arguments: `std::int64_t dim`.

#### See also

- `Series idct(self)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.idct(0);
```

### `idst`

#### Signature

```cpp
Series idst(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `idst` computation to `self`. No additional arguments.

#### See also

- `Series idst(self, std::int64_t dim)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.dst().idst();
```

### `idst`

#### Signature

```cpp
Series idst(self, std::int64_t dim)
```

#### Parameters

`dim` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `idst` computation to `self`. Arguments: `std::int64_t dim`.

#### See also

- `Series idst(self)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.idst(0);
```

### `ifftshift`

#### Signature

```cpp
Series ifftshift(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. No additional arguments.

#### See also

- `Series ifftshift(self, const std::vector<std::int64_t>& dims)`

#### Examples

```cpp
auto mm = pluss::make_series({
  {1,2,3},
  {4,5,6},
  {7,8,9}
});

auto out = mm.fftshift().ifftshift();
```

### `ifftshift`

#### Signature

```cpp
Series ifftshift(self, const std::vector<std::int64_t>& dims)
```

#### Parameters

`dims` : `const std::vector<std::int64_t>&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `const std::vector<std::int64_t>& dims`.

#### See also

- `Series ifftshift(self)`

#### Examples

```cpp
auto mm = pluss::make_series({
  {1,2,3},
  {4,5,6},
  {7,8,9}
});

auto out = mm.ifftshift(std::vector<std::int64_t>{0,1});
```

### `imag`

#### Signature

```cpp
Series imag(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `imag` to `self`. No additional arguments.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.hilbert().imag();
```

### `isfinite`

#### Signature

```cpp
Series isfinite(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `isfinite` to `self`. No additional arguments.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.isfinite();
```

### `isinf`

#### Signature

```cpp
Series isinf(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `isinf` to `self`. No additional arguments.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.isinf();
```

### `isnan`

#### Signature

```cpp
Series isnan(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `isnan` to `self`. No additional arguments.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.isnan();
```

### `kurtosis`

#### Signature

```cpp
Series kurtosis(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `kurtosis` computation to `self`. No additional arguments.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.stat.kurtosis();
```

### `le`

#### Signature

```cpp
Series le(self, scalar)
```

#### Parameters

`arg1` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `le` to `self`. Arguments: `scalar`. The current C++ overload takes `double`, but integer literals like `1` are accepted via implicit conversion.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.le(0.0);
```

### `lgamma`

#### Signature

```cpp
Series lgamma(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `lgamma` computation to `self`. No additional arguments.

#### Examples

```cpp
auto pos = pluss::make_series({0.2,0.5,1.0,2.0,3.0,4.0,5.0});

auto out = pos.lgamma();
```

### `log`

#### Signature

```cpp
Series log(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `log` computation to `self`. No additional arguments.

#### Examples

```cpp
auto pos = pluss::make_series({0.2,0.5,1.0,2.0,3.0,4.0,5.0});

auto out = pos.log();
```

### `log1p`

#### Signature

```cpp
Series log1p(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `log1p` computation to `self`. No additional arguments.

#### Examples

```cpp
auto pos = pluss::make_series({0.2,0.5,1.0,2.0,3.0,4.0,5.0});

auto out = pos.log1p();
```

### `logical_and`

#### Signature

```cpp
Series logical_and(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `logical_and` computation to `self`. Arguments: `const Series&`.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.gt(0.0).logical_and(x.lt(3.0));
```

### `logical_not`

#### Signature

```cpp
Series logical_not(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `logical_not` computation to `self`. No additional arguments.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.gt(0.0).logical_not();
```

### `logical_or`

#### Signature

```cpp
Series logical_or(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `logical_or` computation to `self`. Arguments: `const Series&`.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.gt(0.0).logical_or(x.lt(-2.0));
```

### `logical_xor`

#### Signature

```cpp
Series logical_xor(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `logical_xor` computation to `self`. Arguments: `const Series&`.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.gt(0.0).logical_xor(x.lt(0.0));
```

### `lt`

#### Signature

```cpp
Series lt(self, scalar)
```

#### Parameters

`arg1` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `lt` to `self`. Arguments: `scalar`. The current C++ overload takes `double`, but integer literals like `1` are accepted via implicit conversion.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.lt(0.0);
```

### `masked_scatter`

#### Signature

```cpp
Series masked_scatter(self, const Series&, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

`arg2` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `masked_scatter` to `self`. Arguments: `const Series&, const Series&`.

#### Examples

```cpp
auto m = pluss::make_series({
  {1,2,3},
  {4,5,6}
});
auto n = pluss::make_series({
  {6,5,4},
  {3,2,1}
});
auto mask = pluss::ones({2,3}, pluss::ScalarType::Bool);

auto out = m.masked_scatter(mask, n);
```

### `masked_scatter_`

#### Signature

```cpp
Series& masked_scatter_(self, const Series&, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

`arg2` : `const Series&`
    Required method argument.

#### Returns

`Series&`
    Return value produced by this method call.

#### Notes

In-place variant of `masked_scatter`; it mutates `self` and returns `self`. Arguments: `const Series&, const Series&`.

#### Examples

```cpp
auto m = pluss::make_series({
  {1,2,3},
  {4,5,6}
});
auto n = pluss::make_series({
  {6,5,4},
  {3,2,1}
});
auto mask = pluss::ones({2,3}, pluss::ScalarType::Bool);

auto t = m.clone(); t.masked_scatter_(mask, n);
```

### `masked_select`

#### Signature

```cpp
Series masked_select(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `masked_select` to `self`. Arguments: `const Series&`.

#### Examples

```cpp
auto m = pluss::make_series({
  {1,2,3},
  {4,5,6}
});
auto mask = pluss::ones({2,3}, pluss::ScalarType::Bool);

auto out = m.masked_select(mask);
```

### `maximum`

#### Signature

```cpp
Series maximum(self, scalar)
```

#### Parameters

`arg1` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `maximum` computation to `self`. Arguments: `scalar`. The current C++ overload takes `double`, but integer literals like `1` are accepted via implicit conversion.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.maximum(0.0);
```

### `mean`

#### Signature

```cpp
Series mean(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. No additional arguments.

#### See also

- `Series mean(self, std::int64_t, bool)`

#### Examples

```cpp
auto mm = pluss::make_series({
  {1,2,3},
  {4,5,6},
  {7,8,9}
});

auto out = mm.mean();
```

### `mean`

#### Signature

```cpp
Series mean(self, std::int64_t, bool)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `bool`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. Arguments: `std::int64_t, bool`.

#### See also

- `Series mean(self)`

#### Examples

```cpp
auto mm = pluss::make_series({
  {1,2,3},
  {4,5,6},
  {7,8,9}
});

auto out = mm.mean(1, true);
```

### `minimum`

#### Signature

```cpp
Series minimum(self, scalar)
```

#### Parameters

`arg1` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `minimum` computation to `self`. Arguments: `scalar`. The current C++ overload takes `double`, but integer literals like `1` are accepted via implicit conversion.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.minimum(0.0);
```

### `movmax`

#### Signature

```cpp
Series movmax(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes moving-window maxima along a dimension (default is last dimension). No additional arguments.

#### See also

- `Series movmax(self, std::int64_t window)`
- `Series movmax(self, std::int64_t window, std::int64_t dim)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.stat.movmax();
```

### `movmax`

#### Signature

```cpp
Series movmax(self, std::int64_t window)
```

#### Parameters

`window` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes moving-window maxima along a dimension (default is last dimension). Arguments: `std::int64_t window`.

#### See also

- `Series movmax(self)`
- `Series movmax(self, std::int64_t window, std::int64_t dim)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.stat.movmax(3);
```

### `movmax`

#### Signature

```cpp
Series movmax(self, std::int64_t window, std::int64_t dim)
```

#### Parameters

`window` : `std::int64_t`
    Required method argument.

`dim` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes moving-window maxima along a dimension (default is last dimension). Arguments: `std::int64_t window, std::int64_t dim`.

#### See also

- `Series movmax(self)`
- `Series movmax(self, std::int64_t window)`

#### Examples

```cpp
auto mm = pluss::make_series({
  {1,2,3},
  {4,5,6},
  {7,8,9}
});

auto out = mm.stat.movmax(2, 1);
```

### `movmean`

#### Signature

```cpp
Series movmean(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes moving-window means along a dimension (default is last dimension). No additional arguments.

#### See also

- `Series movmean(self, std::int64_t window)`
- `Series movmean(self, std::int64_t window, std::int64_t dim)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.stat.movmean();
```

### `movmean`

#### Signature

```cpp
Series movmean(self, std::int64_t window)
```

#### Parameters

`window` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes moving-window means along a dimension (default is last dimension). Arguments: `std::int64_t window`.

#### See also

- `Series movmean(self)`
- `Series movmean(self, std::int64_t window, std::int64_t dim)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.stat.movmean(3);
```

### `movmean`

#### Signature

```cpp
Series movmean(self, std::int64_t window, std::int64_t dim)
```

#### Parameters

`window` : `std::int64_t`
    Required method argument.

`dim` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes moving-window means along a dimension (default is last dimension). Arguments: `std::int64_t window, std::int64_t dim`.

#### See also

- `Series movmean(self)`
- `Series movmean(self, std::int64_t window)`

#### Examples

```cpp
auto mm = pluss::make_series({
  {1,2,3},
  {4,5,6},
  {7,8,9}
});

auto out = mm.stat.movmean(2, 1);
```

### `movmin`

#### Signature

```cpp
Series movmin(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes moving-window minima along a dimension (default is last dimension). No additional arguments.

#### See also

- `Series movmin(self, std::int64_t window)`
- `Series movmin(self, std::int64_t window, std::int64_t dim)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.stat.movmin();
```

### `movmin`

#### Signature

```cpp
Series movmin(self, std::int64_t window)
```

#### Parameters

`window` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes moving-window minima along a dimension (default is last dimension). Arguments: `std::int64_t window`.

#### See also

- `Series movmin(self)`
- `Series movmin(self, std::int64_t window, std::int64_t dim)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.stat.movmin(3);
```

### `movmin`

#### Signature

```cpp
Series movmin(self, std::int64_t window, std::int64_t dim)
```

#### Parameters

`window` : `std::int64_t`
    Required method argument.

`dim` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes moving-window minima along a dimension (default is last dimension). Arguments: `std::int64_t window, std::int64_t dim`.

#### See also

- `Series movmin(self)`
- `Series movmin(self, std::int64_t window)`

#### Examples

```cpp
auto mm = pluss::make_series({
  {1,2,3},
  {4,5,6},
  {7,8,9}
});

auto out = mm.stat.movmin(2, 1);
```

### `movstd`

#### Signature

```cpp
Series movstd(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes moving-window standard deviation along a dimension (default is last dimension). No additional arguments.

#### See also

- `Series movstd(self, std::int64_t window)`
- `Series movstd(self, std::int64_t window, std::int64_t dim)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.stat.movstd();
```

### `movstd`

#### Signature

```cpp
Series movstd(self, std::int64_t window)
```

#### Parameters

`window` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes moving-window standard deviation along a dimension (default is last dimension). Arguments: `std::int64_t window`.

#### See also

- `Series movstd(self)`
- `Series movstd(self, std::int64_t window, std::int64_t dim)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.stat.movstd(3);
```

### `movstd`

#### Signature

```cpp
Series movstd(self, std::int64_t window, std::int64_t dim)
```

#### Parameters

`window` : `std::int64_t`
    Required method argument.

`dim` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes moving-window standard deviation along a dimension (default is last dimension). Arguments: `std::int64_t window, std::int64_t dim`.

#### See also

- `Series movstd(self)`
- `Series movstd(self, std::int64_t window)`

#### Examples

```cpp
auto mm = pluss::make_series({
  {1,2,3},
  {4,5,6},
  {7,8,9}
});

auto out = mm.stat.movstd(2, 1);
```

### `movsum`

#### Signature

```cpp
Series movsum(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes moving-window sums along a dimension (default is last dimension). No additional arguments.

#### See also

- `Series movsum(self, std::int64_t window)`
- `Series movsum(self, std::int64_t window, std::int64_t dim)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.stat.movsum();
```

### `movsum`

#### Signature

```cpp
Series movsum(self, std::int64_t window)
```

#### Parameters

`window` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes moving-window sums along a dimension (default is last dimension). Arguments: `std::int64_t window`.

#### See also

- `Series movsum(self)`
- `Series movsum(self, std::int64_t window, std::int64_t dim)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.stat.movsum(3);
```

### `movsum`

#### Signature

```cpp
Series movsum(self, std::int64_t window, std::int64_t dim)
```

#### Parameters

`window` : `std::int64_t`
    Required method argument.

`dim` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes moving-window sums along a dimension (default is last dimension). Arguments: `std::int64_t window, std::int64_t dim`.

#### See also

- `Series movsum(self)`
- `Series movsum(self, std::int64_t window)`

#### Examples

```cpp
auto mm = pluss::make_series({
  {1,2,3},
  {4,5,6},
  {7,8,9}
});

auto out = mm.stat.movsum(2, 1);
```

### `movvar`

#### Signature

```cpp
Series movvar(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes moving-window variance along a dimension (default is last dimension). No additional arguments.

#### See also

- `Series movvar(self, std::int64_t window)`
- `Series movvar(self, std::int64_t window, std::int64_t dim)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.stat.movvar();
```

### `movvar`

#### Signature

```cpp
Series movvar(self, std::int64_t window)
```

#### Parameters

`window` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes moving-window variance along a dimension (default is last dimension). Arguments: `std::int64_t window`.

#### See also

- `Series movvar(self)`
- `Series movvar(self, std::int64_t window, std::int64_t dim)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.stat.movvar(3);
```

### `movvar`

#### Signature

```cpp
Series movvar(self, std::int64_t window, std::int64_t dim)
```

#### Parameters

`window` : `std::int64_t`
    Required method argument.

`dim` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes moving-window variance along a dimension (default is last dimension). Arguments: `std::int64_t window, std::int64_t dim`.

#### See also

- `Series movvar(self)`
- `Series movvar(self, std::int64_t window)`

#### Examples

```cpp
auto mm = pluss::make_series({
  {1,2,3},
  {4,5,6},
  {7,8,9}
});

auto out = mm.stat.movvar(2, 1);
```

### `mul`

#### Signature

```cpp
Series mul(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `mul` computation to `self`. Arguments: `const Series&`.

#### See also

- `Series mul(self, scalar)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});
auto y = pluss::make_series({3,2,1,0,-1,-2,-3});

auto out = x.mul(y);
```

### `mul`

#### Signature

```cpp
Series mul(self, scalar)
```

#### Parameters

`arg1` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `mul` computation to `self`. Arguments: `scalar`. The current C++ overload takes `double`, but integer literals like `1` are accepted via implicit conversion.

#### See also

- `Series mul(self, const Series&)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.mul(2);
```

### `nan_to_num`

#### Signature

```cpp
Series nan_to_num(self, scalar, scalar, scalar)
```

#### Parameters

`arg1` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

`arg2` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

`arg3` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `nan_to_num` computation to `self`. Arguments: `scalar, scalar, scalar`.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.nan_to_num(0.0, 1e9, -1e9);
```

### `nanstd`

#### Signature

```cpp
Series nanstd(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `nanstd` computation to `self`. No additional arguments.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.nanstd();
```

### `nanvar`

#### Signature

```cpp
Series nanvar(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `nanvar` computation to `self`. No additional arguments.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.nanvar();
```

### `ne`

#### Signature

```cpp
Series ne(self, scalar)
```

#### Parameters

`arg1` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `ne` to `self`. Arguments: `scalar`. The current C++ overload takes `double`, but integer literals like `1` are accepted via implicit conversion.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.ne(0.0);
```

### `nonzero`

#### Signature

```cpp
Series nonzero(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. No additional arguments.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});
auto cond = x.gt(0.0);

auto out = cond.nonzero();
```

### `norm`

#### Signature

```cpp
Series norm(self, scalar)
```

#### Parameters

`arg1` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `norm` computation to `self`. Arguments: `scalar`. The current C++ overload takes `double`, but integer literals like `1` are accepted via implicit conversion.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.norm(2.0);
```

### `normc`

#### Signature

```cpp
Series normc(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `normc` computation to `self`. No additional arguments.

#### Examples

```cpp
auto m = pluss::make_series({
  {1,2,3},
  {4,5,6}
});

auto out = m.normc();
```

### `normr`

#### Signature

```cpp
Series normr(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `normr` computation to `self`. No additional arguments.

#### Examples

```cpp
auto m = pluss::make_series({
  {1,2,3},
  {4,5,6}
});

auto out = m.normr();
```

### `pdf`

#### Signature

```cpp
Series pdf(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes Gaussian probability density values for `self` using the provided mean/stddev parameters. No additional arguments.

#### See also

- `Series pdf(self, scalar stddev)`
- `Series pdf(self, scalar mean, scalar stddev)`

#### Examples

```cpp
auto pos = pluss::make_series({0.2,0.5,1.0,2.0,3.0,4.0,5.0});

auto out = pos.stat.pdf();
```

### `pdf`

#### Signature

```cpp
Series pdf(self, scalar mean, scalar stddev)
```

#### Parameters

`mean` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

`stddev` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes Gaussian probability density values for `self` using the provided mean/stddev parameters. Arguments: `scalar mean, scalar stddev`.

#### See also

- `Series pdf(self)`
- `Series pdf(self, scalar stddev)`

#### Examples

```cpp
auto pos = pluss::make_series({0.2,0.5,1.0,2.0,3.0,4.0,5.0});

auto out = pos.stat.pdf(0.0, 1.5);
```

### `pdf`

#### Signature

```cpp
Series pdf(self, scalar stddev)
```

#### Parameters

`stddev` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes Gaussian probability density values for `self` using the provided mean/stddev parameters. Arguments: `scalar stddev`.

#### See also

- `Series pdf(self)`
- `Series pdf(self, scalar mean, scalar stddev)`

#### Examples

```cpp
auto pos = pluss::make_series({0.2,0.5,1.0,2.0,3.0,4.0,5.0});

auto out = pos.stat.pdf(1.5);
```

### `logpdf`

#### Signature

```cpp
Series logpdf(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes Gaussian log-density values for `self` using the provided mean/stddev parameters. No additional arguments.

#### See also

- `Series logpdf(self, scalar stddev)`
- `Series logpdf(self, scalar mean, scalar stddev)`

#### Examples

```cpp
auto pos = pluss::make_series({0.2,0.5,1.0,2.0,3.0,4.0,5.0});

auto out = pos.stat.logpdf();
```

### `logpdf`

#### Signature

```cpp
Series logpdf(self, scalar mean, scalar stddev)
```

#### Parameters

`mean` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

`stddev` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes Gaussian log-density values for `self` using the provided mean/stddev parameters. Arguments: `scalar mean, scalar stddev`.

#### See also

- `Series logpdf(self)`
- `Series logpdf(self, scalar stddev)`

#### Examples

```cpp
auto pos = pluss::make_series({0.2,0.5,1.0,2.0,3.0,4.0,5.0});

auto out = pos.stat.logpdf(0.0, 1.5);
```

### `logpdf`

#### Signature

```cpp
Series logpdf(self, scalar stddev)
```

#### Parameters

`stddev` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes Gaussian log-density values for `self` using the provided mean/stddev parameters. Arguments: `scalar stddev`.

#### See also

- `Series logpdf(self)`
- `Series logpdf(self, scalar mean, scalar stddev)`

#### Examples

```cpp
auto pos = pluss::make_series({0.2,0.5,1.0,2.0,3.0,4.0,5.0});

auto out = pos.stat.logpdf(1.5);
```

### `logcdf`

#### Signature

```cpp
Series logcdf(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes Gaussian log-CDF values for `self` using the provided mean/stddev parameters. No additional arguments.

#### See also

- `Series logcdf(self, scalar stddev)`
- `Series logcdf(self, scalar mean, scalar stddev)`

#### Examples

```cpp
auto pos = pluss::make_series({0.2,0.5,1.0,2.0,3.0,4.0,5.0});

auto out = pos.stat.logcdf();
```

### `logcdf`

#### Signature

```cpp
Series logcdf(self, scalar mean, scalar stddev)
```

#### Parameters

`mean` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

`stddev` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes Gaussian log-CDF values for `self` using the provided mean/stddev parameters. Arguments: `scalar mean, scalar stddev`.

#### See also

- `Series logcdf(self)`
- `Series logcdf(self, scalar stddev)`

#### Examples

```cpp
auto pos = pluss::make_series({0.2,0.5,1.0,2.0,3.0,4.0,5.0});

auto out = pos.stat.logcdf(0.0, 1.5);
```

### `logcdf`

#### Signature

```cpp
Series logcdf(self, scalar stddev)
```

#### Parameters

`stddev` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes Gaussian log-CDF values for `self` using the provided mean/stddev parameters. Arguments: `scalar stddev`.

#### See also

- `Series logcdf(self)`
- `Series logcdf(self, scalar mean, scalar stddev)`

#### Examples

```cpp
auto pos = pluss::make_series({0.2,0.5,1.0,2.0,3.0,4.0,5.0});

auto out = pos.stat.logcdf(1.5);
```

### `pdist2`

#### Signature

```cpp
Series pdist2(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `pdist2` computation to `self`. Arguments: `const Series&`.

#### Examples

```cpp
auto m = pluss::make_series({
  {1,2,3},
  {4,5,6}
});
auto n = pluss::make_series({
  {6,5,4},
  {3,2,1}
});

auto out = m.pdist2(n);
```

### `permute`

#### Signature

```cpp
Series permute(self, std::span<const std::int64_t>)
```

#### Parameters

`arg1` : `std::span<const std::int64_t>`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `std::span<const std::int64_t>`.

#### Examples

```cpp
auto mm = pluss::make_series({
  {1,2,3},
  {4,5,6},
  {7,8,9}
});
std::vector<std::int64_t> perm_10{1,0};

auto out = mm.permute(std::span<const std::int64_t>(perm_10.data(), perm_10.size()));
```

### `pow`

#### Signature

```cpp
Series pow(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `pow` computation to `self`. Arguments: `const Series&`.

#### See also

- `Series pow(self, scalar)`

#### Examples

```cpp
auto pos = pluss::make_series({0.2,0.5,1.0,2.0,3.0,4.0,5.0});

auto out = pos.pow(pos.mul(0.0).add(2.0));
```

### `pow`

#### Signature

```cpp
Series pow(self, scalar)
```

#### Parameters

`arg1` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `pow` computation to `self`. Arguments: `scalar`. The current C++ overload takes `double`, but integer literals like `1` are accepted via implicit conversion.

#### See also

- `Series pow(self, const Series&)`

#### Examples

```cpp
auto pos = pluss::make_series({0.2,0.5,1.0,2.0,3.0,4.0,5.0});

auto out = pos.pow(2);
```

### `put_`

#### Signature

```cpp
Series& put_(self, const Series&, const Series&, bool)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

`arg2` : `const Series&`
    Required method argument.

`arg3` : `bool`
    Required method argument.

#### Returns

`Series&`
    Return value produced by this method call.

#### Notes

In-place variant of `put`; it mutates `self` and returns `self`. Arguments: `const Series&, const Series&, bool`.

#### Examples

```cpp
auto gather_src = pluss::make_series({
  {10,20,30},
  {40,50,60}
});
auto flat_idx = pluss::make_series({0,3,5});
auto flat_src = pluss::make_series({99,88,77});

auto t = gather_src.clone(); t.put_(flat_idx, flat_src, false);
```

### `real`

#### Signature

```cpp
Series real(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `real` to `self`. No additional arguments.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.hilbert().real();
```

### `reciprocal`

#### Signature

```cpp
Series reciprocal(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `reciprocal` computation to `self`. No additional arguments.

#### Examples

```cpp
auto pos = pluss::make_series({0.2,0.5,1.0,2.0,3.0,4.0,5.0});

auto out = pos.reciprocal();
```

### `relu`

#### Signature

```cpp
Series relu(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `relu` computation to `self`. No additional arguments.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.relu();
```

### `reshape`

#### Signature

```cpp
Series reshape(self, std::span<const std::int64_t>)
```

#### Parameters

`arg1` : `std::span<const std::int64_t>`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `std::span<const std::int64_t>`.

#### Examples

```cpp
auto m = pluss::make_series({
  {1,2,3},
  {4,5,6}
});
std::vector<std::int64_t> shape_3x2{3,2};

auto out = m.reshape(std::span<const std::int64_t>(shape_3x2.data(), shape_3x2.size()));
```

### `round`

#### Signature

```cpp
Series round(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `round` computation to `self`. No additional arguments.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.div(2).round();
```

### `scatter`

#### Signature

```cpp
Series scatter(self, std::int64_t, const Series&, const Series&)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `const Series&`
    Required method argument.

`arg3` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `scatter` computation to `self`. Arguments: `std::int64_t, const Series&, const Series&`.

#### Examples

```cpp
auto gather_idx = pluss::make_series({
  {2,1,0},
  {0,1,2}
});
auto scatter_base = pluss::zeros({2,3}, pluss::ScalarType::Int64);
auto scatter_val = pluss::make_series({
  {1,2,3},
  {4,5,6}
});

auto out = scatter_base.scatter(1, gather_idx, scatter_val);
```

### `scatter_`

#### Signature

```cpp
Series& scatter_(self, std::int64_t, const Series&, const Series&)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `const Series&`
    Required method argument.

`arg3` : `const Series&`
    Required method argument.

#### Returns

`Series&`
    Return value produced by this method call.

#### Notes

In-place variant of `scatter`; it mutates `self` and returns `self`. Arguments: `std::int64_t, const Series&, const Series&`.

#### Examples

```cpp
auto gather_idx = pluss::make_series({
  {2,1,0},
  {0,1,2}
});
auto scatter_base = pluss::zeros({2,3}, pluss::ScalarType::Int64);
auto scatter_val = pluss::make_series({
  {1,2,3},
  {4,5,6}
});

auto t = scatter_base.clone(); t.scatter_(1, gather_idx, scatter_val);
```

### `scatter_add`

#### Signature

```cpp
Series scatter_add(self, std::int64_t, const Series&, const Series&)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `const Series&`
    Required method argument.

`arg3` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `scatter_add` computation to `self`. Arguments: `std::int64_t, const Series&, const Series&`.

#### Examples

```cpp
auto gather_idx = pluss::make_series({
  {2,1,0},
  {0,1,2}
});
auto scatter_base = pluss::zeros({2,3}, pluss::ScalarType::Int64);
auto scatter_val = pluss::make_series({
  {1,2,3},
  {4,5,6}
});

auto out = scatter_base.scatter_add(1, gather_idx, scatter_val);
```

### `scatter_add_`

#### Signature

```cpp
Series& scatter_add_(self, std::int64_t, const Series&, const Series&)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `const Series&`
    Required method argument.

`arg3` : `const Series&`
    Required method argument.

#### Returns

`Series&`
    Return value produced by this method call.

#### Notes

In-place variant of `scatter_add`; it mutates `self` and returns `self`. Arguments: `std::int64_t, const Series&, const Series&`.

#### Examples

```cpp
auto gather_idx = pluss::make_series({
  {2,1,0},
  {0,1,2}
});
auto scatter_base = pluss::zeros({2,3}, pluss::ScalarType::Int64);
auto scatter_val = pluss::make_series({
  {1,2,3},
  {4,5,6}
});

auto t = scatter_base.clone(); t.scatter_add_(1, gather_idx, scatter_val);
```

### `scatter_reduce_amax`

#### Signature

```cpp
Series scatter_reduce_amax(self, std::int64_t, const Series&, const Series&, bool)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `const Series&`
    Required method argument.

`arg3` : `const Series&`
    Required method argument.

`arg4` : `bool`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `scatter_reduce_amax` computation to `self`. Arguments: `std::int64_t, const Series&, const Series&, bool`.

#### See also

- `Series scatter_reduce_amax(self, std::int64_t, const Series&, const Series&, bool, scalar initial_scalar)`

#### Examples

```cpp
auto gather_idx = pluss::make_series({
  {2,1,0},
  {0,1,2}
});
auto scatter_base = pluss::zeros({2,3}, pluss::ScalarType::Int64);
auto scatter_val = pluss::make_series({
  {1,2,3},
  {4,5,6}
});

auto out = scatter_base.scatter_reduce_amax(1, gather_idx, scatter_val, true);
```

### `scatter_reduce_amax`

#### Signature

```cpp
Series scatter_reduce_amax(self, std::int64_t, const Series&, const Series&, bool, scalar initial_scalar)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `const Series&`
    Required method argument.

`arg3` : `const Series&`
    Required method argument.

`arg4` : `bool`
    Required method argument.

`initial_scalar` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `scatter_reduce_amax` computation to `self`. Arguments: `std::int64_t, const Series&, const Series&, bool, scalar initial_scalar`.

#### See also

- `Series scatter_reduce_amax(self, std::int64_t, const Series&, const Series&, bool)`

#### Examples

```cpp
auto gather_idx = pluss::make_series({
  {2,1,0},
  {0,1,2}
});
auto scatter_base = pluss::zeros({2,3}, pluss::ScalarType::Int64);
auto scatter_val = pluss::make_series({
  {1,2,3},
  {4,5,6}
});

auto out = scatter_base.scatter_reduce_amax(1, gather_idx, scatter_val, true, -1e9);
```

### `scatter_reduce_amin`

#### Signature

```cpp
Series scatter_reduce_amin(self, std::int64_t, const Series&, const Series&, bool)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `const Series&`
    Required method argument.

`arg3` : `const Series&`
    Required method argument.

`arg4` : `bool`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `scatter_reduce_amin` computation to `self`. Arguments: `std::int64_t, const Series&, const Series&, bool`.

#### See also

- `Series scatter_reduce_amin(self, std::int64_t, const Series&, const Series&, bool, scalar initial_scalar)`

#### Examples

```cpp
auto gather_idx = pluss::make_series({
  {2,1,0},
  {0,1,2}
});
auto scatter_base = pluss::zeros({2,3}, pluss::ScalarType::Int64);
auto scatter_val = pluss::make_series({
  {1,2,3},
  {4,5,6}
});

auto out = scatter_base.scatter_reduce_amin(1, gather_idx, scatter_val, true);
```

### `scatter_reduce_amin`

#### Signature

```cpp
Series scatter_reduce_amin(self, std::int64_t, const Series&, const Series&, bool, scalar initial_scalar)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `const Series&`
    Required method argument.

`arg3` : `const Series&`
    Required method argument.

`arg4` : `bool`
    Required method argument.

`initial_scalar` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `scatter_reduce_amin` computation to `self`. Arguments: `std::int64_t, const Series&, const Series&, bool, scalar initial_scalar`.

#### See also

- `Series scatter_reduce_amin(self, std::int64_t, const Series&, const Series&, bool)`

#### Examples

```cpp
auto gather_idx = pluss::make_series({
  {2,1,0},
  {0,1,2}
});
auto scatter_base = pluss::zeros({2,3}, pluss::ScalarType::Int64);
auto scatter_val = pluss::make_series({
  {1,2,3},
  {4,5,6}
});

auto out = scatter_base.scatter_reduce_amin(1, gather_idx, scatter_val, true, 1e9);
```

### `scatter_reduce_sum`

#### Signature

```cpp
Series scatter_reduce_sum(self, std::int64_t, const Series&, const Series&, bool)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `const Series&`
    Required method argument.

`arg3` : `const Series&`
    Required method argument.

`arg4` : `bool`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `scatter_reduce_sum` computation to `self`. Arguments: `std::int64_t, const Series&, const Series&, bool`.

#### See also

- `Series scatter_reduce_sum(self, std::int64_t, const Series&, const Series&, bool, scalar initial_scalar)`

#### Examples

```cpp
auto gather_idx = pluss::make_series({
  {2,1,0},
  {0,1,2}
});
auto scatter_base = pluss::zeros({2,3}, pluss::ScalarType::Int64);
auto scatter_val = pluss::make_series({
  {1,2,3},
  {4,5,6}
});

auto out = scatter_base.scatter_reduce_sum(1, gather_idx, scatter_val, true);
```

### `scatter_reduce_sum`

#### Signature

```cpp
Series scatter_reduce_sum(self, std::int64_t, const Series&, const Series&, bool, scalar initial_scalar)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `const Series&`
    Required method argument.

`arg3` : `const Series&`
    Required method argument.

`arg4` : `bool`
    Required method argument.

`initial_scalar` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `scatter_reduce_sum` computation to `self`. Arguments: `std::int64_t, const Series&, const Series&, bool, scalar initial_scalar`.

#### See also

- `Series scatter_reduce_sum(self, std::int64_t, const Series&, const Series&, bool)`

#### Examples

```cpp
auto gather_idx = pluss::make_series({
  {2,1,0},
  {0,1,2}
});
auto scatter_base = pluss::zeros({2,3}, pluss::ScalarType::Int64);
auto scatter_val = pluss::make_series({
  {1,2,3},
  {4,5,6}
});

auto out = scatter_base.scatter_reduce_sum(1, gather_idx, scatter_val, true, 0.0);
```

### `select`

#### Signature

```cpp
Series select(self, std::int64_t, std::int64_t)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `std::int64_t, std::int64_t`.

#### Examples

```cpp
auto mm = pluss::make_series({
  {1,2,3},
  {4,5,6},
  {7,8,9}
});

auto out = mm.select(0, 1);
```

### `sigmoid`

#### Signature

```cpp
Series sigmoid(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `sigmoid` computation to `self`. No additional arguments.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.sigmoid();
```

### `sin`

#### Signature

```cpp
Series sin(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `sin` computation to `self`. No additional arguments.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.sin();
```

### `sinh`

#### Signature

```cpp
Series sinh(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `sinh` computation to `self`. No additional arguments.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.sinh();
```

### `skewness`

#### Signature

```cpp
Series skewness(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `skewness` computation to `self`. No additional arguments.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.stat.skewness();
```

### `sqrt`

#### Signature

```cpp
Series sqrt(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `sqrt` computation to `self`. No additional arguments.

#### Examples

```cpp
auto pos = pluss::make_series({0.2,0.5,1.0,2.0,3.0,4.0,5.0});

auto out = pos.sqrt();
```

### `squeeze`

#### Signature

```cpp
Series squeeze(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. No additional arguments.

#### See also

- `Series squeeze(self, std::int64_t)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.unsqueeze(0).squeeze();
```

### `squeeze`

#### Signature

```cpp
Series squeeze(self, std::int64_t)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `std::int64_t`.

#### See also

- `Series squeeze(self)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.unsqueeze(0).squeeze(0);
```

### `standardize`

#### Signature

```cpp
Series standardize(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `standardize` computation to `self`. No additional arguments.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.standardize();
```

### `std`

#### Signature

```cpp
Series std(self, bool)
```

#### Parameters

`arg1` : `bool`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. Arguments: `bool`.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.std(false);
```

### `sub`

#### Signature

```cpp
Series sub(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `sub` computation to `self`. Arguments: `const Series&`.

#### See also

- `Series sub(self, scalar)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});
auto y = pluss::make_series({3,2,1,0,-1,-2,-3});

auto out = x.sub(y);
```

### `sub`

#### Signature

```cpp
Series sub(self, scalar)
```

#### Parameters

`arg1` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `sub` computation to `self`. Arguments: `scalar`. The current C++ overload takes `double`, but integer literals like `1` are accepted via implicit conversion.

#### See also

- `Series sub(self, const Series&)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.sub(1);
```

### `sum`

#### Signature

```cpp
Series sum(self, std::int64_t, bool)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `bool`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. Arguments: `std::int64_t, bool`.

#### Examples

```cpp
auto mm = pluss::make_series({
  {1,2,3},
  {4,5,6},
  {7,8,9}
});

auto out = mm.sum(1, false);
```

### `take`

#### Signature

```cpp
Series take(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `const Series&`.

#### Examples

```cpp
auto gather_src = pluss::make_series({
  {10,20,30},
  {40,50,60}
});
auto flat_idx = pluss::make_series({0,3,5});

auto out = gather_src.take(flat_idx);
```

### `tan`

#### Signature

```cpp
Series tan(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `tan` computation to `self`. No additional arguments.

#### Examples

```cpp
auto unit = pluss::make_series({-0.9,-0.5,-0.1,0.0,0.1,0.5,0.9});

auto out = unit.tan();
```

### `tanh`

#### Signature

```cpp
Series tanh(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `tanh` computation to `self`. No additional arguments.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.tanh();
```

### `transpose`

#### Signature

```cpp
Series transpose(self, std::int64_t, std::int64_t)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `std::int64_t, std::int64_t`.

#### Examples

```cpp
auto m = pluss::make_series({
  {1,2,3},
  {4,5,6}
});

auto out = m.transpose(0, 1);
```

### `trunc`

#### Signature

```cpp
Series trunc(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `trunc` computation to `self`. No additional arguments.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.div(2).trunc();
```

### `unique`

#### Signature

```cpp
Series unique(self, bool)
```

#### Parameters

`arg1` : `bool`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. Arguments: `bool`.

#### Examples

```cpp
auto i = pluss::make_series({1,2,3,2,1,0,3});

auto out = i.unique(true);
```

### `unsqueeze`

#### Signature

```cpp
Series unsqueeze(self, std::int64_t)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `std::int64_t`.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.unsqueeze(0);
```

### `unwrap`

#### Signature

```cpp
Series unwrap(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `unwrap` computation to `self`. No additional arguments.

#### See also

- `Series unwrap(self, scalar period)`
- `Series unwrap(self, scalar period, std::int64_t dim)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.unwrap();
```

### `unwrap`

#### Signature

```cpp
Series unwrap(self, scalar period)
```

#### Parameters

`period` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `unwrap` computation to `self`. Arguments: `scalar period`.

#### See also

- `Series unwrap(self)`
- `Series unwrap(self, scalar period, std::int64_t dim)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.unwrap(3.141592653589793);
```

### `unwrap`

#### Signature

```cpp
Series unwrap(self, scalar period, std::int64_t dim)
```

#### Parameters

`period` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

`dim` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `unwrap` computation to `self`. Arguments: `scalar period, std::int64_t dim`.

#### See also

- `Series unwrap(self)`
- `Series unwrap(self, scalar period)`

#### Examples

```cpp
auto mm = pluss::make_series({
  {1,2,3},
  {4,5,6},
  {7,8,9}
});

auto out = mm.unwrap(3.141592653589793, 1);
```

### `var`

#### Signature

```cpp
Series var(self, bool)
```

#### Parameters

`arg1` : `bool`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. Arguments: `bool`.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.var(false);
```

### `vdot`

#### Signature

```cpp
Series vdot(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `vdot` computation to `self`. Arguments: `const Series&`.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});
auto y = pluss::make_series({3,2,1,0,-1,-2,-3});

auto out = x.vdot(y);
```

### `view`

#### Signature

```cpp
Series view(self, std::span<const std::int64_t>)
```

#### Parameters

`arg1` : `std::span<const std::int64_t>`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `std::span<const std::int64_t>`.

#### Examples

```cpp
auto m = pluss::make_series({
  {1,2,3},
  {4,5,6}
});
std::vector<std::int64_t> shape_3x2{3,2};

auto out = m.view(std::span<const std::int64_t>(shape_3x2.data(), shape_3x2.size()));
```

### `view_as`

#### Signature

```cpp
Series view_as(self, const Series& other)
```

#### Parameters

`other` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `const Series& other`.

#### Examples

```cpp
auto s1 = pluss::make_series({0,1,2,3,4,5});
auto shape_ref = pluss::ones({3,2});

auto out = s1.view_as(shape_ref);
```

### `vsplit`

#### Signature

```cpp
Series vsplit(self, const std::vector<std::int64_t>& split_points)
```

#### Parameters

`split_points` : `const std::vector<std::int64_t>&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `const std::vector<std::int64_t>& split_points`.

#### See also

- `Series vsplit(self, std::int64_t sections)`

#### Examples

```cpp
auto out = pluss::make_series({
  {0,1},
  {2,3},
  {4,5},
  {6,7},
  {8,9},
  {10,11}
}).vsplit(std::vector<std::int64_t>{2,4});
```

### `vsplit`

#### Signature

```cpp
Series vsplit(self, std::int64_t sections)
```

#### Parameters

`sections` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `std::int64_t sections`.

#### See also

- `Series vsplit(self, const std::vector<std::int64_t>& split_points)`

#### Examples

```cpp
auto out = pluss::make_series({
  {0,1},
  {2,3},
  {4,5},
  {6,7},
  {8,9},
  {10,11}
}).vsplit(2);
```

### `where`

#### Signature

```cpp
Series where(self, const Series&, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

`arg2` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Selects values from `self` or fallback inputs based on a boolean condition/mask. Arguments: `const Series&, const Series&`.

#### See also

- `Series where(self, const Series&, scalar)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});
auto y = pluss::make_series({3,2,1,0,-1,-2,-3});
auto cond = x.gt(0.0);

auto out = x.where(cond, y);
```

### `where`

#### Signature

```cpp
Series where(self, const Series&, scalar)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

`arg2` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Selects values from `self` or fallback inputs based on a boolean condition/mask. Arguments: `const Series&, scalar`.

#### See also

- `Series where(self, const Series&, const Series&)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});
auto cond = x.gt(0.0);

auto out = x.where(cond, 42.0);
```

### `xlogy`

#### Signature

```cpp
Series xlogy(self, const Series& other)
```

#### Parameters

`other` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes `self * log(other)` with scalar/series overloads. Arguments: `const Series& other`.

#### See also

- `Series xlogy(self, scalar scalar)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});
auto y = pluss::make_series({3,2,1,0,-1,-2,-3});

auto out = x.abs().xlogy(y.abs().add(1.0));
```

### `xlogy`

#### Signature

```cpp
Series xlogy(self, scalar scalar)
```

#### Parameters

`scalar` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes `self * log(other)` with scalar/series overloads. Arguments: `scalar scalar`.

#### See also

- `Series xlogy(self, const Series& other)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto out = x.abs().xlogy(2.0);
```

### `xlogy_`

#### Signature

```cpp
Series& xlogy_(self, const Series& other)
```

#### Parameters

`other` : `const Series&`
    Required method argument.

#### Returns

`Series&`
    Return value produced by this method call.

#### Notes

In-place form of `xlogy`; mutates `self`. Arguments: `const Series& other`.

#### See also

- `Series& xlogy_(self, scalar scalar)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});
auto y = pluss::make_series({3,2,1,0,-1,-2,-3});

auto t = x.abs().clone(); t.xlogy_(y.abs().add(1.0));
```

### `xlogy_`

#### Signature

```cpp
Series& xlogy_(self, scalar scalar)
```

#### Parameters

`scalar` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series&`
    Return value produced by this method call.

#### Notes

In-place form of `xlogy`; mutates `self`. Arguments: `scalar scalar`.

#### See also

- `Series& xlogy_(self, const Series& other)`

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto t = x.abs().clone(); t.xlogy_(2.0);
```

### `zero_`

#### Signature

```cpp
Series& zero_(self)
```

#### Parameters

`None`

#### Returns

`Series&`
    Return value produced by this method call.

#### Notes

In-place reset: fills `self` with zeros and returns `self`. No additional arguments.

#### Examples

```cpp
auto x = pluss::make_series({-3,-2,-1,0,1,2,3});

auto t = x.clone(); t.zero_();
```
### `zeta`

#### Signature

```cpp
Series zeta(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes the Hurwitz zeta-style transform for `self` with scalar/series parameter overloads. No additional arguments.

#### See also

- `Series zeta(self, scalar)`
- `Series zeta(self, const Series&)`

#### Examples

```cpp
auto pos = pluss::make_series({0.2,0.5,1.0,2.0,3.0,4.0,5.0});

auto out = pos.zeta();
```

### `zeta`

#### Signature

```cpp
Series zeta(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes the Hurwitz zeta-style transform for `self` with scalar/series parameter overloads. Arguments: `const Series&`.

#### See also

- `Series zeta(self)`
- `Series zeta(self, scalar)`

#### Examples

```cpp
auto y = pluss::make_series({3,2,1,0,-1,-2,-3});
auto pos = pluss::make_series({0.2,0.5,1.0,2.0,3.0,4.0,5.0});

auto out = pos.zeta(y.abs().add(1.0));
```

### `zeta`

#### Signature

```cpp
Series zeta(self, scalar)
```

#### Parameters

`arg1` : `scalar`
    Numeric scalar argument. The current C++ overload takes `double`; integer literals like `1` are accepted via implicit conversion.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes the Hurwitz zeta-style transform for `self` with scalar/series parameter overloads. Arguments: `scalar`. The current C++ overload takes `double`, but integer literals like `1` are accepted via implicit conversion.

#### See also

- `Series zeta(self)`
- `Series zeta(self, const Series&)`

#### Examples

```cpp
auto pos = pluss::make_series({0.2,0.5,1.0,2.0,3.0,4.0,5.0});

auto out = pos.zeta(1.0);
```
