<!-- pluss-hand-authored -->

# Series::var

[Back to SeriesStatAccessor API Reference](../../series_stat_accessor_api_reference)

## Signature

```cpp
Series::var() const -> Series
Series::var(bool unbiased) const -> Series
Series::var(std::int64_t dim,
            bool unbiased = true,
            bool keepdim = false) const -> Series
Series::stat.var(...) -> Series
Series::stat().var(...) -> Series
```

## What It Does

`var()` measures spread around the mean.

If values cluster tightly around their average, variance is small. If they are
farther apart, variance is larger. Because variance squares deviations, it
reports spread in squared units rather than in the original units of the data.

Two choices drive almost every behavior on this page:

- do you reduce the whole `Series`, or one axis at a time?
- do you want the sample estimator or the population estimator?

The estimator choice is carried by `unbiased`:

- `unbiased=true` uses the sample convention and divides by `n - 1`
- `unbiased=false` uses the population convention and divides by `n`

That toggle changes both the numeric result and the legal edge cases. Sample
variance needs at least two values in every reduced lane. Population variance
does not.

## Overload Map

**`var()`**

Global sample variance.

**`var(bool unbiased)`**

Global variance with explicit sample vs population control.

**`var(dim, unbiased, keepdim)`**

Axis-wise variance with estimator control and optional axis retention.

**`stat.var(...)` / `stat().var(...)`**

Accessor spellings for the same reduction family.

## Parameters

**dim** (`std::int64_t`)

The axis to reduce when using the dimension overload.

- positive dims count from the front
- negative dims count from the back
- the dim must exist

Changing `dim` changes which values belong to one variance lane. On a rank-2
`Series`, `dim = 0` computes one variance per column, while `dim = 1` computes
one variance per row.

**unbiased** (`bool`, default `true`)

Controls the estimator.

- `true`: sample variance, divisor `n - 1`
- `false`: population variance, divisor `n`

This is the most important toggle on the page. It changes:

- the numeric result
- whether singleton lanes are legal
- the current global output shape in some overloads

**keepdim** (`bool`, default `false`)

Controls whether the reduced axis stays in the output shape.

- `false`: remove the reduced axis
- `true`: keep it as size `1`

This matters when the reduced result needs to stay easy to broadcast against
the original `Series`.

## Returns

`Series`
    A floating-point variance result.

    - global overloads return one-value results
    - dimension overloads return one variance per reduced lane

## Gradient

`var()` is differentiable for floating inputs.

In practice, this method is usually used as a descriptive reduction rather than
as a modeling primitive, but floating `Series` still participate in ordinary
reverse-mode paths.

## Examples

The examples below teach estimator choice first, then axis behavior, then shape
retention, accessor parity, and finally edge cases.

Example 1. With no arguments, `var()` uses the global sample estimator.

```cpp
auto values = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});

std::cout << values.var();
```

### Output

```text
pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[3.5]
```

The current implementation has a shape quirk here: the no-argument sample form
returns shape `[1]`, not shape `[]`.

Example 2. `unbiased=false` switches to the population estimator.

```cpp
std::cout << values.var(false);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=2.9166667
```

This answer is smaller because population variance divides by `n` instead of
`n - 1`. It also currently comes back as a scalar-shaped result, unlike
Example 1.

Example 3. `var(0, false)` computes one population variance per column.

```cpp
std::cout << values.var(0, false);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2.25, 2.25, 2.25]
```

Each column reduces independently.

Example 4. `var(1, false)` computes one population variance per row.

```cpp
std::cout << values.var(1, false);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.66666667, 0.66666667]
```

This is the same data as Example 3, but a different grouping rule. That is why
both shape and value change.

Example 5. `keepdim=true` preserves the reduced axis as size `1`.

```cpp
std::cout << values.var(1, true, true);
```

### Output

```text
pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[1],
  [1]]
```

This example is using the sample estimator again, so the row-wise answer is `1`
instead of the population value `0.66666667`.

Example 6. Negative dims count from the back.

```cpp
std::cout << values.var(-1, false);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.66666667, 0.66666667]
```

For a rank-2 `Series`, `-1` means the last axis, so this matches `var(1,
false)`.

Example 7. The stats accessor is just another spelling of the same operation.

```cpp
std::cout << values.stat.var(1, false);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.66666667, 0.66666667]
```

Example 8. The explicit proxy spelling is also equivalent.

```cpp
std::cout << values.stat().var(1, false);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.66666667, 0.66666667]
```

Examples 7 and 8 make it clear that the accessor forms are not different
algorithms.

Example 9. Singleton lanes are legal for the population estimator and return
zero spread.

```cpp
auto singleton_lanes = pluss::make_series(std::vector<double>{1.0, 2.0}, {2, 1});

std::cout << singleton_lanes.var(1, false);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0, 0]
```

That follows directly from the population convention: one observed value has
zero deviation from its own mean.

Example 10. The sample estimator rejects singleton lanes.

```cpp
try {
  std::cout << singleton_lanes.var(1, true) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
var(dim, unbiased): reduced dimension must have more than one element
```

This is one of the most important failure modes on the page.

Example 11. Invalid dims fail explicitly.

```cpp
try {
  std::cout << values.var(2, false) << "\n";
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

- `unbiased=true` and `unbiased=false` are not minor variants. They change the
  divisor, the result, and the legality of singleton lanes.
- Global `var()` and global `var(false)` currently differ in output shape as
  well as estimator.
- `dim = 0` and `dim = 1` answer different questions because they group
  different lanes.
- `keepdim=true` changes shape, not grouping.
- Sample variance on singleton lanes throws.
- Invalid dims throw immediately.

## Summary

Use `var()` when you want squared spread around the mean.

- Choose `unbiased=true` when you want the sample estimator.
- Choose `unbiased=false` when you want the population estimator.
- Choose `var(dim, ...)` when each row, column, or lane needs its own spread.
- Keep the axis with `keepdim=true` when downstream shape alignment matters.

The fastest way to stay out of trouble is to decide the estimator first and the
axis second. Those two choices explain almost every behavior on the page.

## See Also

- [`Series::std`](std.md): square root of variance, in the original units.
- [`Series::nanvar`](nanvar.md): NaN-omitting population variance.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="argmin.html"><span class="label">Previous</span><span class="title">argmin</span></a>
  <a class="next-link" href="std.html"><span class="label">Next</span><span class="title">std</span></a>
</div>
