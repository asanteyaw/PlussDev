<!-- pluss-hand-authored -->

# pluss::series::eye_like

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
pluss::series::eye_like(const Series& like,
                        ScalarType dtype = ScalarType::Float32) -> Series

pluss::series::eye_like(const Series& like,
                        Config cfg) -> Series

pluss::series::eye_like(const Series& like,
                        opts...) -> Series
```

## What It Builds

`eye_like()` is not "copy this tensor and put ones on its diagonal."

It is "use the final two dimensions of this tensor as the matrix shape for an
identity-like constructor."

So the important rule is:

- rank must be at least `2`
- only the last two dimensions matter

## Verified Examples

Example 1. A `2 x 3` reference tensor produces a `2 x 3` identity layout.

```cpp
auto like = pluss::series::make({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});

std::cout << pluss::series::eye_like(like);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float32, platform=cpu, numel=6, contiguous=true)
data=[[1, 0, 0],
  [0, 1, 0]]
```

Example 2. The dtype override applies to the identity values too.

```cpp
std::cout << pluss::series::eye_like(like, pluss::ScalarType::Bool);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=[[true, false, false],
  [false, true, false]]
```

Example 3. Higher-rank references do not preserve rank. Only the final two
dimensions are reused.

```cpp
auto like_rank3 = pluss::series::make(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, {1, 2, 3});
std::cout << pluss::series::eye_like(like_rank3, pluss::ScalarType::Int16);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=int16, platform=cpu, numel=6, contiguous=true)
data=[[1, 0, 0],
  [0, 1, 0]]
```

Example 4. The config overload changes dtype policy but not the final-two-axis
shape rule.

```cpp
std::cout << pluss::series::eye_like(
    like,
    pluss::Config{}.with_dtype(pluss::ScalarType::Int16));
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=int16, platform=cpu, numel=6, contiguous=true)
data=[[1, 0, 0],
  [0, 1, 0]]
```

Example 5. Rank-1 inputs fail because there is no final two-axis matrix shape to reuse.

```cpp
try {
  auto vec = pluss::series::make({1.0, 2.0, 3.0});
  std::cout << pluss::series::eye_like(vec) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
eye_like: expected input with rank >= 2
```

## Notes

- `eye_like()` reuses matrix geometry, not data values.
- The default dtype here is `Float32`, not "whatever `like` already was."
- Rank greater than two is reduced to the final two dimensions.

## See Also

- [`pluss::series::eye`](eye.md): explicit row/column identity constructor.
- [`pluss::series::zeros_like`](zeros_like.md): geometry reuse without diagonal structure.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="randint_like.html">
    <span class="label">Previous</span>
    <span class="title">randint_like</span>
  </a>
  <a class="next-link" href="bernoulli.html">
    <span class="label">Next</span>
    <span class="title">bernoulli</span>
  </a>
</div>
