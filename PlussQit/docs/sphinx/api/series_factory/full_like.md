<!-- pluss-hand-authored -->

# pluss::series::full_like

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
Series pluss::series::full_like(const Series& like,
                                double fill_value,
                                ScalarType dtype)

Series pluss::series::full_like(const Series& like,
                                double fill_value,
                                Config cfg = {})

Series pluss::series::full_like(const Series& like,
                                double fill_value,
                                opts...)
```

## What It Builds

`full_like()` is the most general geometry-preserving constant constructor.

It keeps the shape of `like`, then fills every element with the caller's chosen
value. So it is the like-factory counterpart to [`full()`](full.md), and the
parent surface that `zeros_like()` and `ones_like()` conceptually specialize.

## Verified Examples

Example 1. The default path preserves the source dtype while repeating the fill.

```cpp
auto like = pluss::series::make({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});

std::cout << pluss::series::full_like(like, -2.5);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-2.5, -2.5, -2.5],
  [-2.5, -2.5, -2.5]]
```

Example 2. The dtype override converts the fill into a different storage type.

```cpp
std::cout << pluss::series::full_like(like, 7.0, pluss::ScalarType::Int32);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=int32, platform=cpu, numel=6, contiguous=true)
data=[[7, 7, 7],
  [7, 7, 7]]
```

Example 3. Scalar references stay scalar here too.

```cpp
auto like_scalar = pluss::series::make(5.0);
std::cout << pluss::series::full_like(like_scalar, -2.5);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=-2.5
```

Example 4. Boolean destinations interpret non-zero fills as `true`.

```cpp
std::cout << pluss::series::full_like(like, 3.0, pluss::ScalarType::Bool);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=[[true, true, true],
  [true, true, true]]
```

## Notes

- `full_like()` preserves geometry, not payload type, unless you leave dtype implicit.
- The fill parameter is still `double`, even when the destination dtype is integral.

## See Also

- [`pluss::series::full`](full.md): shape-first repeated fill.
- [`pluss::series::ones_like`](ones_like.md): the fixed-value `1` specialization.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ones_like.html">
    <span class="label">Previous</span>
    <span class="title">ones_like</span>
  </a>
  <a class="next-link" href="rand_like.html">
    <span class="label">Next</span>
    <span class="title">rand_like</span>
  </a>
</div>
