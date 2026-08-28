<!-- pluss-hand-authored -->

# Series::any

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::any() const -> Series
Series::any(std::int64_t dim) const -> Series
Series::any(std::int64_t dim, bool keepdim) const -> Series
```

## What It Does

`any()` is the boolean sibling of [`all()`](all.md).

It answers:

"Is at least one value in this reduction lane truthy?"

As with `all()`, numeric inputs are interpreted as booleans first:

- zero becomes false
- non-zero becomes true

So `any()` is the right reduction when one passing element is enough.

## Overload Map

**`any()`**

Reduces the whole `Series` to one scalar boolean answer.

**`any(dim)`**

Reduces one axis and drops that axis.

**`any(dim, keepdim)`**

Reduces one axis and keeps it as size `1`.

## Parameters

**dim** (`std::int64_t`)

The axis to reduce.

- positive dims count from the front
- negative dims count from the back
- the dim must exist

This decides which values are grouped together for the truth test.

**keepdim** (`bool`)

Whether to remove the reduced axis or keep it as a singleton axis.

- `false`: smaller rank
- `true`: same rank with size `1` on the reduced axis

This affects output geometry, not the truth rule itself.

## Returns

`Series`
    A boolean `Series`.

## Gradient

`any()` is a boolean reduction and should be treated as non-differentiable
control information.

## Examples

Example 1. The global overload returns one answer for the whole input.

```cpp
auto sparse = pluss::make_series({
    {0.0, 0.0, 0.0},
    {0.0, 5.0, 0.0},
});

pluss::series::print(std::cout, sparse.any());
```

### Output

```text
Series(shape=[], strides=[], dtype=bool, platform=cpu, numel=1, contiguous=true)
data=true
```

Example 2. `any(0)` tests column by column.

```cpp
std::cout << sparse.any(0);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=bool, platform=cpu, numel=3, contiguous=true)
data=[false, true, false]
```

Example 3. `keepdim=true` preserves the reduced axis.

```cpp
std::cout << sparse.any(1, true);
```

### Output

```text
pluss::Series(shape=[2,1], strides=[1,1], dtype=bool, platform=cpu, numel=2, contiguous=true)
data=[[false],
  [true]]
```

Example 4. Negative dims work the same after normalization.

```cpp
std::cout << sparse.any(-1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=bool, platform=cpu, numel=2, contiguous=true)
data=[false, true]
```

Example 5. A scalar zero stays false after boolean conversion.

```cpp
std::cout << pluss::make_series(0.0).any();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=bool, platform=cpu, numel=1, contiguous=true)
data=false
```

Example 6. Invalid reduction axes fail explicitly.

```cpp
try {
  std::cout << sparse.any(2, false) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
reduce: dim out of range
```

## Gotchas And Failure Modes

- Numeric inputs are interpreted through truthiness, not compared against a
  special threshold other than zero.
- `any()` returns a boolean `Series`, even for scalar answers.
- `keepdim=true` is a shape-management toggle, not a semantic toggle.
- Out-of-range dims fail; they do not clamp.

## Summary

Use `any()` when one truthy value is enough.

- reach for it in mask validation and guard logic
- use the global overload for one scalar decision
- use the axis overloads when each lane needs its own pass/fail answer

## See Also

- [`Series::all`](all.md): every-value counterpart.
- [`Series::where`](where.md): act on the masks or reduced truth results you
  build with `any()`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="all.html">
    <span class="label">Previous</span>
    <span class="title">all</span>
  </a>
  <a class="next-link" href="argmax.html">
    <span class="label">Next</span>
    <span class="title">argmax</span>
  </a>
</div>
