<!-- pluss-hand-authored -->

# pluss::series::normal

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
pluss::series::normal(double mean,
                      double std,
                      sizes,
                      ScalarType dtype = ScalarType::Float32,
                      std::uint64_t seed = 0) -> Series

pluss::series::normal(double mean,
                      double std,
                      sizes,
                      Config config,
                      std::uint64_t seed = 0) -> Series
```

## What It Builds

Builds Gaussian random samples with caller-chosen mean and standard deviation.

This is the parameterized companion to [`randn()`](randn.md):

- `randn()` means standard normal: mean `0`, std `1`
- `normal(mean, std, ...)` means "shift and scale to the distribution I want"

So `normal()` is usually the better teaching surface once the problem statement
already names a mean and a spread.

## Parameters

**mean** (`double`)

The target mean of the distribution.

**std** (`double`)

The target standard deviation.

**sizes**

The output shape.

**dtype** (`ScalarType`)

The destination floating dtype for the direct overload.

**config** (`Config`)

The config path for dtype/platform routing.

**seed** (`std::uint64_t`)

The explicit seed for reproducible draws.

## Returns

`Series`
    A floating series of Gaussian samples with the requested mean and standard
    deviation.

## Examples

Example 1. A seeded parameterized normal draw.

```cpp
std::cout << pluss::series::normal(10.0, 2.0, {4}, pluss::ScalarType::Float64, 42);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[10.82944, 11.305362, 8.2162275, 12.653667]
```

The sample is centered around `10` and spread roughly like a width-`2`
Gaussian, but small batches still vary visibly from the theoretical moments.

Example 2. The same constructor works on matrix shapes.

```cpp
std::cout << pluss::series::normal(0.0, 1.0, {2, 2}, pluss::ScalarType::Float32, 42);
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float32, platform=cpu, numel=4, contiguous=true)
data=[[0.41471976, 0.65268123],
  [-0.89188623, 1.3268336]]
```

Example 3. `std = 0` collapses the distribution to a constant fill at `mean`.

```cpp
std::cout << pluss::series::normal(5.0, 0.0, {4}, pluss::ScalarType::Float64, 42);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[5, 5, 5, 5]
```

Example 4. Negative means are fine; only the standard deviation has the
non-negativity contract.

```cpp
std::cout << pluss::series::normal(-3.0, 0.5, {4}, pluss::ScalarType::Float64, 42);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[-2.7926401, -2.6736594, -3.4459431, -2.3365832]
```

Example 5. Reusing the same explicit seed replays the same shifted-and-scaled
Gaussian sample.

```cpp
auto a = pluss::series::normal(1.0, 0.25, {4}, pluss::ScalarType::Float32, 5);
auto b = pluss::series::normal(1.0, 0.25, {4}, pluss::ScalarType::Float32, 5);

std::cout << a << "\n" << b;
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float32, platform=cpu, numel=4, contiguous=true)
data=[1.0049948, 0.65544766, 1.3464099, 1.2494913]
pluss::Series(shape=[4], strides=[1], dtype=float32, platform=cpu, numel=4, contiguous=true)
data=[1.0049948, 0.65544766, 1.3464099, 1.2494913]
```

## Failure Modes

The standard deviation must be non-negative.

If not, the factory fails with:

```text
normal: expected std >= 0
```

Example 6. Negative standard deviations fail immediately.

```cpp
try {
  std::cout << pluss::series::normal(0.0, -1.0, {4}, pluss::ScalarType::Float64, 42) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
normal: expected std >= 0
```

## Notes

- `normal()` is usually the right Gaussian factory for user-facing code.
- The seed semantics are the same as [`randn()`](randn.md) and [`rand()`](rand.md).
- The direct overload defaults to `float32`.

## See Also

- [`pluss::series::randn`](randn.md): standard-normal base constructor.
- [`pluss::series::rand_btw`](rand_btw.md): uniform bounded draws.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="randn.html">
    <span class="label">Previous</span>
    <span class="title">randn</span>
  </a>
  <a class="next-link" href="randperm.html">
    <span class="label">Next</span>
    <span class="title">randperm</span>
  </a>
</div>
