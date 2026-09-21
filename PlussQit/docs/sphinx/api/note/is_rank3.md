<!-- pluss-hand-authored -->
# Note::is_rank3

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
bool Note::is_rank3() const noexcept
```

## Ask About Geometry, Not Payload

`is_rank3()` reports whether a defined Note has exactly three axes. Rank three
is the largest geometry supported by Note and is often used for a sequence of
matrix-shaped text or typed-lane blocks. The method does not assign meaning to
those axes and does not inspect, convert, or allocate cell data.

Rank and cardinality are independent. Shapes `[1, 1, 1]`, `[2, 0, 3]`, and
`[0, 4, 5]` are all rank three even though the latter two contain no cells. A
matrix with one cell remains rank two. Undefined handles return `false`, which
makes this predicate safe in validation branches without a preceding
`defined()` check.

The operation is constant-time, allocation-free, and `noexcept`. Use `dim()`
when the numeric rank itself is needed, or `slabs()` when a confirmed rank-three
Note must be decomposed into rank-two Note views.

## Example 1: A Dense Rank-Three Note

```cpp
const auto value = pluss::note::make(
    std::vector<std::string>{"a", "b", "c", "d", "e", "f", "g", "h"},
    std::array<std::int64_t, 3>{2, 2, 2});
std::cout << std::boolalpha
          << "rank3=" << value.is_rank3()
          << " dim=" << value.dim()
          << " numel=" << value.numel() << '\n';
```

### Output

```text
rank3=true dim=3 numel=8
```

## Example 2: Zero Extents Do Not Change Rank

```cpp
const auto value = pluss::note::make(
    std::vector<std::string>{}, std::array<std::int64_t, 3>{1, 2, 0});
std::cout << std::boolalpha
          << "rank3=" << value.is_rank3()
          << " empty=" << value.empty()
          << " last=" << value.size(-1) << '\n';
```

### Output

```text
rank3=true empty=true last=0
```

## Example 3: A Matrix Is Not Promoted Implicitly

```cpp
const auto matrix = pluss::note::make({{"a", "b"}});
std::cout << std::boolalpha
          << "matrix=" << matrix.is_matrix()
          << " rank3=" << matrix.is_rank3() << '\n';
```

### Output

```text
matrix=true rank3=false
```

## Example 4: Undefined Handles Are Safely False

```cpp
const auto absent = pluss::Note::undefined();
std::cout << std::boolalpha
          << "defined=" << absent.defined()
          << " rank3=" << absent.is_rank3() << '\n';
```

### Output

```text
defined=false rank3=false
```

`is_rank3()` is a query only. Use `unsqueeze()`, `squeeze()`, or `reshape()`
when a deliberate geometry change is required.

## See Also

- [`dim`](dim.md): return the numeric rank.
- [`is_matrix`](is_matrix.md): test for exactly two axes.
- [`is_scalar`](is_scalar.md): test for rank zero.
- [`slabs`](slabs.md): decompose rank three into owning rank-two views.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_matrix.html"><span class="label">Previous</span><span class="title">is_matrix</span></a>
  <a class="next-link" href="is_scalar.html"><span class="label">Next</span><span class="title">is_scalar</span></a>
</div>
