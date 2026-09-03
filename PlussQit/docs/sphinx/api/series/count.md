<!-- pluss-hand-authored -->

# Series::count

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::count(bool count_true = true) const -> Series
```

## What It Does

`count()` returns a one-element `int64` `Series` that counts how many elements
match the requested truth branch.

The branch behavior depends on dtype:

- for `bool` series, `count(true)` counts `true` values and `count(false)`
  counts `false` values
- for non-`bool` series, `count()` means "how many elements exist"

That second rule matters. On numeric data, this is not a nonzero-count
operation. It is a total-element count.

## Parameters

**count_true** (`bool`)

Selects which boolean lane to count when the series dtype is `bool`.

- `true`: count `true` values
- `false`: count `false` values

On non-boolean series, only `count_true = true` is valid, because there is no
boolean false-lane notion to count there.

## Returns

`Series`
    A one-element `int64` `Series`.

This follows the rest of the `Series` reduction surface: even scalar answers
stay in `Series` form unless the API explicitly chooses a plain C++ scalar.

## Gradient

`count()` is not a differentiable numeric transform.

It is a counting/control operation.

## Examples

Example 1. On a boolean mask, the default branch counts `true` values.

```cpp
auto mask = pluss::make_series({true, false, true, true, false}, pluss::ScalarType::Bool);
std::cout << "true_count=" << mask.count() << "\n";
```

### Output

```text
true_count=pluss::Series(shape=[1], strides=[1], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=[3]
```

Example 2. Passing `false` switches the boolean branch and counts `false`
values instead.

```cpp
std::cout << "false_count=" << mask.count(false) << "\n";
```

### Output

```text
false_count=pluss::Series(shape=[1], strides=[1], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=[2]
```

Examples 1 and 2 are the important toggle pair on this method.

Example 3. On numeric data, `count()` counts total elements, not nonzero
elements.

```cpp
auto numeric = pluss::make_series({
    {5.0, 0.0, -2.0},
    {8.0, 9.0, 10.0},
});

pluss::series::print(std::cout, numeric.count());
```

### Output

```text
Series(shape=[1], strides=[1], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=[6]
```

The zero value inside the series does not reduce the count. Every element
position is included.

Example 4. `count(false)` is invalid on non-boolean data.

```cpp
try {
  std::cout << numeric.count(false) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Series::count(false) is only valid for Bool Series
```

## Gotchas And Failure Modes

- On numeric series, `count()` counts total elements, not nonzero elements.
- `count(false)` is only meaningful for `bool` series and fails on other dtypes.
- The result is a one-element `Series`, not a plain `std::int64_t`.

## Summary

Use `count()` when you need cardinality in `Series` form:

- on `bool`, it counts `true` or `false` lanes depending on the toggle
- on numeric data, it counts total elements
- if you need a plain C++ scalar, extract it from the one-element result

## See Also

- [`Series::bincount`](bincount.md): value-frequency counting by integer label.
- [`Series::histc`](histc.md): range-bucket counting.
- [`Series::nonzero`](nonzero.md): explicit positions instead of counts.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="copysign.html">
    <span class="label">Previous</span>
    <span class="title">copysign</span>
  </a>
  <a class="next-link" href="cos.html">
    <span class="label">Next</span>
    <span class="title">cos</span>
  </a>
</div>
