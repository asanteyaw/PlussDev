<!-- pluss-hand-authored -->

# pluss::series::zeros

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
Series pluss::series::zeros(sizes,
                            ScalarType dt = ScalarType::Float32)

Series pluss::series::zeros(sizes,
                            Config cfg)

Series pluss::series::zeros(sizes,
                            opts...)

Series pluss::series::zeros(sizes,
                            {{"key", value}, ...})
```

## What It Builds

Allocates a new `Series` and initializes every element to zero.

This is one of the most load-bearing constructors in the whole surface because
it is both:

- a user-facing constant factory
- a common staging surface for scratch buffers and downstream writes

Unlike [`empty()`](empty.md), the payload is known immediately.

## Parameters

**sizes**

The requested shape.

**dt** (`ScalarType`)

The explicit destination dtype for the simple overload.

**cfg** (`Config`)

Config-based dtype/platform routing.

## Returns

`Series`
    A newly allocated, fully initialized zero-valued series.

## Examples

Example 1. The default overload emits float32 zeros.

```cpp
std::cout << pluss::series::zeros({2, 3});
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float32, platform=cpu, numel=6, contiguous=true)
data=[[0, 0, 0],
  [0, 0, 0]]
```

Example 2. Zero-initialization is equally valid for boolean outputs.

```cpp
std::cout << pluss::series::zeros({4}, pluss::ScalarType::Bool);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[false, false, false, false]
```

For `bool`, the zero constructor naturally means `false`.

Example 3. A scalar shape gives a scalar zero.

```cpp
std::cout << pluss::series::zeros({}, pluss::ScalarType::Float64);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=0
```

Example 4. Integer dtypes keep the same initialization contract.

```cpp
std::cout << pluss::series::zeros({4}, pluss::ScalarType::Int16);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=int16, platform=cpu, numel=4, contiguous=true)
data=[0, 0, 0, 0]
```

## Notes

- `zeros()` is the predictable-initialization alternative to [`empty()`](empty.md).
- For boolean outputs, zero means `false`.
- For non-zero constants, prefer [`full()`](full.md).

## See Also

- [`pluss::series::empty`](empty.md): same shape-first pattern without payload
  initialization.
- [`pluss::series::ones`](ones.md): the unit-valued sibling.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ones.html">
    <span class="label">Previous</span>
    <span class="title">ones</span>
  </a>
  <a class="next-link" href="rand.html">
    <span class="label">Next</span>
    <span class="title">rand</span>
  </a>
</div>
