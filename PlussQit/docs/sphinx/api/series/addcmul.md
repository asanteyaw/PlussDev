<!-- pluss-hand-authored -->

# Series::addcmul

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::addcmul(const Series& tensor1, const Series& tensor2,
                double value = 1.0) const -> Series
Series& Series::addcmul_(const Series& tensor1, const Series& tensor2, double value=1.0) const
```

## What It Does

`addcmul()` is the fused **elementwise** multiply-accumulate:

```
addcmul(input, tensor1, tensor2, value) = input + value * (tensor1 * tensor2)
```

Every operand is combined elementwise under broadcasting — there is no matrix
product here, unlike the [`addmm()`](addmm.md) family. It is the pattern that
appears inside optimizer updates (`param += -lr * grad`, a bias-corrected
moment step, and so on): one call instead of a temporary product and a separate
add.

The division counterpart is [`addcdiv()`](addcdiv.md).

## Broadcasting The Three Inputs

Broadcasting is resolved across all three operands. A receiver `[r,1]`, first
factor `[1,c]`, and second factor `[r,c]` produce `[r,c]`. During reverse
propagation, sensitivity is summed over every axis an input had broadcast, so
row and column parameters retain their original shapes.

## Parameters

**tensor1**, **tensor2** (`const Series&`)

The two factors. They broadcast against each other and against `input`.

**value** (`double`, default `1.0`)

Scalar multiplier on the `tensor1 * tensor2` product.

## Returns

`Series`
    `input + value * (tensor1 * tensor2)`, at the broadcasted shape.

## Gradient

`addcmul()` is differentiable in all three tensors:

```
d/d(input)   = g
d/d(tensor1) = value * g * tensor2
d/d(tensor2) = value * g * tensor1
```

## In-Place Form

`addcmul_()` is the in-place sibling of [`addcmul()`](addcmul.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `addcmul_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({1.0, 1.0, 1.0});
auto t1 = pluss::make_series({2.0, 3.0, 4.0});
auto t2 = pluss::make_series({1.0, 1.0, 1.0});

x.addcmul_(t1, t2, 1.0);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[3, 4, 5]
```

## Examples

Example 1. `input + 2 * (t1 * t2)`.

```cpp
auto base = pluss::make_series({1.0, 1.0, 1.0});
auto t1 = pluss::make_series({2.0, 3.0, 4.0});
auto t2 = pluss::make_series({5.0, 6.0, 7.0});

std::cout << base.addcmul(t1, t2, 2.0);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[21, 37, 57]
```

`1 + 2 * [10, 18, 28]`.

Example 2. The gradient with respect to `tensor1` is `value * g * tensor2`.

```cpp
auto base = pluss::make_series({1.0, 1.0, 1.0});
auto t1 = pluss::make_series({2.0, 3.0, 4.0});
auto t2 = pluss::make_series({5.0, 6.0, 7.0});
t1.tracks_slope_(true);

base.addcmul(t1, t2, 2.0).sum().reverse();

std::cout << "dL/dt1=" << t1.sensitivity();
```

### Output

```text
dL/dt1=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[10, 12, 14]
```

With `g = ones`, `dL/dt1 = 2 * t2 = [10, 12, 14]`.

Example 3. Row and column factors form a complete update grid.

```cpp
auto base = pluss::make_series(1.0);
auto column = pluss::make_series({{2.0}, {3.0}});
auto row = pluss::make_series({10.0, 20.0, 30.0});
std::cout << base.addcmul(column, row, 0.5);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[11, 21, 31],
  [16, 31, 46]]
```

## Gotchas And Failure Modes

- This is elementwise, not a matrix product. For the matrix case use
  [`addmm()`](addmm.md).
- `value` is a scalar, not a `Series`.
- All three operands participate in broadcasting.

## Summary

`addcmul(tensor1, tensor2, value)` fuses an elementwise multiply and an add:

- `input + value * (tensor1 * tensor2)`
- differentiable in all three tensors
- division form: [`addcdiv()`](addcdiv.md)

## See Also

- [`Series::addcdiv`](addcdiv.md): the same with `/` instead of `*`.
- [`Series::addmm`](addmm.md): the fused *matrix* multiply-accumulate.
- [`Series::lerp`](lerp.md): the related fused linear interpolation.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="addcdiv.html">
    <span class="label">Previous</span>
    <span class="title">addcdiv</span>
  </a>
  <a class="next-link" href="addmm.html">
    <span class="label">Next</span>
    <span class="title">addmm</span>
  </a>
</div>
