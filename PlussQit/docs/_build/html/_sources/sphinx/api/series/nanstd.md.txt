<!-- pluss-hand-authored -->

# Series::nanstd

[Back to Series::StatAccessor API Reference](../../series_stat_accessor_api_reference)

## Signature

```cpp
Series::nanstd() const -> Series
Series::nanstd(std::int64_t dim) const -> Series
Series::nanstd(std::int64_t dim, bool keepdim) const -> Series
Series::stat.nanstd(...) -> Series
Series::stat().nanstd(...) -> Series
```

## What It Does

`nanstd()` is the standard-deviation companion to [`nanvar()`](nanvar.md).

It keeps the same missing-data rule:

- ignore NaNs
- compute over the observed values that remain
- use the population convention

The main difference is interpretability. `nanstd()` reports spread in the same
units as the input, so it is often easier to read directly than `nanvar()`.

Like `nanvar()`, there is no `unbiased` toggle here. This method always uses
the NaN-omitting population convention.

## Overload Map

**`nanstd()`**

Global NaN-omitting population standard deviation.

**`nanstd(dim)`**

Axis-wise NaN-omitting population standard deviation.

**`nanstd(dim, keepdim)`**

Axis-wise NaN-omitting population standard deviation with optional axis
retention.

**`stat.nanstd(...)` / `stat().nanstd(...)`**

Accessor spellings for the same reduction family.

## Parameters

**dim** (`std::int64_t`)

The axis to reduce.

- positive dims count from the front
- negative dims count from the back
- the dim must exist

**keepdim** (`bool`, default `false`)

Whether the reduced axis stays in the result shape as size `1`.

## Returns

`Series`
    A floating-point NaN-omitting population standard deviation result.

## Gradient

`nanstd()` is primarily a descriptive reduction with explicit missing-data
policy.

Use it when omission is semantically correct, not because you want NaNs to
quietly disappear from every analysis step.

## Examples

These examples deliberately contrast `nanstd()` with ordinary `std()` so the
policy difference is impossible to miss.

Example 1. Global `nanstd()` ignores NaNs and computes over the observed
population.

```cpp
auto missing = pluss::make_series({
    {1.0, std::numeric_limits<double>::quiet_NaN(), 3.0},
    {std::numeric_limits<double>::quiet_NaN(), 5.0, 7.0},
});

std::cout << missing.nanstd();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=2.236068
```

Example 2. `nanstd(0)` makes the one-observed-value rule visible in standard
deviation form.

```cpp
std::cout << missing.nanstd(0);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0, 2]
```

Columns `0` and `1` each have one observed value, so their standard deviation
is `0`.

Example 3. `nanstd(1)` computes one NaN-omitting population standard deviation
per row.

```cpp
std::cout << missing.nanstd(1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 1]
```

Example 4. `keepdim=true` preserves the reduced axis.

```cpp
std::cout << missing.stat.nanstd(-1, true);
```

### Output

```text
pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[1],
  [1]]
```

For rank `2`, `-1` means the last axis, so this is row-wise `nanstd()` with
axis retention.

Example 5. The explicit proxy spelling gives the same result.

```cpp
std::cout << missing.stat().nanstd(1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 1]
```

Example 6. `std()` and `nanstd()` answer different questions on the same data.

```cpp
std::cout << missing.std(1, false) << "\n";
std::cout << missing.nanstd(1) << "\n";
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[nan, nan]
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 1]
```

This contrast is the heart of the API split:

- `std()` propagates NaNs
- `nanstd()` omits them

Example 7. An all-NaN lane still has no observed spread, so the result is NaN.

```cpp
auto none = pluss::make_series({
    std::numeric_limits<double>::quiet_NaN(),
    std::numeric_limits<double>::quiet_NaN(),
});

std::cout << none.nanstd();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=nan
```

Example 8. Invalid dims fail explicitly, but the current message reuses
`nanvar` wording.

```cpp
try {
  std::cout << missing.nanstd(2) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
nanvar: dim out of range
```

That is a real current quirk worth documenting honestly.

## Gotchas And Failure Modes

- `nanstd()` always uses the population convention.
- One observed value yields `0`.
- All-NaN lanes yield `NaN`.
- `nanstd()` and `std()` differ by policy, not just convenience.
- Some current `nanstd(...)` dim failures report through `nanvar` wording.

## Summary

Use `nanstd()` when NaN means "missing value that should be ignored during the
reduction" and you want spread in the original units.

- Choose it instead of `std()` when omission is the correct policy.
- Expect singleton observed lanes to return `0`.
- Expect all-NaN lanes to remain undefined and return `NaN`.
- Keep the axis with `keepdim=true` when downstream shape alignment matters.

The key question to ask is not "do I want standard deviation?" but rather "what
should NaN mean in this analysis?"

## See Also

- [`Series::nanvar`](nanvar.md): squared spread with the same omission rule.
- [`Series::std`](std.md): ordinary standard deviation that propagates NaNs.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nanvar.html"><span class="label">Previous</span><span class="title">nanvar</span></a>
  <a class="next-link" href="nansum.html"><span class="label">Next</span><span class="title">nansum</span></a>
</div>
