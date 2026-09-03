<!-- pluss-hand-authored -->

# Series::bincount

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::bincount() const -> Series
Series::bincount(std::int64_t minlength) const -> Series
```

## What It Does

`bincount()` counts how many times each non-negative integer appears.

If the input contains the labels:

`0, 2, 2, 4, 2, 1`

then the output says:

- how many zeroes occurred
- how many ones occurred
- how many twos occurred
- and so on up to the largest label, or up to `minlength`

This is a counting-by-value operation, not a histogram-by-range operation.
Exact integer equality determines the bucket.

## Overload Map

**`bincount()`**

Return exactly enough bins to cover the largest observed value.

**`bincount(std::int64_t minlength)`**

Return at least `minlength` bins, even if the observed labels do not require
that many.

## Parameters

**minlength** (`std::int64_t`)

The minimum output length.

- `0`: no extra padding beyond the observed maximum label
- positive value: pad with trailing zero-count bins if necessary

This changes output shape only. It does not change the observed counts.

## Returns

`Series`
    A 1D `int64` series of counts.

Bin `i` holds the count for input value `i`.

## Gradient

`bincount()` is not a differentiable numeric transform.

It converts integer labels into integer counts, so treat it as counting logic,
not as part of a gradient-carrying numeric path.

## Examples

Example 1. The zero-argument form creates one bin per observed label class up
to the maximum.

```cpp
auto counts_in = pluss::make_series({0, 2, 2, 4, 2, 1}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, counts_in.bincount());
```

### Output

```text
Series(shape=[5], strides=[1], dtype=int64, platform=cpu, numel=5, contiguous=true)
data=[1, 1, 3, 0, 1]
```

There is one `0`, one `1`, three `2`s, zero `3`s, and one `4`.

Example 2. `minlength` pads the output with additional zero bins.

```cpp
std::cout << counts_in.bincount(7);
```

### Output

```text
pluss::Series(shape=[7], strides=[1], dtype=int64, platform=cpu, numel=7, contiguous=true)
data=[1, 1, 3, 0, 1, 0, 0]
```

Example 3. Empty integer input is allowed. `minlength` still decides the output
size.

```cpp
std::cout << pluss::series::zeros({0}, pluss::ScalarType::Int64).bincount(4);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[0, 0, 0, 0]
```

Example 4. Negative values are rejected because bin indices must be
non-negative.

```cpp
try {
  auto bad = pluss::make_series({0, 1, -1}, pluss::ScalarType::Int64);
  std::cout << bad.bincount() << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
bincount: input must be non-negative
```

Example 5. The input must be integer-typed.

```cpp
try {
  std::cout << pluss::make_series({0.0, 1.0, 2.0}).bincount() << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
bincount: input must be integer
```

## Gotchas And Failure Modes

- `bincount()` is about exact integer values, not numeric ranges.
- Missing labels still get bins if they fall below the observed maximum.
- Negative labels are invalid.
- Floating input is invalid even if every value happens to be a whole number.

## Summary

Use `bincount()` for exact label-frequency counting.

- use `bincount()` when the natural bucket id is already the integer value
- use `minlength` when later code expects a fixed output width
- expect one output bin per label id
- move to [`histc()`](histc.md) when your buckets are numeric ranges rather
  than discrete integer labels

## See Also

- [`Series::histc`](histc.md): bucket numeric values by ranges instead of exact
  integer labels.
- [`Series::unique`](unique.md): distinct extraction without counts.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="atanh.html">
    <span class="label">Previous</span>
    <span class="title">atanh</span>
  </a>
  <a class="next-link" href="bitwise_and.html">
    <span class="label">Next</span>
    <span class="title">bitwise_and</span>
  </a>
</div>
