<!-- pluss-hand-authored -->

# pluss::series::from_host

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
Series pluss::series::from_host(void* data,
                                pluss::RangeView<std::int64_t> sizes,
                                std::span<const std::int64_t> strides,
                                std::function<void(void*)> deleter,
                                ScalarType dtype = ScalarType::Float32)
```

## What It Builds

Wraps memory you already own as a `Series`, without copying it. The series reads and writes `data` directly, so a change you make to the
buffer is visible through the series and the other way round. The `deleter` runs once, when the last copy of the series is gone, so that is
where the buffer is released.

Use it to hand a series a buffer that came from elsewhere. To build a series from values, use [`make`](make.md) instead, which copies them.

## Parameters

**data** (`void*`)

The first element of the buffer, on the CPU. It must stay valid until the deleter runs.

**sizes** (`pluss::RangeView<std::int64_t>`)

The extent of each dimension. A braced list or an existing vector works.

**strides** (`std::span<const std::int64_t>`)

The distance, in elements and not bytes, between neighbours along each dimension. A row-major matrix of `2 x 3` has strides `{3, 1}`; its
transpose over the same buffer has `{1, 3}`. There must be one stride per dimension.

**deleter** (`std::function<void(void*)>`)

Called with the buffer pointer when the series, and every copy of it, has been destroyed.

**dtype** (`ScalarType`, default `Float32`)

How the bytes are read. It must match what the buffer holds.

## Returns

`Series`
    A view over the buffer, with the given shape and strides. It is not contiguous when the strides are not the row-major ones.

## Examples

Example 1. Wrap a heap buffer, and see that the series and the buffer share memory.

```cpp
auto* data = new double[6]{1, 2, 3, 4, 5, 6};
bool freed = false;
{
  std::vector<std::int64_t> sizes{2, 3};
  std::vector<std::int64_t> strides{3, 1};
  auto x = pluss::series::from_host(data, sizes, strides,
                                    [data, &freed](void*) { delete[] data; freed = true; },
                                    pluss::ScalarType::Float64);
  std::cout << x;
  data[0] = 100;
  std::cout << "after writing the buffer: " << x.sum().item<double>() << "\n";
  std::cout << "freed yet: " << freed << "\n";
}
std::cout << "freed after the series is gone: " << freed << "\n";
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 2, 3],
  [4, 5, 6]]after writing the buffer: 120
freed yet: 0
freed after the series is gone: 1
```

Example 2. A transposed view of the same six numbers, by swapping the strides.

```cpp
auto* data = new float[6]{1, 2, 3, 4, 5, 6};

auto t = pluss::series::from_host(data, std::vector<std::int64_t>{3, 2}, std::vector<std::int64_t>{1, 3},
                                  [data](void*) { delete[] data; });
std::cout << t;
```

### Output

```text
pluss::Series(shape=[3,2], strides=[1,3], dtype=float32, platform=cpu, numel=6, contiguous=false)
data=[[1, 4],
  [2, 5],
  [3, 6]]
```

## Notes

- The series never frees the buffer itself. If `deleter` does nothing, the memory must outlive every copy of the series.
- Copies of the series share the buffer, so the deleter runs only after the last copy is destroyed.
- The buffer must really hold `dtype` values laid out as the sizes and strides say. This is not checked.

## See Also

- [`pluss::series::make`](make.md): build a series from values, copying them.
- [`pluss::series::from_torch`](from_torch.md): wrap a tensor without copying it.
