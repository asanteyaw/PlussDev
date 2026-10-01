<!-- pluss-hand-authored -->

# Series::addcdiv

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::addcdiv(const Series& tensor1, const Series& tensor2,
                double value = 1.0) const -> Series
Series& Series::addcdiv_(const Series& tensor1, const Series& tensor2, double value=1.0) const
```

## What It Does

`addcdiv()` is [`addcmul()`](addcmul.md) with a division:

```
addcdiv(input, tensor1, tensor2, value) = input + value * (tensor1 / tensor2)
```

All operands combine elementwise under broadcasting. It is the shape of an
Adam-style parameter step, where a moment is divided by a running
root-mean-square. See [`addcmul()`](addcmul.md) for broadcasting and the
gradient contract; the divisor rule is the quotient rule scaled by `value`.

## Reading The Denominator

The denominator is applied after broadcasting. A denominator row can scale
every row of a table, and a scalar denominator can scale the entire update.
Small magnitudes amplify both the forward update and denominator sensitivity;
zero follows floating-point division and produces non-finite values rather
than an exception.

## Parameters

**tensor1** (`const Series&`) — the numerator.

**tensor2** (`const Series&`) — the denominator.

**value** (`double`, default `1.0`) — scalar multiplier on the quotient.

## Returns

`Series`
    `input + value * (tensor1 / tensor2)`, at the broadcasted shape.

## Gradient

Differentiable in all three tensors:

```
d/d(input)   = g
d/d(tensor1) = value * g / tensor2
d/d(tensor2) = -value * g * tensor1 / tensor2^2
```

## In-Place Form

`addcdiv_()` is the in-place sibling of [`addcdiv()`](addcdiv.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `addcdiv_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({1.0, 1.0, 1.0});
auto t1 = pluss::make_series({2.0, 3.0, 4.0});
auto t2 = pluss::make_series({2.0, 3.0, 4.0});

x.addcdiv_(t1, t2, 1.0);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2, 2, 2]
```

## Example

```cpp
auto base = pluss::make_series({1.0, 1.0, 1.0});
auto t1 = pluss::make_series({2.0, 3.0, 4.0});
auto t2 = pluss::make_series({5.0, 6.0, 7.0});

std::cout << base.addcdiv(t1, t2);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.4, 1.5, 1.5714286]
```

`1 + [0.4, 0.5, 0.5714...]`.

Example 2. A denominator row broadcasts across a matrix update.

```cpp
auto base = pluss::make_series({{1.0}, {10.0}});
auto numer = pluss::make_series({{2.0, 4.0}, {6.0, 8.0}});
auto denom = pluss::make_series({2.0, 4.0});
std::cout << base.addcdiv(numer, denom, 2.0);
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[3, 3],
  [16, 14]]
```

The quotient is `[[1,1],[3,2]]`; doubling it and adding the broadcast receiver
column gives the displayed matrix.

## Gotchas And Failure Modes

- Elementwise, not a matrix operation.
- Division by zero yields `inf` / `nan`, propagated, not an error.
- `value` is a scalar.

## Summary

`addcdiv(tensor1, tensor2, value)` fuses an elementwise divide and an add:
`input + value * (tensor1 / tensor2)`, differentiable in all three tensors.

## See Also

- [`Series::addcmul`](addcmul.md): the multiply form and shared contract.
- [`Series::div`](div.md): the bare elementwise quotient.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="addbmm.html">
    <span class="label">Previous</span>
    <span class="title">addbmm</span>
  </a>
  <a class="next-link" href="addcmul.html">
    <span class="label">Next</span>
    <span class="title">addcmul</span>
  </a>
</div>
