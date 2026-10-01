<!-- pluss-hand-authored -->

# Duration::sizes

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
auto Duration::sizes() const
```

## What It Tells You

`sizes()` exposes the logical extent of each axis in outermost-to-innermost
order. A scalar reports `[]`, not `[1]`; `[1]` is a one-element vector.

The result is a borrowed, read-only range backed by the Duration handle. It is
cheap to inspect but should not outlive the Duration. Construct an owning
`std::vector<std::int64_t>` when shape metadata must be stored independently.
Shape describes arrangement only; it does not encode units, validity, or lane
values.

## Returns

A borrowed range whose length equals [`dim()`](dim.md). The product of its
extents equals [`numel()`](numel.md).

## Examples

Example 1. Scalar, vector, matrix, and rank-three shapes.

```cpp
auto shape = [](const pluss::Duration& d) {
  std::cout << '[';
  for (std::size_t i = 0; i < d.sizes().size(); ++i) {
    if (i) std::cout << ',';
    std::cout << d.sizes()[i];
  }
  std::cout << "]\n";
};

shape(pluss::duration::seconds(90));
shape(pluss::duration::minutes({15,30,45}));
shape(pluss::duration::hours({1,2,3,4}, {2,2}));
shape(pluss::duration::nanos({1,2,3,4}, {2,1,2}));
```

### Output

```text
[]
[3]
[2,2]
[2,1,2]
```

Example 2. Preserve a zero-length axis.

```cpp
const auto empty = pluss::duration::zeros({2,0,3});
std::cout << "dim=" << empty.dim() << " numel=" << empty.numel() << " sizes=[";
for (std::size_t i = 0; i < empty.sizes().size(); ++i) {
  if (i) std::cout << ',';
  std::cout << empty.sizes()[i];
}
std::cout << "]\n";
```

### Output

```text
dim=3 numel=0 sizes=[2,0,3]
```

Example 3. Take an owning copy for independent storage.

```cpp
const auto value = pluss::duration::hours({1,2,3,4}, {2,2});
const std::vector<std::int64_t> owned(value.sizes().begin(), value.sizes().end());
std::cout << "rank=" << owned.size()
          << " rows=" << owned[0] << " columns=" << owned[1] << "\n";
```

### Output

```text
rank=2 rows=2 columns=2
```

## See Also

- [`dim`](dim.md): number of extents.
- [`numel`](numel.md): product of extents.
- [`host`](host.md): owning copy of lane data rather than shape metadata.
