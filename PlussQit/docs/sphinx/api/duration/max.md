<!-- pluss-hand-authored -->

# Duration::max

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Duration Duration::max(bool skipna = true) const;
```

## Largest Elapsed Value

`max()` performs a global reduction over all receiver lanes and returns the
largest represented duration as a rank-zero `Duration`. It compares the native
signed nanosecond values; negative durations therefore sort before zero and
positive durations without a precision-changing conversion.

By default, `skipna=true` excludes `NaT` from the candidates. Pass `false` when
the result must be absent if even one source lane is absent. Empty and
all-`NaT` receivers return scalar `NaT`, because neither contains a finite
maximum.

## Examples

Example 1. A matrix reduces to one scalar while `NaT` is skipped.

```cpp
auto values = pluss::duration::nanos(
    {5, pluss::Duration::kNaT, -2, 9}, {2,2});
std::cout << pluss::duration::print(values.max());
```

### Output

```text
Duration(shape=[], dim=0, numel=1)
0 days 00:00:00.000000009
```

Example 2. Propagating absence changes the answer, not the result shape.

```cpp
auto result = values.max(false);
std::cout << "dim=" << result.dim() << " numel=" << result.numel() << "\n";
std::cout << pluss::duration::print(result);
```

### Output

```text
dim=0 numel=1
Duration(shape=[], dim=0, numel=1)
NaT
```

Example 3. Empty input still produces a scalar reduction result.

```cpp
auto empty = pluss::duration::nanos(std::vector<std::int64_t>{});
std::cout << pluss::duration::print(empty.max());
```

### Output

```text
Duration(shape=[], dim=0, numel=1)
NaT
```

## See Also

- [`min()`](min.md): global minimum with the same option.
- [`argsort()`](argsort.md): positions that arrange a one-dimensional input.

