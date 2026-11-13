<!-- pluss-hand-authored -->

# Series::isin

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::isin(const Series& test_elements) const
Series Series::isin(const Series& test_elements, bool assume_unique) const
Series Series::isin(const Series& test_elements, bool assume_unique, bool invert) const
```

## What It Does

`isin()` tests membership element by element.

For each value in `self`, it asks:

"Does this value appear in `test_elements`?"

The answer comes back as a boolean series shaped like `self`.

That makes `isin()` a clean bridge between:

- set-like membership questions
- boolean masks
- later filtering, `where()`, or counting logic

## Parameters

**test_elements** (`const Series&`)

The values that define membership.

Every element in `self` is tested against this collection. The two inputs must
live on the same platform and have the same dtype.

**assume_unique** (`bool`)

Tells `isin()` whether it should assume `test_elements` is already distinct.

- `false`: deduplicate `test_elements` internally before membership work
- `true`: trust the caller and skip that extra deduplication step

This toggle is mainly about work, not semantics. If the assumption is true,
the boolean answers are the same.

**invert** (`bool`)

Flips the meaning of the membership test.

- `false`: `true` means "is present in `test_elements`"
- `true`: `true` means "is not present in `test_elements`"

This is the direct toggle between inclusion mask and exclusion mask.

## Returns

`Series`
    A boolean series shaped like `self`.

Higher-rank inputs keep their shape. `isin()` tests elementwise and then
reshapes the mask back to match the original layout.

## Gradient

`isin()` is not a differentiable numeric transform.

It produces a boolean membership mask, so treat it as control information.

## Examples

Example 1. The base form answers "which of these values belong to that set?"

```cpp
auto members = pluss::make_series({10, 30, 10, 50}, pluss::ScalarType::Int64);
auto universe = pluss::make_series({30, 40, 50}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, members.isin(universe));
```

### Output

```text
Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[false, true, false, true]
```

Example 2. `invert=true` turns inclusion into exclusion.

```cpp
std::cout << members.isin(universe, false, true);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[true, false, true, false]
```

Example 3. Higher-rank input keeps its original shape.

```cpp
auto matrix_members = pluss::make_series({
    {1, 2, 5},
    {7, 2, 9},
}, pluss::ScalarType::Int64);

std::cout << matrix_members.isin(
    pluss::make_series({2, 9}, pluss::ScalarType::Int64));
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=[[false, true, false],
  [false, true, true]]
```

Example 4. Dtype mismatch is not allowed, even if the numeric values look
compatible to a human reader.

```cpp
try {
  auto ints = pluss::make_series({1, 2, 3}, pluss::ScalarType::Int64);
  auto floats = pluss::make_series({1.0, 3.0});
  std::cout << ints.isin(floats) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
isin: elements and test_elements must have the same dtype
```

## Gotchas And Failure Modes

- `isin()` is elementwise; it does not compare rows or larger slices as units.
- `assume_unique` is a performance/work toggle, not a meaning toggle.
- `invert=true` flips the mask after membership, so read the result as
  "not in".
- Dtype equality is strict.

## Summary

Use `isin()` when you want a mask from a membership question.

- pass the candidate set as `test_elements`
- keep `assume_unique=false` unless you already know the candidate set is
  distinct
- turn on `invert` when you want the complement mask
- expect the output shape to mirror `self`

## See Also

- [`Series::unique`](unique.md): build a distinct candidate set first.
- [`Series::where`](where.md): consume the resulting boolean mask.
- [`Series::nonzero`](nonzero.md): convert the boolean mask into explicit
  coordinates if needed.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isfinite.html">
    <span class="label">Previous</span>
    <span class="title">isfinite</span>
  </a>
  <a class="next-link" href="isinf.html">
    <span class="label">Next</span>
    <span class="title">isinf</span>
  </a>
</div>
