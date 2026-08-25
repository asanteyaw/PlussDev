<!-- pluss-hand-authored -->

# pluss::series::arange

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
pluss::series::arange(std::int64_t end,
                      ScalarType dt = ScalarType::Int64) -> Series

pluss::series::arange(std::int64_t start,
                      std::int64_t end,
                      ScalarType dt = ScalarType::Int64) -> Series

pluss::series::arange(std::int64_t start,
                      std::int64_t end,
                      std::int64_t step,
                      ScalarType dt = ScalarType::Int64) -> Series

pluss::series::arange(std::int64_t start,
                      std::int64_t end,
                      Config cfg) -> Series

pluss::series::arange(std::int64_t start,
                      std::int64_t end,
                      std::int64_t step,
                      Config cfg) -> Series
```

## What It Builds

Builds a 1D arithmetic progression.

This is the canonical "counting range" factory:

- `arange(5)` means `0, 1, 2, 3, 4`
- `arange(2, 7)` means `2, 3, 4, 5, 6`
- `arange(10, 0, -3)` means `10, 7, 4, 1`

So the mental model is not "give me N values." The mental model is:

"start here, keep stepping, stop before crossing the end boundary."

## Parameters

**start** (`std::int64_t`)

The first value emitted.

**end** (`std::int64_t`)

The exclusive boundary. `end` itself is not included.

**step** (`std::int64_t`)

How much to move between emitted values.

- positive `step`: walk upward
- negative `step`: walk downward

**dt** (`ScalarType`)

The output dtype for the explicit-dtype overloads.

The default is `Int64`, which is the natural default for an integer counting
range.

**cfg** (`Config`)

Configuration overload for dtype/platform routing.

One important current quirk is worth calling out explicitly:

- if `cfg.dtype` is `Float32`, this overload still normalizes to `Int64`

That is a current policy choice in the implementation, not a docs typo.

## Returns

`Series`
    A rank-1 numeric series containing the arithmetic progression.

## Examples

Example 1. The one-argument form starts at zero.

```cpp
std::cout << pluss::series::arange(5);
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=int64, platform=cpu, numel=5, contiguous=true)
data=[0, 1, 2, 3, 4]
```

Example 2. The two-bound form lets you choose the starting value.

```cpp
std::cout << pluss::series::arange(2, 7);
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=int64, platform=cpu, numel=5, contiguous=true)
data=[2, 3, 4, 5, 6]
```

This is a half-open interval: `2` is included and `7` is not.

Example 3. A negative step walks backward.

```cpp
std::cout << pluss::series::arange(10, 0, -3);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[10, 7, 4, 1]
```

The sequence stops before it would cross the exclusive lower boundary `0`.

Example 4. Positive multi-step ranges stop before the exclusive end as well.

```cpp
std::cout << pluss::series::arange(0, 10, 2);
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=int64, platform=cpu, numel=5, contiguous=true)
data=[0, 2, 4, 6, 8]
```

Example 5. When the interval is already exhausted, `arange()` returns an empty
range object.

```cpp
std::cout << pluss::series::arange(5, 5);
```

### Output

```text
pluss::Series(shape=[0], strides=[1], dtype=int64, platform=cpu, numel=0, contiguous=true)
[]
```

Example 6. The `Config` overload currently preserves integer `arange`
semantics even when the config asks for `Float32`.

```cpp
std::cout << pluss::series::arange(
    2,
    7,
    pluss::Config{}.with_dtype(pluss::ScalarType::Float32));
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=int64, platform=cpu, numel=5, contiguous=true)
data=[2, 3, 4, 5, 6]
```

That shows the current rule clearly: the config path does not emit a float range
just because `cfg.dtype` was `Float32`.

## Notes

- `arange` is a 1D factory.
- The interval is half-open.
- The explicit `Config` path currently coerces `Float32` back to `Int64`.

## See Also

- [`pluss::series::make`](make.md): direct value construction instead of counting.
- [`pluss::series::zeros`](zeros.md): shape-first zero allocation.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../../series_factory_api_reference.html">
    <span class="label">Previous</span>
    <span class="title">Series Factory API Reference</span>
  </a>
  <a class="next-link" href="empty.html">
    <span class="label">Next</span>
    <span class="title">empty</span>
  </a>
</div>
