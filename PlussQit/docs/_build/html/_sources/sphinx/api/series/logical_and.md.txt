<!-- pluss-hand-authored -->

# Series::logical_and

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::logical_and(const Series& rhs) const -> Series
```

## What It Does

`logical_and()` compares two inputs by truth value and returns a boolean
`Series`.

The mental model is:

- each operand is interpreted as true or false at each position
- the output lane is true only when both lanes are truthy

For boolean inputs, this reads exactly like ordinary boolean conjunction. For
numeric inputs, non-zero values act as true and zero acts as false.

## Parameters

**rhs** (`const Series&`)

The right-hand truth-value operand.

Behavior that matters in practice:

- `rhs` may broadcast against `self`
- numeric values are read by truthiness, not by exact equality to `1`
- the result dtype is boolean, even when both inputs were numeric

## Returns

`Series`
    A boolean `Series` containing the lane-wise logical conjunction.

## Examples

Example 1. Boolean inputs behave exactly like ordinary lane-wise conjunction.

```cpp
auto a = pluss::make_series({
    {true, false},
    {true, false},
}, pluss::ScalarType::Bool);
auto b = pluss::make_series({
    {true, true},
    {false, false},
}, pluss::ScalarType::Bool);

std::cout << a.logical_and(b);
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[[true, false],
  [false, false]]
```

Example 2. Numeric values are first converted to truth values: zero is false
and every non-zero value is true.

```cpp
auto left = pluss::make_series({0, 2, 0, -5}, pluss::ScalarType::Int32);
auto right = pluss::make_series({1, 0, 3, 0}, pluss::ScalarType::Int32);
std::cout << left.logical_and(right);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[false, false, false, false]
```

No aligned pair has two non-zero values. The result is bool even though both
inputs are `int32`.

Example 3. A length-two condition broadcasts across the leading matrix axis.

```cpp
auto matrix = pluss::make_series({
    {true, false},
    {true, false},
}, pluss::ScalarType::Bool);
pluss::series::print(std::cout, matrix.logical_and(
    pluss::make_series({true, false}, pluss::ScalarType::Bool)));
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[[true, false],
  [true, false]]
```

## Gradient

Logical conjunction is a control and mask operation. It produces boolean
decisions from truthiness and does not carry a numeric derivative through the
truth conversion.

## Gotchas And Failure Modes

- The result is boolean, not a copy of the input dtype.
- Numeric inputs are interpreted by truthiness, so `-5`, `2`, and `1` all
  count as true.
- This is control-style logic, not a differentiable numeric transform.
- Incompatible shapes fail through normal broadcasting validation.

## Summary

Use `logical_and()` to build boolean masks that require both conditions to be
truthy.

## See Also

- [`Series::logical_or`](logical_or.md): true when either side is truthy.
- [`Series::logical_xor`](logical_xor.md): true when exactly one side is
  truthy.
- [`Series::bitwise_and`](bitwise_and.md): raw bit overlap instead of
  truth-value logic.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="logcumsumexp.html">
    <span class="label">Previous</span>
    <span class="title">logcumsumexp</span>
  </a>
  <a class="next-link" href="logical_not.html">
    <span class="label">Next</span>
    <span class="title">logical_not</span>
  </a>
</div>
