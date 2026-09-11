# Note::dim

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
std::int64_t Note::dim() const noexcept
```

## Count Shape Axes, Not Cells

`dim()` returns the rank of a `Note`: the number of axes in its shape. A scalar
has rank `0`, a vector rank `1`, a matrix rank `2`, and the largest supported
Note rank is `3`. Axis lengths do not affect rank, so shape `[2, 0]` remains a
matrix even though it contains no cells.

Rank answers how many indices are required by `at_indices()` and which axes may
be passed to `size()`, `select()`, or `transpose()`. It does not report element
count; use `numel()` for that. It also does not distinguish an undefined handle
from a scalar by itself: both produce the integer `0`, while `defined()` and
`is_scalar()` resolve the difference.

The operation is constant-time, allocation-free, and `noexcept`.

## Example 1: Rank Across The Complete Note Range

```cpp
const auto scalar = pluss::note::make("alpha");
const auto vector = pluss::note::make({"alpha"});
const auto matrix = pluss::note::make(
    std::vector<std::string>{"a", "b", "c", "d", "e", "f"},
    std::array<std::int64_t, 2>{2, 3});
const auto tensor = pluss::note::make(
    std::vector<std::string>{"a", "b", "c", "d", "e", "f", "g", "h"},
    std::array<std::int64_t, 3>{2, 2, 2});

std::cout << scalar.dim() << ' ' << vector.dim() << ' '
          << matrix.dim() << ' ' << tensor.dim() << '\n';
```

### Output

```text
0 1 2 3
```

The singleton vector is still rank `1`; cardinality does not collapse an axis.

## Example 2: Empty Axes Do Not Remove Rank

```cpp
const auto empty = pluss::note::make(
    std::vector<std::string>{}, std::array<std::int64_t, 2>{2, 0});
std::cout << "dim=" << empty.dim()
          << " numel=" << empty.numel()
          << " matrix=" << std::boolalpha << empty.is_matrix() << '\n';
```

### Output

```text
dim=2 numel=0 matrix=true
```

## Example 3: Check Absence Separately

```cpp
const auto absent = pluss::Note::undefined();
std::cout << std::boolalpha
          << "defined=" << absent.defined()
          << " dim=" << absent.dim()
          << " scalar=" << absent.is_scalar() << '\n';
```

### Output

```text
defined=false dim=0 scalar=false
```

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="diff_words.html"><span class="label">Previous</span><span class="title">diff_words</span></a>
  <a class="next-link" href="empty.html"><span class="label">Next</span><span class="title">empty</span></a>
</div>
<!-- pluss-hand-authored -->
