<!-- pluss-hand-authored -->

# Duration::numel

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
std::int64_t Duration::numel() const
```

## What It Tells You

`numel()` returns the number of logical duration lanes. It is the product of
all extents reported by [`sizes()`](sizes.md); a scalar is the rank-zero case
and contains one lane.

The count includes `NaT` lanes because `NaT` occupies a position. It says
nothing about validity, sign, or uniqueness. A shape with any zero extent has
zero lanes but remains defined and retains its full rank.

## Returns

The logical element count as `std::int64_t`. The query is constant time and
does not materialize or scan duration values.

## Examples

Example 1. Scalar, vector, and matrix cardinalities.

```cpp
std::cout << pluss::duration::seconds(90).numel() << "\n"
          << pluss::duration::minutes({15,30,45}).numel() << "\n"
          << pluss::duration::hours({1,2,3,4}, {2,2}).numel() << "\n";
```

### Output

```text
1
3
4
```

Example 2. Equal cardinality does not imply equal shape.

```cpp
const auto vector = pluss::duration::nanos({1,2,3,4});
const auto cube = pluss::duration::nanos({1,2,3,4}, {2,1,2});
std::cout << "vector dim=" << vector.dim() << " numel=" << vector.numel() << "\n"
          << "cube dim=" << cube.dim() << " numel=" << cube.numel() << "\n";
```

### Output

```text
vector dim=1 numel=4
cube dim=3 numel=4
```

Example 3. One zero extent makes the product zero.

```cpp
const auto empty = pluss::duration::zeros({2,0,3});
std::cout << std::boolalpha << "defined=" << empty.defined()
          << " dim=" << empty.dim() << " numel=" << empty.numel() << "\n";
```

### Output

```text
defined=true dim=3 numel=0
```

## See Also

- [`dim`](dim.md): count axes rather than lanes.
- [`sizes`](sizes.md): inspect the factors producing this count.
- [`isnat`](isnat.md): count or select invalid lanes through a boolean mask.
