<!-- pluss-hand-authored -->

# Series::nanvar

[Back to SeriesStatAccessor API Reference](../../series_stat_accessor_api_reference)

## Signature

```cpp
Series::nanvar() const -> Series
Series::nanvar(std::int64_t dim) const -> Series
Series::nanvar(std::int64_t dim, bool keepdim) const -> Series
Series::stat.nanvar(...) -> Series
Series::stat().nanvar(...) -> Series
```

## What It Does

`nanvar()` computes variance while treating IEEE NaN values as missing
observations.

That missing-data policy is the whole point of the method. Instead of allowing
one NaN to contaminate the whole reduction lane, `nanvar()` does this:

1. discard NaN values from the lane
2. compute the mean of the remaining values
3. compute variance from that observed subset

There is no `unbiased` toggle here. `nanvar()` is always the NaN-omitting
population variance.

That means two important edge-case rules follow naturally:

- one observed value in a lane yields variance `0`
- a lane with no observed values yields `NaN`

## Overload Map

**`nanvar()`**

Global NaN-omitting population variance.

**`nanvar(dim)`**

Axis-wise NaN-omitting population variance.

**`nanvar(dim, keepdim)`**

Axis-wise NaN-omitting population variance with optional axis retention.

**`stat.nanvar(...)` / `stat().nanvar(...)`**

Accessor spellings for the same reduction family.

## Parameters

**dim** (`std::int64_t`)

The axis to reduce.

- positive dims count from the front
- negative dims count from the back
- the dim must exist

**keepdim** (`bool`, default `false`)

Whether to preserve the reduced axis as size `1`.

- `false`: remove the reduced axis
- `true`: keep it as size `1`

## Returns

`Series`
    A floating-point NaN-omitting population variance result.

## Gradient

`nanvar()` is best read as a descriptive reduction with a very specific
missing-data policy.

If NaN means "missing observation," this method is appropriate. If NaN means
"bad upstream state that should remain visible," it is the wrong surface.

## Examples

These examples are built to teach the missing-data policy explicitly, because
that policy matters more than the variance formula itself.

Example 1. Global `nanvar()` drops NaNs before computing population variance.

```cpp
auto missing = pluss::make_series({
    {1.0, std::numeric_limits<double>::quiet_NaN(), 3.0},
    {std::numeric_limits<double>::quiet_NaN(), 5.0, 7.0},
});

std::cout << missing.nanvar();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=5
```

The observed values are `[1, 3, 5, 7]`, and the NaNs are excluded from the
calculation entirely.

Example 2. `nanvar(0)` makes the one-observed-value rule very visible.

```cpp
std::cout << missing.nanvar(0);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0, 4]
```

This is one of the most important behaviors on the page:

- columns `0` and `1` each have exactly one observed value, so they return `0`
- column `2` has two observed values, so it returns a nonzero population
  variance

Example 3. `nanvar(1)` computes one NaN-omitting population variance per row.

```cpp
std::cout << missing.nanvar(1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 1]
```

Each row reduces over two observed values: `[1, 3]` and `[5, 7]`.

Example 4. `keepdim=true` preserves the reduced axis.

```cpp
std::cout << missing.stat.nanvar(1, true);
```

### Output

```text
pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[1],
  [1]]
```

Example 5. Negative dims count from the back.

```cpp
std::cout << missing.nanvar(-1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 1]
```

For a rank-2 input, `-1` means the last axis, so this matches `nanvar(1)`.

Example 6. The explicit accessor proxy gives the same result.

```cpp
std::cout << missing.stat().nanvar(1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 1]
```

Example 7. A lane containing only NaNs has no observed population, so the
result is NaN.

```cpp
auto none = pluss::make_series({
    std::numeric_limits<double>::quiet_NaN(),
    std::numeric_limits<double>::quiet_NaN(),
});

std::cout << none.nanvar();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=nan
```

Example 8. Invalid dims fail explicitly.

```cpp
try {
  std::cout << missing.nanvar(2) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
nanvar: dim out of range
```

## Gotchas And Failure Modes

- `nanvar()` always uses the population convention. There is no sample toggle.
- One observed value yields `0`, not an error.
- All-NaN lanes yield `NaN`, not `0`.
- `nanvar()` is not just a convenience alias for `var()`. It is a different
  missing-data policy.
- Invalid dims throw explicitly.

## Teaching Summary

Use `nanvar()` when NaN means "missing value that should be omitted from the
reduction."

- Choose it instead of `var()` when you want omission, not propagation.
- Expect singleton observed lanes to return `0`.
- Expect all-NaN lanes to stay undefined and return `NaN`.
- Use `keepdim=true` when later broadcasting alignment matters.

The essential lesson is simple: `nanvar()` answers "what is the population
variance of the observed values that remain?"

## See Also

- [`Series::nanstd`](nanstd.md): square root of the same NaN-omitting variance.
- [`Series::var`](var.md): ordinary variance that does not omit NaNs.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="std.html"><span class="label">Previous</span><span class="title">std</span></a>
  <a class="next-link" href="nanstd.html"><span class="label">Next</span><span class="title">nanstd</span></a>
</div>
