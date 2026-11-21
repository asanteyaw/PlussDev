<!-- pluss-hand-authored -->

# pluss::series::pile

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
Series pluss::series::pile(std::initializer_list<Series> values, std::int64_t dim = 0)
```

## What It Builds

Stacks several series of the same shape along a **new** dimension. Piling three series of length `n` gives a `3 x n` matrix with `dim = 0`,
or an `n x 3` matrix with `dim = 1`. An existing `std::vector<Series>` can be passed in place of the braced list.

## Parameters

**values** (`std::initializer_list<Series>`)

The series to stack. They must all have the same shape and dtype.

**dim** (`std::int64_t`, default `0`)

Where the new dimension is inserted.

## Returns

`Series`
    A new series with one more dimension than the inputs.

## Errors

Inputs with different shapes are refused with `stack: shape mismatch`.

## Examples

Example 1. Stack two series into a matrix, one per row.

```cpp
auto a = pluss::make_series({1.0, 2.0, 3.0});
auto b = pluss::make_series({4.0, 5.0, 6.0});

std::cout << pluss::series::pile({a, b});
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 2, 3],
  [4, 5, 6]]
```

Example 2. Stack along the last dimension to make columns.

```cpp
auto a = pluss::make_series({1.0, 2.0, 3.0});
auto b = pluss::make_series({4.0, 5.0, 6.0});

std::cout << pluss::series::pile({a, b}, 1);
```

### Output

```text
pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 4],
  [2, 5],
  [3, 6]]
```

Example 3. Shapes must match.

```cpp
auto a = pluss::make_series({1.0, 2.0, 3.0});
auto b = pluss::make_series({4.0, 5.0});

try {
  auto out = pluss::series::pile({a, b});
} catch (const pluss::Error& e) {
  std::cout << e.message() << "\n";
}
```

### Output

```text
stack: shape mismatch
```

## Notes

- To join along an existing dimension, use [`pluss::series::unite`](unite.md).

## See Also

- [`pluss::series::unite`](unite.md): join along an existing dimension.
- [`Series::pile`](../series/pile.md): the method form.
