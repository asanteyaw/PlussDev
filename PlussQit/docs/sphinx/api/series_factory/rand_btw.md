<!-- pluss-hand-authored -->

# pluss::series::rand_btw

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
Series pluss::series::rand_btw(double lo,
                               double hi,
                               sizes,
                               std::uint64_t seed = 0)

Series pluss::series::rand_btw(double lo,
                               double hi,
                               sizes,
                               Config config,
                               std::uint64_t seed = 0)

Series pluss::series::rand_btw(double lo,
                               double hi,
                               std::int64_t size,
                               std::uint64_t seed = 0)
```

## What It Builds

Builds uniformly distributed floating samples on a caller-chosen interval.

This is the "same idea, different interval" companion to [`rand()`](rand.md):

- `rand()` emits on `[0, 1)`
- `rand_btw(lo, hi, ...)` emits on `[lo, hi)`

That makes it the most convenient factory when the bounds are part of the
problem statement and you do not want to rescale a unit-interval draw by hand.

## Parameters

**lo** (`double`)

The inclusive lower bound.

**hi** (`double`)

The exclusive upper bound.

**sizes**

The output shape.

There is also a convenience overload for a single 1D extent.

**config** (`Config`)

The config path for dtype/platform routing.

One current dtype quirk matters here:

- the simple overload emits `float64`
- the config path only accepts floating outputs (`float32` or `float64`)

## Returns

`Series`
    A floating series of uniform samples in `[lo, hi)`.

## Examples

Example 1. A seeded interval draw stays within the requested bounds.

```cpp
std::cout << pluss::series::rand_btw(-2.0, 2.0, {4}, 42);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.96625948, -1.3603584, -0.88559544, -0.62323713]
```

The shape rule is ordinary shape-first construction; the main difference from
`rand()` is that the value range now follows the caller's interval.

Example 2. The same bounded-uniform rule works for matrix shapes.

```cpp
std::cout << pluss::series::rand_btw(-1.0, 1.0, {2, 2}, 42);
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[0.48312974, -0.68017921],
  [-0.44279772, -0.31161857]]
```

Example 3. The degenerate interval `lo == hi` collapses to a constant result.

```cpp
std::cout << pluss::series::rand_btw(3.0, 3.0, {4}, 42);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[3, 3, 3, 3]
```

Example 4. The config path can request `float32` output explicitly.

```cpp
std::cout << pluss::series::rand_btw(
    -2.0,
    2.0,
    {4},
    pluss::Config{}.with_dtype(pluss::ScalarType::Float32),
    42);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float32, platform=cpu, numel=4, contiguous=true)
data=[0.96625948, -1.3603585, -0.88559544, -0.62323713]
```

## Failure Modes

`hi` must be greater than or equal to `lo`.

If that contract is violated, the factory fails with:

```text
rand_btw: expected hi >= lo
```

Example 5. That failure mode is explicit and immediate.

```cpp
try {
  std::cout << pluss::series::rand_btw(2.0, -2.0, {4}, 42) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
rand_btw: expected hi >= lo
```

## Notes

- The simple overload defaults to `float64`.
- The interval is half-open: values can equal `lo`, but they should not equal
  `hi`.
- The seed semantics are the same as [`rand()`](rand.md).

## See Also

- [`pluss::series::rand`](rand.md): the unit-interval base constructor.
- [`pluss::series::normal`](normal.md): use when mean and spread matter more
  than hard bounds.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rand.html">
    <span class="label">Previous</span>
    <span class="title">rand</span>
  </a>
  <a class="next-link" href="randint.html">
    <span class="label">Next</span>
    <span class="title">randint</span>
  </a>
</div>
