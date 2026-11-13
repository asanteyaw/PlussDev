<!-- pluss-hand-authored -->

# Series::logical_or

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::logical_or(const Series& rhs) const
Series& Series::logical_or_(const Series& rhs) const
```

## What It Does

`logical_or()` returns a boolean `Series` that is true wherever either operand
is truthy.

This is one of the core mask-building surfaces because it lets you merge
independent conditions into one combined selector.

## Parameters

**rhs** (`const Series&`)

The right-hand truth-value operand.

Important behavior:

- `rhs` may broadcast against `self`
- numeric operands are read by truthiness
- the output dtype is boolean

## Returns

`Series`
    A boolean `Series` whose lanes are true when either aligned input lane is
    truthy.

## In-Place Form

`logical_or_()` is the in-place sibling of [`logical_or()`](logical_or.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `logical_or_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({true, true, false}, pluss::ScalarType::Bool);
auto y = pluss::make_series({true, false, false}, pluss::ScalarType::Bool);

x.logical_or_(y);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=bool, platform=cpu, numel=3, contiguous=true)
data=[true, true, false]
```

## Examples

Example 1. Broadcasting lets a smaller mask expand across a larger shape.

```cpp
auto rows = pluss::make_series({
    {true, false},
    {true, false},
}, pluss::ScalarType::Bool);
auto cols = pluss::make_series({true, false}, pluss::ScalarType::Bool);

std::cout << rows.logical_or(cols);
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[[true, false],
  [true, false]]
```

The 1-D mask broadcasts across both rows, so each row is combined with the same
column-wise truth pattern.

Example 2. Numeric truthiness does not require values to be exactly `1`.

```cpp
auto left = pluss::make_series({0, 2, 0, -5}, pluss::ScalarType::Int32);
auto right = pluss::make_series({1, 0, 3, 0}, pluss::ScalarType::Int32);
std::cout << left.logical_or(right);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[true, true, true, true]
```

Each pair has at least one non-zero member.

Example 3. Shapes that cannot broadcast are rejected rather than guessed.

```cpp
try {
  std::cout << rows.logical_or(
      pluss::make_series({true, false, true}, pluss::ScalarType::Bool));
} catch (const std::exception& error) { std::cout << error.what(); }
```

### Output

```text
broadcast: incompatible shapes
```

## Gradient

`logical_or()` is a boolean decision surface, so it is suitable for building
masks but not for carrying a numeric derivative through truthiness.

## Gotchas And Failure Modes

- The result is boolean even when the inputs were numeric.
- Truthiness is based on zero vs non-zero, not on equality to `1`.
- Shape mistakes still fail through normal broadcasting rules.

## Summary

Use `logical_or()` to merge conditions when any satisfied branch should count
as a match.

## See Also

- [`Series::logical_and`](logical_and.md): require both conditions.
- [`Series::logical_xor`](logical_xor.md): require exactly one truthy side.
- [`Series::where`](where.md): consume a mask after you have built it.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="logical_not_.html">
    <span class="label">Previous</span>
    <span class="title">logical_not_</span>
  </a>
  <a class="next-link" href="logical_xor.html">
    <span class="label">Next</span>
    <span class="title">logical_xor</span>
  </a>
</div>
