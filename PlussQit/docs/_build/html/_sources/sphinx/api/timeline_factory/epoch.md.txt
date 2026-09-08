<!-- pluss-hand-authored -->
# pluss::timeline::epoch

[Back to Timeline Factory API Reference](../../timeline_factory_api_reference)

## Signature

```cpp
Timeline pluss::timeline::epoch(integer_range sizes = {})
```

## Allocate A Known Shape At Tick Zero

`epoch()` allocates an owning `Timeline` whose every lane is the Unix epoch:
the valid instant represented by epoch nanosecond `0`, displayed in UTC as
`1970-01-01T00:00:00.000000000+00:00`.

This factory is useful when shape must be established before meaningful
instants are assigned or transformed. Epoch is ordinary data, not an absent
instant and not an uninitialized placeholder. Code that must represent absence
should construct `Timeline::kNaT` explicitly with `from_epoch()` or `full()`.

Omitting `sizes` creates a rank-zero scalar. `{n}` creates a vector, and two or
three extents create a matrix or rank-three value. Zero-length axes are retained
as geometry. The public `Timeline` contract supports ranks zero through three.

The returned value owns fresh contiguous `int64` storage. Construction is
linear in the number of elements and initializes the storage without parsing
text or converting through floating point.

## Example 1: The Default Is A Scalar

```cpp
const auto origin = pluss::timeline::epoch();
std::cout << pluss::note::print(
    pluss::timeline::format(origin, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"1970-01-01T00:00:00.000000000+00:00"
```

An empty shape list means rank zero and one lane; it does not mean zero lanes.

## Example 2: A Vector Of Exact Zero Ticks

```cpp
const auto values = pluss::timeline::epoch({4});
std::cout << pluss::series::print(values.epoch_nanos()) << '\n';
```

### Output

```text
Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[0, 0, 0, 0]
```

## Example 3: Rank-Three Geometry

```cpp
const auto block = pluss::timeline::epoch({1, 2, 2});
std::cout << pluss::series::print(block.epoch_nanos()) << '\n';
```

### Output

```text
Series(shape=[1,2,2], strides=[4,2,1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data[0, :, :] =
⎡ 0 0 ⎤
⎣ 0 0 ⎦
```

## Example 4: Zero-Length Axes Preserve Shape

```cpp
const auto empty_rows = pluss::timeline::epoch({2, 0});
std::cout << pluss::timeline::print(empty_rows) << '\n';
```

### Output

```text
Timeline(shape=[2, 0], tz=UTC)[  ]
[  ]
```

The value is defined and has shape `[2, 0]`, rank two, and zero elements. This
is useful in generic shape-preserving code because the nonempty axis is not
discarded.

## Validation And Failure

Every extent must be nonnegative, rank must not exceed three, and the product
of the extents must fit in signed 64-bit shape arithmetic. Validation happens
before allocation.

```cpp
try {
  (void)pluss::timeline::epoch({1, 1, 1, 1});
} catch (const pluss::Error&) {
  std::cout << "rejected\n";
}
```

### Output

```text
rejected
```

## See Also

- [`full`](full.md): allocate a shape filled with any scalar instant.
- [`full_like`](full_like.md): obtain shape from an existing `Timeline`.
- [`from_epoch`](from_epoch.md): construct from supplied epoch nanoseconds.

