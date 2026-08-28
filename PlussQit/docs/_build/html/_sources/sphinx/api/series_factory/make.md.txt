<!-- pluss-hand-authored -->

# pluss::series::make

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
pluss::series::make(data, option0 = {}, option1, option2, ...) -> Series
```

## What It Builds

`make()` is the value-first constructor for `Series`.

This is the factory to reach for when the values already exist and you want
PlussQit to infer or honor the shape from those values:

- scalar literals
- 1D initializer lists
- nested initializer lists
- flat host buffers with an explicit shape option
- host data plus dtype/config/autodiff options

Where the shape-first factories start from "how big should the output be?",
`make()` starts from "what values do I already have?"

## How The Options Work

The first argument is consumed through `DataReader`, and the remaining arguments
act like a compact option stream.

The important option categories are:

- shape override for flat host data
- dtype override
- config override
- autodiff/storage options

That means one public factory name covers a broad practical surface without
requiring half a dozen separate constructor families.

## Returns

`Series`
    A newly materialized series whose shape and dtype come from the input data
    plus any explicit overrides you supply.

## Examples

Example 1. A floating scalar becomes a scalar `Series`.

```cpp
std::cout << pluss::series::make(42.5);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=42.5
```

Example 2. Integer scalars keep integer dtype by default.

```cpp
std::cout << pluss::series::make(7);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=7
```

Example 3. A 1D initializer list becomes a 1D series.

```cpp
std::cout << pluss::series::make({10.0, 12.5, 9.0});
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[10, 12.5, 9]
```

Example 4. Boolean initializer lists stay boolean.

```cpp
std::cout << pluss::series::make({true, false, true, true});
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[true, false, true, true]
```

Example 5. Nested initializer lists infer a higher-rank shape directly.

```cpp
std::cout << pluss::series::make({
    {1.0, 2.0},
    {3.0, 4.0},
});
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 2],
  [3, 4]]
```

Example 6. Integer nested lists infer integer dtype by policy.

```cpp
std::cout << pluss::series::make({
    {1, 2},
    {3, 4},
});
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[[1, 2],
  [3, 4]]
```

Example 7. Flat host data can be paired with an explicit shape option.

```cpp
std::cout << pluss::series::make(
    std::vector<double>{1.0, 2.0, 3.0, 4.0},
    {2, 2});
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 2],
  [3, 4]]
```

This is the important split inside `make()`:

- nested values imply shape naturally
- flat values need a shape hint if you want more than rank 1

Example 8. The explicit-shape path can build higher-rank results too.

```cpp
std::cout << pluss::series::make(
    std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0},
    {1, 2, 3});
```

### Output

```text
pluss::Series(shape=[1,2,3], strides=[6,3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[[1, 2, 3],
  [4, 5, 6]]]
```

Example 9. A dtype override changes how the incoming values are stored.

```cpp
std::cout << pluss::series::make(
    std::vector<int>{1, 2, 3, 4},
    {2, 2},
    pluss::ScalarType::Float64);
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 2],
  [3, 4]]
```

The source values were integers, but the explicit dtype override stored them as
`float64`.

Example 10. The config path can override dtype as well.

```cpp
std::cout << pluss::series::make(
    std::vector<int>{1, 2, 3, 4},
    {2, 2},
    pluss::Config{}.with_dtype(pluss::ScalarType::Int16));
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=int16, platform=cpu, numel=4, contiguous=true)
data=[[1, 2],
  [3, 4]]
```

## Notes

- `make()` is the most flexible value-first constructor on the public Series
  surface.
- Flat buffers plus explicit shape are one of its most important practical
  uses.
- Additional config and autodiff options are accepted through the same option
  stream even though this first page focuses on the core construction cases.

## See Also

- [`pluss::series::full`](full.md): shape-first constant construction.
- [`pluss::series::arange`](arange.md): generated ranges instead of user-supplied
  values.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="linspace.html">
    <span class="label">Previous</span>
    <span class="title">linspace</span>
  </a>
  <a class="next-link" href="print.html">
    <span class="label">Next</span>
    <span class="title">print</span>
  </a>
</div>
