<!-- pluss-hand-authored -->

# Series::logical_not_

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series& Series::logical_not_()
```

## What It Does

`logical_not_()` flips truth values in place.

It follows the same public truthiness rule as [`logical_not()`](logical_not.md):

- zero-like or false-like lanes become `true`
- non-zero or true-like lanes become `false`

The only semantic difference is mutation:

- `logical_not()` returns a new boolean `Series`
- `logical_not_()` stores that boolean result back into `self`

## Returns

`Series&`
    `self`, after its truth values have been inverted in place.

## Examples

Example 1. Boolean data flips in place.

```cpp
auto mask = pluss::make_series({true, false, true, false}, pluss::ScalarType::Bool);
mask.logical_not_();

std::cout << mask;
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[false, true, false, true]
```

Example 2. Numeric inputs are also accepted by truthiness, then rewritten as a
boolean result in place.

```cpp
auto numeric = pluss::make_series({0.0, 2.0, 0.0, -5.0});
numeric.logical_not_();

pluss::series::print(std::cout, numeric);
```

### Output

```text
Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[true, false, true, false]
```

Example 3. The return value aliases the same Series that was mutated.

```cpp
auto mask = pluss::make_series({true, false, true, false}, pluss::ScalarType::Bool);
auto& result = mask.logical_not_();
std::cout << result << "\n";
std::cout << mask;
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[false, true, false, true]
pluss::Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[false, true, false, true]
```

Both lines show the same post-mutation state. Keep this aliasing in mind when
the original Series is also referenced elsewhere.

## Gradient

In-place logical negation rewrites values as a boolean decision and is not a
numeric differentiable transform. Avoid mutating a Series needed by an active
reverse-mode graph.

## Gotchas And Failure Modes

- This is a mutation method, so it changes the original `Series`.
- On numeric input, `self` becomes boolean after the in-place rewrite.
- Truthiness is zero-vs-non-zero, not a special `0/1` convention.
- The returned `Series&` aliases `self`; it is not an independent copy.

## Summary

Use `logical_not_()` when the current `Series` itself should become the
inverted boolean mask.

## See Also

- [`Series::logical_not`](logical_not.md): out-of-place inversion with the same
  truthiness rule.
- [`Series::where_`](where_.md): another mask-driven in-place transform.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="logical_not.html">
    <span class="label">Previous</span>
    <span class="title">logical_not</span>
  </a>
  <a class="next-link" href="logical_or.html">
    <span class="label">Next</span>
    <span class="title">logical_or</span>
  </a>
</div>
