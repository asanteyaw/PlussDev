# Note::sizes

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
std::vector<std::int64_t> Note::sizes() const
```

## Materialize The Complete Shape

`sizes()` returns every axis extent from first to last. Its vector length is
exactly `dim()`, and the product of its entries is `numel()`. A scalar therefore
returns an empty vector: rank zero has no axes even though the scalar contains
one cell. A zero-sized dimension is retained rather than removed.

The returned vector owns its integers. Changing or destroying it cannot change
the Note, and the Note may be destroyed while the vector remains in use. The
cell arena is neither copied nor exposed. This makes `sizes()` appropriate for
persisting geometry or passing it to another constructor; use `size(dim)` to
avoid the small allocation when only one extent is needed.

An undefined handle has no shape and raises `pluss::Error`. Call `defined()`
first when absence is possible.

## Example 1: Scalar Through Rank Three

```cpp
auto print_sizes = [](const pluss::Note& value) {
  const auto shape = value.sizes();
  std::cout << '[';
  for (std::size_t i = 0; i < shape.size(); ++i)
    std::cout << (i ? ", " : "") << shape[i];
  std::cout << "]\n";
};

print_sizes(pluss::note::make("a"));
print_sizes(pluss::note::make({"a"}));
print_sizes(pluss::note::make({{"a", "b"}}));
```

### Output

```text
[]
[1]
[1, 2]
```

## Example 2: The Shape Vector Is An Independent Value

```cpp
const auto note = pluss::note::make({{"a", "b", "c"}, {"d", "e", "f"}});
auto shape = note.sizes();
shape[0] = 99;
std::cout << "copy=[" << shape[0] << ", " << shape[1] << "] "
          << "note=[" << note.size(0) << ", " << note.size(1) << "]\n";
```

### Output

```text
copy=[99, 3] note=[2, 3]
```

## Example 3: Empty Geometry And Undefined Handles

```cpp
const auto empty = pluss::note::make(
    std::vector<std::string>{}, std::array<std::int64_t, 2>{2, 0});
const auto shape = empty.sizes();
std::cout << "empty sizes=[" << shape[0] << ", " << shape[1] << "]\n";

try {
  (void)pluss::Note::undefined().sizes();
} catch (const pluss::Error&) {
  std::cout << "undefined.sizes(): rejected\n";
}
```

### Output

```text
empty sizes=[2, 0]
undefined.sizes(): rejected
```

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="size.html"><span class="label">Previous</span><span class="title">size</span></a>
  <a class="next-link" href="sizes_view.html"><span class="label">Next</span><span class="title">sizes_view</span></a>
</div>
<!-- pluss-hand-authored -->
