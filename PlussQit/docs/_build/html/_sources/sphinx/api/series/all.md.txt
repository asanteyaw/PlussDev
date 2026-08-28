<!-- pluss-hand-authored -->

# Series::all

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::all() const -> Series
Series::all(std::int64_t dim) const -> Series
Series::all(std::int64_t dim, bool keepdim) const -> Series
```

## What It Does

`all()` performs a boolean reduction.

It answers one question:

"Are all values in this reduction lane truthy?"

That lane can be the whole `Series`, or one chosen axis. For numeric inputs,
the values are interpreted as booleans first:

- zero becomes false
- non-zero becomes true

So `all()` is about truthiness, not numeric equality to `1`.

## Overload Map

**`all()`**

Reduces the whole `Series` to one scalar boolean answer.

**`all(dim)`**

Reduces one axis and drops that axis.

**`all(dim, keepdim)`**

Reduces one axis but keeps it as size `1`.

## Parameters

**dim** (`std::int64_t`)

The axis to reduce.

- positive dims count from the front
- negative dims count from the back
- the dim must refer to a real axis

This parameter decides which values are tested together.

**keepdim** (`bool`)

Controls whether the reduced axis is removed or retained as size `1`.

- `false`: remove the axis
- `true`: keep a singleton axis

This changes shape only. It does not change which lanes evaluate to true or
false.

## Returns

`Series`
    A boolean `Series`.

## Gradient

`all()` is a boolean reduction, not a differentiable numeric transform.

Treat it as control information for branching, validation, or mask logic.

## Examples

Example 1. The zero-argument overload reduces the whole `Series`.

```cpp
auto truth = pluss::make_series({
    {1.0, 1.0, 0.0},
    {2.0, -3.0, 4.0},
});

pluss::series::print(std::cout, truth.all());
```

### Output

```text
Series(shape=[], strides=[], dtype=bool, platform=cpu, numel=1, contiguous=true)
data=false
```

Example 2. `all(0)` asks whether every row agrees column by column.

```cpp
std::cout << truth.all(0);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=bool, platform=cpu, numel=3, contiguous=true)
data=[true, true, false]
```

Example 3. `keepdim=true` preserves the reduced axis as a singleton.

```cpp
std::cout << truth.all(1, true);
```

### Output

```text
pluss::Series(shape=[2,1], strides=[1,1], dtype=bool, platform=cpu, numel=2, contiguous=true)
data=[[false],
  [true]]
```

Example 4. Negative dims are normalized from the back.

```cpp
std::cout << truth.all(-1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=bool, platform=cpu, numel=2, contiguous=true)
data=[false, true]
```

Example 5. A scalar numeric zero becomes boolean false.

```cpp
std::cout << pluss::make_series(0.0).all();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=bool, platform=cpu, numel=1, contiguous=true)
data=false
```

Example 6. Out-of-range dimensions fail explicitly.

```cpp
try {
  std::cout << truth.all(2, false) << "\n";
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

- Numeric `all()` is a truthiness reduction, not a "value equals one"
  reduction.
- `keepdim` changes shape, not truth values.
- Scalar answers come back as rank-0 boolean `Series`, not plain C++ `bool`.
- Negative dims are supported, but they still have to land on a real axis after
  normalization.

## Summary

Use `all()` when every value in a lane must pass.

- choose `all()` for one global answer
- choose `all(dim)` for per-axis answers
- choose `keepdim=true` when later broadcasting or alignment will be easier

## See Also

- [`Series::any`](any.md): lane-wise "at least one passes" reduction.
- [`Series::where`](where.md): use masks and reduction results in branch
  selection.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="add.html">
    <span class="label">Previous</span>
    <span class="title">add</span>
  </a>
  <a class="next-link" href="any.html">
    <span class="label">Next</span>
    <span class="title">any</span>
  </a>
</div>
