<!-- pluss-hand-authored -->

# Duration::dim

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
std::int64_t Duration::dim() const
```

## What It Tells You

`dim()` returns the logical rank: the number of coordinates required to select
one elapsed-time lane. A scalar has rank zero, a vector rank one, a matrix rank
two, and a slab collection rank three.

Rank is independent of cardinality. A scalar and a one-element vector both
contain one lane but have ranks zero and one. Likewise, shape `[2,0,3]` retains
three axes even though it contains no lanes. Use [`sizes()`](sizes.md) for each
axis extent and [`numel()`](numel.md) for the total lane count.

## Returns

The rank as `std::int64_t`. Public Duration factories accept ranks zero through
three and reject higher-rank shapes during construction.

## Examples

Example 1. Scalar and singleton-vector structure remain distinct.

```cpp
const auto scalar = pluss::duration::seconds(90);
const auto singleton = pluss::duration::seconds({90});
std::cout << "scalar dim=" << scalar.dim() << " numel=" << scalar.numel() << "\n"
          << "singleton dim=" << singleton.dim() << " numel=" << singleton.numel() << "\n";
```

### Output

```text
scalar dim=0 numel=1
singleton dim=1 numel=1
```

Example 2. Explicit shape controls rank without changing the unit conversion.

```cpp
const auto matrix = pluss::duration::hours({1,2,3,4}, {2,2});
const auto cube = pluss::duration::nanos({1,2,3,4}, {2,1,2});
std::cout << "matrix dim=" << matrix.dim() << " numel=" << matrix.numel() << "\n"
          << "cube dim=" << cube.dim() << " numel=" << cube.numel() << "\n";
```

### Output

```text
matrix dim=2 numel=4
cube dim=3 numel=4
```

Example 3. A zero extent does not erase its axis.

```cpp
const auto empty = pluss::duration::zeros({2,0,3});
std::cout << "dim=" << empty.dim() << " numel=" << empty.numel() << "\n";
```

### Output

```text
dim=3 numel=0
```

## See Also

- [`sizes`](sizes.md): read every extent.
- [`numel`](numel.md): read total cardinality.
- [`host`](host.md): extract lanes in logical row-major order.
