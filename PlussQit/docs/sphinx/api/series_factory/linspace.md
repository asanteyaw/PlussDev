<!-- pluss-hand-authored -->

# pluss::series::linspace

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
Series pluss::series::linspace(double start,
                               double end,
                               std::int64_t steps,
                               ScalarType dtype = ScalarType::Float32)

Series pluss::series::linspace(double start,
                               double end,
                               std::int64_t steps,
                               Config config)
```

## What It Builds

Builds a 1D grid of evenly spaced values between two endpoints.

This is the key contrast with [`arange`](arange.md):

- `arange(start, end, step)` thinks in increments
- `linspace(start, end, steps)` thinks in sample count

That difference matters because `linspace()` is endpoint-oriented. When `steps`
is greater than one, both `start` and `end` are included in the output.

## Parameters

**start** (`double`)

The first emitted value.

**end** (`double`)

The last emitted value when `steps > 1`.

**steps** (`std::int64_t`)

How many samples to emit.

This is a count, not a step size.

**dtype** (`ScalarType`)

The destination dtype for the explicit-dtype overload.

**config** (`Config`)

The config path for dtype/platform routing.

## Returns

`Series`
    A rank-1 series containing `steps` evenly spaced samples.

## Examples

Example 1. `linspace()` includes both endpoints when it emits multiple values.

```cpp
std::cout << pluss::series::linspace(-1.0, 1.0, 5, pluss::ScalarType::Float64);
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-1, -0.5, 0, 0.5, 1]
```

This is the canonical reason to choose `linspace()` over `arange()`: you ask
for the number of samples you want, and the factory chooses the spacing.

Example 2. With one step, the only emitted sample is `start`.

```cpp
std::cout << pluss::series::linspace(3.5, 9.0, 1, pluss::ScalarType::Float64);
```

### Output

```text
pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[3.5]
```

Example 3. Descending endpoint grids work too.

```cpp
std::cout << pluss::series::linspace(3.0, -3.0, 4, pluss::ScalarType::Float64);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[3, 1, -1, -3]
```

Example 4. When `start == end`, the factory repeats that same endpoint.

```cpp
std::cout << pluss::series::linspace(2.5, 2.5, 4, pluss::ScalarType::Float64);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[2.5, 2.5, 2.5, 2.5]
```

Example 5. Two steps is the smallest multi-endpoint grid.

```cpp
std::cout << pluss::series::linspace(10.0, 20.0, 2, pluss::ScalarType::Float64);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[10, 20]
```

Example 6. A current quirk worth documenting explicitly: `steps = 0` returns
an empty series instead of throwing.

```cpp
std::cout << pluss::series::linspace(0.0, 1.0, 0);
```

### Output

```text
pluss::Series(shape=[0], strides=[1], dtype=float32, platform=cpu, numel=0, contiguous=true)
[]
```

## Notes

- `linspace()` is always rank `1`.
- The final argument is a sample count, not a stride.
- `steps=0` currently yields an empty result.
- `steps=1` yields `[start]`, not some average of `start` and `end`.

## See Also

- [`pluss::series::arange`](arange.md): increment-driven range construction.
- [`pluss::series::eye`](eye.md): another structured factory where shape, not
  input values, is the main idea.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="full.html">
    <span class="label">Previous</span>
    <span class="title">full</span>
  </a>
  <a class="next-link" href="make.html">
    <span class="label">Next</span>
    <span class="title">make</span>
  </a>
</div>
