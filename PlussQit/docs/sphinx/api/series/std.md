<!-- pluss-hand-authored -->

# Series::std

[Back to Series::StatAccessor API Reference](../../series_stat_accessor_api_reference)

## Signature

```cpp
Series Series::std() const
Series Series::std(bool unbiased) const
Series Series::std(std::int64_t dim,
                   bool unbiased = true,
                   bool keepdim = false) const
Series Series::stat.std(...)
Series Series::stat().std(...)
```

## What It Does

`std()` is standard deviation: the square root of [`var()`](var.md).

That makes it the spread statistic people usually read more naturally, because
the result lives in the same units as the original values rather than in
squared units.

The same two major choices still drive behavior:

- do you reduce globally or along an axis?
- do you want the sample or population convention?

The `unbiased` toggle carries that estimator choice:

- `unbiased=true` means sample standard deviation
- `unbiased=false` means population standard deviation

So even though `std()` feels simpler to interpret than `var()`, it inherits the
same legal edge cases and most of the same shape behavior.

## Overload Map

**`std()`**

Global sample standard deviation.

**`std(bool unbiased)`**

Global standard deviation with explicit sample vs population control.

**`std(dim, unbiased, keepdim)`**

Axis-wise standard deviation with estimator control and optional axis
retention.

**`stat.std(...)` / `stat().std(...)`**

Accessor spellings for the same reduction family.

## Parameters

**dim** (`std::int64_t`)

The axis to reduce for the dimension overload.

- positive dims count from the front
- negative dims count from the back
- the dim must exist

Changing `dim` changes which values belong to one standard-deviation lane.

**unbiased** (`bool`, default `true`)

Controls the estimator.

- `true`: sample standard deviation
- `false`: population standard deviation

This changes:

- the numeric answer
- whether singleton lanes are legal
- the current global output shape in some overloads

**keepdim** (`bool`, default `false`)

Whether the reduced axis remains in the output shape.

- `false`: remove the reduced axis
- `true`: keep it as size `1`

## Returns

`Series`
    A floating-point standard deviation result.

    - global overloads return one-value results
    - dimension overloads return one value per reduced lane

## Gradient

`std()` is differentiable for floating inputs, although zero-spread lanes are
where statistical intuition and calculus intuition start to part company.

For normalization-style work, [`standardize`](standardize.md) is usually the
clearer public surface than manually chaining mean and standard deviation.

## Examples

These examples present `std()` as the readable companion to `var()`, so they
emphasize estimator choice, axis grouping, accessor parity, and the same edge
cases.

Example 1. With no arguments, `std()` uses the global sample convention.

```cpp
auto values = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});

std::cout << values.std();
```

### Output

```text
pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1.8708287]
```

Like `var()`, the current no-argument sample form returns shape `[1]`.

Example 2. `unbiased=false` switches to the population estimator.

```cpp
std::cout << values.std(false);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=1.7078251
```

This answer is smaller because the underlying divisor is `n`, not `n - 1`.

Example 3. `std(0, false)` computes one population standard deviation per
column.

```cpp
std::cout << values.std(0, false);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.5, 1.5, 1.5]
```

Example 4. `std(1, false)` computes one population standard deviation per row.

```cpp
std::cout << values.std(1, false);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.81649658, 0.81649658]
```

Again, the result changes because the grouping rule changed.

Example 5. The accessor spelling is equivalent to the direct method.

```cpp
std::cout << values.stat.std(1, false);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.81649658, 0.81649658]
```

Example 6. The explicit proxy spelling is also equivalent.

```cpp
std::cout << values.stat().std(1, false);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.81649658, 0.81649658]
```

Example 7. `keepdim=true` preserves the reduced axis as size `1`.

```cpp
std::cout << values.std(-1, false, true);
```

### Output

```text
pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[0.81649658],
  [0.81649658]]
```

For a rank-2 `Series`, `-1` means the last axis, so this is row-wise
population standard deviation with axis retention.

Example 8. Singleton lanes are legal for the population form and produce zero.

```cpp
auto singleton_lanes = pluss::make_series(std::vector<double>{1.0, 2.0}, {2, 1});

std::cout << singleton_lanes.std(1, false);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0, 0]
```

Example 9. The sample form rejects singleton lanes.

```cpp
try {
  std::cout << singleton_lanes.std(1, true) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
var(dim, unbiased): reduced dimension must have more than one element
```

This is a real current quirk: the error text is routed through the variance
check and therefore still says `var(...)`.

Example 10. Invalid dims also currently reuse variance wording.

```cpp
try {
  std::cout << values.std(2, false) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
var(dim): dim out of range
```

## Gotchas And Failure Modes

- `std()` inherits the same estimator logic as `var()`.
- Global `std()` and global `std(false)` currently differ in output shape as
  well as estimator.
- Population singleton lanes yield `0`; sample singleton lanes fail.
- Some current `std(...)` failures still report through variance wording.
- `keepdim=true` changes shape, not grouping.

## Summary

Use `std()` when you want spread in the same units as the original data.

- Choose `unbiased=true` for sample standard deviation.
- Choose `unbiased=false` for population standard deviation.
- Choose `std(dim, ...)` when each row, column, or axis lane needs its own
  spread value.
- Keep the axis with `keepdim=true` when later shape alignment matters.

If you already understand `var()`, then `std()` is mostly the same decision
tree with a more human-readable scale.

## See Also

- [`Series::var`](var.md): squared spread and the underlying estimator logic.
- [`Series::nanstd`](nanstd.md): NaN-omitting population standard deviation.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="var.html"><span class="label">Previous</span><span class="title">var</span></a>
  <a class="next-link" href="nanvar.html"><span class="label">Next</span><span class="title">nanvar</span></a>
</div>
