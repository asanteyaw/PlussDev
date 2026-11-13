<!-- pluss-hand-authored -->

# pluss::series::ones

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
Series pluss::series::ones(sizes,
                           ScalarType dt = ScalarType::Float32)

Series pluss::series::ones(sizes,
                           Config cfg)

Series pluss::series::ones(sizes,
                           opts...)

Series pluss::series::ones(sizes,
                           {{"key", value}, ...})
```

## What It Builds

Allocates a new series and initializes every element to `1`.

This is the unit-constant sibling of:

- [`zeros()`](zeros.md)
- [`full()`](full.md)

Use it when the constant value really is conceptually "one" rather than just
"some repeated fill."

## Parameters

**sizes**

The requested output shape.

**dt** (`ScalarType`)

The explicit output dtype for the simple overload.

**cfg** (`Config`)

The config-based routing/dtype surface.

## Returns

`Series`
    A newly allocated series filled entirely with ones.

## Examples

Example 1. The default path emits a float32 tensor of ones.

```cpp
std::cout << pluss::series::ones({2, 3});
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float32, platform=cpu, numel=6, contiguous=true)
data=[[1, 1, 1],
  [1, 1, 1]]
```

Example 2. The dtype overload applies equally well to integer outputs.

```cpp
std::cout << pluss::series::ones({4}, pluss::ScalarType::Int16);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=int16, platform=cpu, numel=4, contiguous=true)
data=[1, 1, 1, 1]
```

Example 3. A scalar shape gives a scalar one rather than a length-1 vector.

```cpp
std::cout << pluss::series::ones({}, pluss::ScalarType::Float64);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=1
```

Example 4. Boolean outputs interpret the unit value as `true`.

```cpp
std::cout << pluss::series::ones({4}, pluss::ScalarType::Bool);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[true, true, true, true]
```

## Notes

- `ones()` is just the constant-1 specialization of the broader shape-first
  constructor family.
- When the value is not literally one, prefer [`full()`](full.md).

## See Also

- [`pluss::series::zeros`](zeros.md): the zero-valued sibling.
- [`pluss::series::full`](full.md): the general repeated-fill constructor.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="select.html">
    <span class="label">Previous</span>
    <span class="title">select</span>
  </a>
  <a class="next-link" href="zeros.html">
    <span class="label">Next</span>
    <span class="title">zeros</span>
  </a>
</div>
