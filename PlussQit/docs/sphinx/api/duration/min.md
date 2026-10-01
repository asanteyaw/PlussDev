<!-- pluss-hand-authored -->

# Duration::min

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Duration Duration::min(bool skipna = true) const;
```

## Smallest Elapsed Value

`min()` reduces every lane of the receiver to one scalar `Duration`. It is a
global reduction: a matrix does not retain axes, and there is no dimension
parameter. Comparison occurs directly in signed nanoseconds, preserving full
integer precision.

`skipna=true` ignores `NaT` lanes. With `skipna=false`, any `NaT` makes the
result `NaT`, which is useful when completeness is part of the validity of the
answer. Empty input and all-`NaT` input have no finite candidate and return a
scalar `NaT`; they do not throw.

The receiver is not mutated. The result always has rank zero and one logical
element, including the no-candidate cases.

## Examples

Example 1. Reduction crosses every axis and skips `NaT` by default.

```cpp
auto values = pluss::duration::nanos(
    {5, pluss::Duration::kNaT, -2, 9}, {2,2});
std::cout << pluss::duration::print(values.min());
```

### Output

```text
Duration(shape=[], dim=0, numel=1)
-1 days 23:59:59.999999998
```

Example 2. Require every lane to be present.

```cpp
std::cout << pluss::duration::print(values.min(false));
```

### Output

```text
Duration(shape=[], dim=0, numel=1)
NaT
```

Example 3. All-`NaT` and empty inputs converge on the same no-candidate
result.

```cpp
auto all_nat = pluss::duration::nanos(
    {pluss::Duration::kNaT, pluss::Duration::kNaT});
auto empty = pluss::duration::nanos(std::vector<std::int64_t>{});

std::cout << pluss::duration::print(all_nat.min()) << "\n";
std::cout << pluss::duration::print(empty.min());
```

Both results are scalar `NaT`, not empty arrays. This makes the reduction's
return shape independent of the number of input lanes.

## See Also

- [`max()`](max.md): global maximum with the same `NaT` policy.
- [`sort()`](sort.md): retain all values in ascending order.

