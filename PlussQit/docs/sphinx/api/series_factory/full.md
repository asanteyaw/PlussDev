<!-- pluss-hand-authored -->

# pluss::series::full

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
Series pluss::series::full(sizes, double fill_value,
                           ScalarType dt = ScalarType::Float32)

Series pluss::series::full(sizes, double fill_value,
                           Config cfg)

Series pluss::series::full(sizes, double fill_value,
                           opts...)

Series pluss::series::full(sizes, double fill_value,
                           {{"key", value}, ...})
```

## What It Builds

Creates a new `Series` whose every element is initialized to the same value.

This makes `full()` the most general member of the shape-first constant family:

- [`zeros()`](zeros.md) is the fixed-value `0` specialization
- [`ones()`](ones.md) is the fixed-value `1` specialization
- `full()` lets you choose the value directly

## Parameters

**sizes**

The output shape.

**fill_value** (`double`)

The value written into every element before any user code touches the result.

Because the public value parameter is `double`, what actually gets stored then
depends on the destination dtype. That is especially relevant for integral
outputs.

**dt** (`ScalarType`)

The explicit destination dtype for the simple overload.

**cfg** (`Config`)

The configuration surface for dtype/platform routing.

## Returns

`Series`
    A newly allocated series with every element initialized to `fill_value`
    after dtype conversion.

## Examples

Example 1. The default dtype path builds a float32 constant block.

```cpp
std::cout << pluss::series::full({2, 3}, 1.5);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float32, platform=cpu, numel=6, contiguous=true)
data=[[1.5, 1.5, 1.5],
  [1.5, 1.5, 1.5]]
```

Example 2. `full({}, value)` is the scalar construction form.

```cpp
std::cout << pluss::series::full({}, -2.0);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float32, platform=cpu, numel=1, contiguous=true)
data=-2
```

That is often cleaner than building a one-element vector and then reshaping it
back down to a scalar.

Example 3. The dtype overload converts the fill value into the requested dtype.

```cpp
std::cout << pluss::series::full({2, 2}, 7.0, pluss::ScalarType::Int32);
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=int32, platform=cpu, numel=4, contiguous=true)
data=[[7, 7],
  [7, 7]]
```

Example 4. Boolean destinations treat any non-zero fill as `true`.

```cpp
std::cout << pluss::series::full({4}, 2.0, pluss::ScalarType::Bool);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[true, true, true, true]
```

Example 5. Fractional fills are converted to the destination dtype before
storage.

```cpp
std::cout << pluss::series::full({3}, 2.75, pluss::ScalarType::Int32);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=int32, platform=cpu, numel=3, contiguous=true)
data=[2, 2, 2]
```

That conversion step is one of the main `full()` gotchas: the public fill
parameter is `double`, but the stored payload always follows the destination
dtype.

## Notes

- `full()` is the general constant-valued shape-first constructor.
- `{}` gives you a scalar result.
- The fill parameter is `double`, but storage follows the destination dtype.

## See Also

- [`pluss::series::zeros`](zeros.md): specialized `full(..., 0)`.
- [`pluss::series::ones`](ones.md): specialized `full(..., 1)`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="eye.html">
    <span class="label">Previous</span>
    <span class="title">eye</span>
  </a>
  <a class="next-link" href="linspace.html">
    <span class="label">Next</span>
    <span class="title">linspace</span>
  </a>
</div>
