<!-- pluss-hand-authored -->

# pluss::series::randperm

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
Series pluss::series::randperm(std::int64_t n,
                               ScalarType dtype = ScalarType::Int64,
                               std::uint64_t seed = 0)

Series pluss::series::randperm(std::int64_t n,
                               Config config,
                               std::uint64_t seed = 0)
```

## What It Builds

Builds a random permutation of the integers `0` through `n - 1`.

This is not "random integers without caring about repetition." It is the
stronger contract:

- every value appears exactly once
- the output is always rank `1`
- the result is a shuffled ordering of the canonical index range

That makes `randperm()` especially useful for shuffle indices, randomized row
orders, and train/test splits built from explicit index vectors.

## Parameters

**n** (`std::int64_t`)

How many integers should appear in the permutation.

**dtype** (`ScalarType`)

The destination dtype for the direct overload.

**config** (`Config`)

The config path for dtype/platform routing.

The config path has the same current integer-policy quirk seen on
[`randint()`](randint.md):

- if `config.dtype` is left at generic `Float32`, it is normalized to `Int64`

**seed** (`std::uint64_t`)

The explicit seed for reproducible permutations.

## Returns

`Series`
    A rank-1 permutation of `0..n-1`.

## Examples

Example 1. A seeded permutation of six indices.

```cpp
std::cout << pluss::series::randperm(6, pluss::ScalarType::Int64, 42);
```

### Output

```text
pluss::Series(shape=[6], strides=[1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[0, 3, 4, 2, 1, 5]
```

The defining property is not the specific order. It is that every integer from
`0` to `5` appears exactly once.

Example 2. `n = 0` yields an empty permutation rather than an error.

```cpp
std::cout << pluss::series::randperm(0, pluss::ScalarType::Int64, 42);
```

### Output

```text
pluss::Series(shape=[0], strides=[1], dtype=int64, platform=cpu, numel=0, contiguous=true)
[]
```

Example 3. The permutation rule is independent of the integer output dtype.

```cpp
std::cout << pluss::series::randperm(6, pluss::ScalarType::Int32, 42);
```

### Output

```text
pluss::Series(shape=[6], strides=[1], dtype=int32, platform=cpu, numel=6, contiguous=true)
data=[0, 3, 4, 2, 1, 5]
```

Example 4. Reusing the same explicit seed replays the same permutation.

```cpp
auto a = pluss::series::randperm(6, pluss::ScalarType::Int64, 17);
auto b = pluss::series::randperm(6, pluss::ScalarType::Int64, 17);

std::cout << a << "\n" << b;
```

### Output

```text
pluss::Series(shape=[6], strides=[1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[3, 1, 0, 2, 4, 5]
pluss::Series(shape=[6], strides=[1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[3, 1, 0, 2, 4, 5]
```

Example 5. The config path currently normalizes a generic `Float32` request
back to `Int64`.

```cpp
std::cout << pluss::series::randperm(
    6,
    pluss::Config{}.with_dtype(pluss::ScalarType::Float32),
    42);
```

### Output

```text
pluss::Series(shape=[6], strides=[1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[0, 3, 4, 2, 1, 5]
```

## Failure Modes

`n` must be non-negative.

If not, the factory fails with:

```text
randperm: expected n >= 0
```

Example 6. Negative sizes fail immediately.

```cpp
try {
  std::cout << pluss::series::randperm(-1, pluss::ScalarType::Int64, 42) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
randperm: expected n >= 0
```

## Notes

- `randperm()` is the right choice when repeats are not allowed.
- The result is always one-dimensional.
- The default dtype is `Int64`.

## See Also

- [`pluss::series::randint`](randint.md): discrete draws where repetition is
  allowed.
- [`pluss::series::arange`](arange.md): the ordered, unshuffled base range.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="normal.html">
    <span class="label">Previous</span>
    <span class="title">normal</span>
  </a>
  <a class="next-link" href="empty_like.html">
    <span class="label">Next</span>
    <span class="title">empty_like</span>
  </a>
</div>
