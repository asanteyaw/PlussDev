<!-- pluss-hand-authored -->

# pluss::series::unite

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
Series pluss::series::unite(std::initializer_list<Series> values, std::int64_t dim = 0, bool strict_dtype = true)
```

## What It Builds

Joins several series end to end along an existing dimension. The series must agree in every other dimension. An existing
`std::vector<Series>` can be passed in place of the braced list.

## Parameters

**values** (`std::initializer_list<Series>`)

The series to join, in order.

**dim** (`std::int64_t`, default `0`)

The dimension to join along. The result is as long along `dim` as all the inputs together.

**strict_dtype** (`bool`, default `true`)

When `true`, inputs with different dtypes are refused. When `false`, they are promoted to a common dtype.

## Returns

`Series`
    A new series.

## Errors

Mixed dtypes with `strict_dtype = true` are refused with `unite: dtype mismatch (set strict_dtype=false to enable promotion/coercion)`.

## Examples

Example 1. Join two series end to end.

```cpp
auto a = pluss::make_series({1.0, 2.0});
auto b = pluss::make_series({3.0, 4.0, 5.0});

std::cout << pluss::series::unite({a, b});
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1, 2, 3, 4, 5]
```

Example 2. Join matrices along a chosen dimension.

```cpp
auto m = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {2, 2});

std::cout << pluss::series::unite({m, m}, 0);
std::cout << pluss::series::unite({m, m}, 1);
```

### Output

```text
pluss::Series(shape=[4,2], strides=[2,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[1, 2],
  [3, 4],
  [1, 2],
  [3, 4]]pluss::Series(shape=[2,4], strides=[4,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[1, 2, 1, 2],
  [3, 4, 3, 4]]
```

Example 3. Different dtypes are refused unless `strict_dtype` is false.

```cpp
auto f = pluss::make_series({1.0, 2.0});
auto i = pluss::make_series(std::vector<std::int64_t>{3, 4}, {2});

try {
  auto out = pluss::series::unite({f, i});
} catch (const pluss::Error& e) {
  std::cout << e.message() << "\n";
}
std::cout << pluss::series::unite({f, i}, 0, false);
```

### Output

```text
unite: dtype mismatch (set strict_dtype=false to enable promotion/coercion)
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 2, 3, 4]
```

## Notes

- To stack series along a new dimension instead, use [`pluss::series::pile`](pile.md).

## See Also

- [`pluss::series::pile`](pile.md): stack along a new dimension.
- [`Series::unite`](../series/unite.md): the method form.
