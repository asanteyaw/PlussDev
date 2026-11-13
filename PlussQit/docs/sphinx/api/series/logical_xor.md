<!-- pluss-hand-authored -->

# Series::logical_xor

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::logical_xor(const Series& rhs) const
Series& Series::logical_xor_(const Series& rhs) const
```

## What It Does

`logical_xor()` returns true exactly where one side is truthy and the other is
not.

This is the exclusive version of [`logical_or()`](logical_or.md): it does not
reward both sides being true at once.

## Parameters

**rhs** (`const Series&`)

The right-hand truth-value operand. It may broadcast against `self`.

Numeric inputs are interpreted by truthiness before the exclusive-or is
computed.

## Returns

`Series`
    A boolean `Series` whose lanes are true only where the aligned truth values
    differ.

## In-Place Form

`logical_xor_()` is the in-place sibling of [`logical_xor()`](logical_xor.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `logical_xor_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({true, true, false}, pluss::ScalarType::Bool);
auto y = pluss::make_series({true, false, false}, pluss::ScalarType::Bool);

x.logical_xor_(y);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=bool, platform=cpu, numel=3, contiguous=true)
data=[false, true, false]
```

## Examples

Example 1. Numeric truthiness works exactly the way boolean masks do: zero is
false, non-zero is true.

```cpp
auto a = pluss::make_series({0, 2, 0, -5}, pluss::ScalarType::Int32);
auto b = pluss::make_series({1, 0, 3, 0}, pluss::ScalarType::Int32);

pluss::series::print(std::cout, a.logical_xor(b));
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[true, true, true, true]
```

Lane by lane, each pair has exactly one truthy member:

- `(0, 1)` -> false xor true -> true
- `(2, 0)` -> true xor false -> true
- `(0, 3)` -> false xor true -> true
- `(-5, 0)` -> true xor false -> true

Example 2. Two truthy values are not exclusive, so their lane becomes false.

```cpp
auto left = pluss::make_series({0, 2, 0, -5}, pluss::ScalarType::Int32);
auto right = pluss::make_series({1, 0, 3, 0}, pluss::ScalarType::Int32);
std::cout << left.logical_xor(right);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[true, true, true, true]
```

In this specific set every pair has exactly one non-zero value. Replacing the
second pair with `(2, 1)` would make that lane false because both are truthy.

Example 3. Broadcasting uses the same shape contract as the other logical
binary methods.

```cpp
try {
  auto matrix = pluss::make_series({{true, false}, {true, false}},
                                   pluss::ScalarType::Bool);
  std::cout << matrix.logical_xor(
      pluss::make_series({true, false, true}, pluss::ScalarType::Bool));
} catch (const std::exception& error) { std::cout << error.what(); }
```

### Output

```text
broadcast: incompatible shapes
```

## Gradient

Exclusive-or creates boolean control information. It does not provide a
numeric derivative through the truth-value conversion.

## Gotchas And Failure Modes

- If both sides are true, the result is false.
- If both sides are false, the result is also false.
- Numeric inputs use truthiness, not exact `0/1` semantics.

## Summary

Use `logical_xor()` when the interesting case is disagreement between two
conditions.

## See Also

- [`Series::logical_or`](logical_or.md): true when either side is truthy.
- [`Series::logical_and`](logical_and.md): true only when both sides are
  truthy.
- [`Series::bitwise_xor`](bitwise_xor.md): bit-pattern difference instead of
  truth-value difference.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="logical_or.html">
    <span class="label">Previous</span>
    <span class="title">logical_or</span>
  </a>
  <a class="next-link" href="logit.html">
    <span class="label">Next</span>
    <span class="title">logit</span>
  </a>
</div>
