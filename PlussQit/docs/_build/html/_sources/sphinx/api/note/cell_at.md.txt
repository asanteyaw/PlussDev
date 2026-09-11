<!-- pluss-hand-authored -->
# Note::cell_at

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
TextRef Note::cell_at(std::span<const std::int64_t> indices) const
```

## Retain One Coordinate-Selected Cell

`cell_at(indices)` combines the shape-aware lookup of
[`at_indices()`](at_indices.md) with the storage ownership of
[`cell()`](cell.md). It converts a complete coordinate to row-major position
and returns a `TextRef` that keeps the packed arena alive. The Note itself is
not copied or modified.

Supply exactly one non-negative index per axis. Each index must be less than
the corresponding extent. Scalars use an empty coordinate; a zero-sized axis
has no valid coordinate. `std::array`, `std::vector`, and `std::span` all work
because the public parameter is a span and is consumed during the call.

Coordinate conversion costs `O(dim())`, with rank limited to three. Retaining
the result is constant-time and allocation-free, but one retained cell keeps
the complete source arena alive. Call `.str()` when a detached copy is the
better memory tradeoff.

## Example 1: Address A Matrix Cell

```cpp
const auto compass = pluss::note::make(
    {{"north", "east"}, {"south", "west"}});
const auto south = compass.cell_at(
    std::array<std::int64_t, 2>{1, 0});
std::cout << south << '\n';
```

### Output

```text
south
```

## Example 2: Keep A Coordinate Result Past The Source

```cpp
pluss::TextRef coordinate;
{
  const auto temporary = pluss::note::make(
      {{"north", "east"}, {"south", "west"}});
  coordinate = temporary.cell_at(
      std::array<std::int64_t, 2>{1, 0});
}
std::cout << "coordinate after source=" << coordinate << '\n';
```

### Output

```text
coordinate after source=south
```

## Example 3: Scalar And Three-Dimensional Coordinates

```cpp
const auto scalar = pluss::note::make("origin");
const auto tensor = pluss::note::make(
    std::vector<std::string>{
        "000", "001", "010", "011", "100", "101", "110", "111"},
    std::array<std::int64_t, 3>{2, 2, 2});

std::cout << scalar.cell_at(std::array<std::int64_t, 0>{}) << ' '
          << tensor.cell_at(std::array<std::int64_t, 3>{1, 1, 0}) << '\n';
```

### Output

```text
origin 110
```

## Example 4: Dynamic Coordinates

```cpp
const auto grid = pluss::note::make({{"a", "b"}, {"c", "d"}});
const std::vector<std::int64_t> coordinate{0, 1};
const auto ref = grid.cell_at(coordinate);
std::cout << ref << '\n';
```

### Output

```text
b
```

The returned `TextRef` does not depend on the coordinate vector after the call.

## Example 5: Rank, Sign, And Bounds Reject

```cpp
const auto grid = pluss::note::make({{"a", "b"}, {"c", "d"}});
auto rejected = [&](auto coordinate) {
  try { (void)grid.cell_at(coordinate); }
  catch (const pluss::Error&) { std::cout << "rejected\n"; }
};
rejected(std::array<std::int64_t, 1>{0});
rejected(std::array<std::int64_t, 2>{0, -1});
rejected(std::array<std::int64_t, 2>{2, 0});
```

### Output

```text
rejected
rejected
rejected
```

## See Also

- [`at_indices`](at_indices.md): the borrowed coordinate result.
- [`cell`](cell.md): owning access by flat position.
- [`select`](select.md): retain an entire coordinate slab as a Note.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cell.html"><span class="label">Previous</span><span class="title">cell</span></a>
  <a class="next-link" href="collapse_newlines.html"><span class="label">Next</span><span class="title">collapse_newlines</span></a>
</div>
