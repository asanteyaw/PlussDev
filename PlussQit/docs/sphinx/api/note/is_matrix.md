# Note::is_matrix

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
bool Note::is_matrix() const noexcept
```

## Test For Two-Dimensional Geometry

`is_matrix()` reports whether a defined Note has exactly two axes. It is useful
before row-and-column logic, but it says nothing about the lengths of those
axes: `[2, 3]`, `[1, 1]`, and `[2, 0]` are all matrices.

The method does not inspect cell text or allocate. Scalars, vectors, rank-3
Notes, and undefined handles return false. When exact geometry matters, follow
the predicate with `size(0)` and `size(1)`.

## Example 1: Matrix Shape

```cpp
const auto value = pluss::note::make({{"a", "b", "c"}, {"d", "e", "f"}});
std::cout << std::boolalpha
          << "matrix=" << value.is_matrix()
          << " rows=" << value.size(0)
          << " columns=" << value.size(1) << '\n';
```

### Output

```text
matrix=true rows=2 columns=3
```

## Example 2: Singleton Axes Do Not Lower Rank

```cpp
const auto value = pluss::note::make(
    std::vector<std::string>{"only"}, std::array<std::int64_t, 2>{1, 1});
std::cout << std::boolalpha
          << "matrix=" << value.is_matrix()
          << " scalar=" << value.is_scalar()
          << " numel=" << value.numel() << '\n';
```

### Output

```text
matrix=true scalar=false numel=1
```

## Example 3: Zero Columns Preserve Matrix Identity

```cpp
const auto value = pluss::note::make(
    std::vector<std::string>{}, std::array<std::int64_t, 2>{2, 0});
std::cout << std::boolalpha
          << "matrix=" << value.is_matrix()
          << " empty=" << value.empty()
          << " numel=" << value.numel() << '\n';
```

### Output

```text
matrix=true empty=true numel=0
```

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_lower.html"><span class="label">Previous</span><span class="title">is_lower</span></a>
  <a class="next-link" href="is_scalar.html"><span class="label">Next</span><span class="title">is_scalar</span></a>
</div>
<!-- pluss-hand-authored -->
