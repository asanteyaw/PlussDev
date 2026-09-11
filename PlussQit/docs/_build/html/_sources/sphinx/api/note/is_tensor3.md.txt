# Note::is_tensor3

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
bool Note::is_tensor3() const noexcept
```

## Test For The Maximum Supported Rank

`is_tensor3()` reports whether a defined Note has three axes. Rank three is the
largest Note geometry and commonly represents batches of text matrices: axis
zero selects a batch, axis one a row, and axis two a column. The names assigned
to those axes remain application-defined; the predicate checks only rank.

Singleton and zero-sized axes still count. A shape such as `[1, 2, 0]` is a
rank-3 Note with no cells. Scalars, vectors, matrices, and undefined handles
return false. The method is constant-time, allocation-free, and `noexcept`.

## Example 1: A Dense Rank-Three Note

```cpp
const auto value = pluss::note::make(
    std::vector<std::string>{"a", "b", "c", "d", "e", "f", "g", "h"},
    std::array<std::int64_t, 3>{2, 2, 2});
std::cout << std::boolalpha
          << "tensor3=" << value.is_tensor3()
          << " dim=" << value.dim()
          << " numel=" << value.numel() << '\n';
```

### Output

```text
tensor3=true dim=3 numel=8
```

## Example 2: Zero Extents Do Not Change Rank

```cpp
const auto value = pluss::note::make(
    std::vector<std::string>{}, std::array<std::int64_t, 3>{1, 2, 0});
std::cout << std::boolalpha
          << "tensor3=" << value.is_tensor3()
          << " empty=" << value.empty()
          << " last=" << value.size(-1) << '\n';
```

### Output

```text
tensor3=true empty=true last=0
```

## Example 3: A Matrix Is Not Promoted Implicitly

```cpp
const auto matrix = pluss::note::make({{"a", "b"}});
std::cout << std::boolalpha
          << "matrix=" << matrix.is_matrix()
          << " tensor3=" << matrix.is_tensor3() << '\n';
```

### Output

```text
matrix=true tensor3=false
```

Use `unsqueeze(dim)` when a real new axis is required; a predicate never changes
the Note.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_scalar.html"><span class="label">Previous</span><span class="title">is_scalar</span></a>
  <a class="next-link" href="is_title.html"><span class="label">Next</span><span class="title">is_title</span></a>
</div>
<!-- pluss-hand-authored -->
