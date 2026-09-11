# Note::numel

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
std::int64_t Note::numel() const noexcept
```

## Count Cells Across Every Axis

`numel()` returns the total number of text cells, independent of rank. For a
defined Note it is the product of all entries in `sizes()`. The empty product
for a scalar is one, so rank zero still contains one cell. If any extent is
zero, the product is zero.

Use this query to size flat result buffers, bound `at()` or `flat_at()` loops,
and reason about methods that pair Notes by flattened cardinality. Use `dim()`
or `sizes()` when axis structure matters. Text length is a different property:
`lengths_bytes()` and `lengths_chars()` measure each cell's contents.

An undefined handle reports zero without throwing. Therefore `numel() == 0`
means “no iterable cells,” not necessarily “a defined empty Note”; combine it
with `defined()` when absence is semantically distinct. The query is
constant-time, allocation-free, and `noexcept`.

## Example 1: Rank Does Not Determine Cardinality

```cpp
const auto scalar = pluss::note::make("alpha");
const auto singleton = pluss::note::make({"alpha"});
const auto matrix = pluss::note::make({{"a", "b", "c"}, {"d", "e", "f"}});

std::cout << "scalar=" << scalar.numel()
          << " singleton=" << singleton.numel()
          << " matrix=" << matrix.numel() << '\n';
```

### Output

```text
scalar=1 singleton=1 matrix=6
```

The first two have equal cardinality but different ranks and shapes.

## Example 2: Zero Extents Make The Product Zero

```cpp
const auto empty = pluss::note::make(
    std::vector<std::string>{}, std::array<std::int64_t, 3>{4, 0, 7});
std::cout << "dim=" << empty.dim()
          << " numel=" << empty.numel()
          << " empty=" << std::boolalpha << empty.empty() << '\n';
```

### Output

```text
dim=3 numel=0 empty=true
```

## Example 3: Cardinality Does Not Resolve Absence

```cpp
const auto empty = pluss::note::make(
    std::vector<std::string>{}, std::array<std::int64_t, 1>{0});
const auto absent = pluss::Note::undefined();
std::cout << std::boolalpha
          << "empty: defined=" << empty.defined() << " numel=" << empty.numel() << '\n'
          << "absent: defined=" << absent.defined() << " numel=" << absent.numel() << '\n';
```

### Output

```text
empty: defined=true numel=0
absent: defined=false numel=0
```

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="normalize_quotes.html"><span class="label">Previous</span><span class="title">normalize_quotes</span></a>
  <a class="next-link" href="overlap_acronyms.html"><span class="label">Next</span><span class="title">overlap_acronyms</span></a>
</div>
<!-- pluss-hand-authored -->
