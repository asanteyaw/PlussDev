<!-- pluss-hand-authored -->

# Series::logical_not

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::logical_not() const
```

## What It Does

`logical_not()` flips truth values lane by lane and returns the result as a
boolean `Series`.

The public rule is simple:

- zero-like or false-like lanes become `true`
- non-zero or true-like lanes become `false`

Numeric inputs are first interpreted by truthiness:

- `0` becomes false
- any non-zero numeric value becomes true

The in-place sibling [`logical_not_()`](logical_not_.md) follows the same
truthiness rule and simply stores the result back into `self`.

## Returns

`Series`
    A boolean `Series` containing the truth-value inverse of `self`.

## Examples

Example 1. Numeric inputs are converted to truthiness first, then inverted.

```cpp
auto values = pluss::make_series({0, 2, 0, -5}, pluss::ScalarType::Int32);

pluss::series::print(std::cout, values.logical_not());
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[true, false, true, false]
```

The interpretation is:

- `0` is false, so `!0` becomes true
- `2` is true, so `!2` becomes false
- `-5` is also true, so it becomes false

Example 2. Boolean input follows the same rule without a numeric conversion
step.

```cpp
auto mask = pluss::make_series({true, false, true, false},
                               pluss::ScalarType::Bool);
std::cout << mask.logical_not();
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[false, true, false, true]
```

Example 3. The out-of-place method preserves the original Series.

```cpp
auto values = pluss::make_series({0.0, 2.0, 0.0, -5.0});
auto inverted = values.logical_not();
std::cout << values << "\n";
std::cout << inverted;
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0, 2, 0, -5]
pluss::Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[true, false, true, false]
```

## Gradient

Logical negation is a boolean mask transform. It does not define a numeric
derivative through the zero-versus-non-zero decision.

## Gotchas And Failure Modes

- The result dtype is boolean.
- Numeric truthiness is based on zero vs non-zero, not on exact equality to
  `1`.
- Because the result is boolean, integer or floating-point inputs do not keep
  their original dtype on the returned `Series`.

## Summary

Use `logical_not()` when you need a boolean inverse mask while preserving the
original `Series`.

## See Also

- [`Series::logical_not_`](logical_not_.md): same truth-value inversion, but
  written back into `self`.
- [`Series::logical_and`](logical_and.md): combine masks by conjunction.
- [`Series::bitwise_not`](bitwise_not.md): raw bit inversion instead of
  truth-value inversion.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="logical_and.html">
    <span class="label">Previous</span>
    <span class="title">logical_and</span>
  </a>
  <a class="next-link" href="logical_not_.html">
    <span class="label">Next</span>
    <span class="title">logical_not_</span>
  </a>
</div>
