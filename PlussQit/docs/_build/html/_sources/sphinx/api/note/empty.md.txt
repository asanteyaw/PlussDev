# Note::empty

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
bool Note::empty() const noexcept
```

## Test Whether There Are Any Cells

`empty()` is the cardinality predicate `numel() == 0`. For a defined Note it
becomes true when at least one shape extent is zero. It does not inspect cell
contents: a scalar containing the empty string has one cell and therefore is
not empty.

An undefined handle also reports `true` because it has no elements. That makes
`empty()` convenient for loops that only care whether iteration can begin, but
insufficient for APIs where “not supplied” and “supplied with zero cells” mean
different things. Pair it with `defined()` in those cases.

The query is constant-time, allocation-free, and `noexcept`.

## Example 1: Empty Text Is Still One Cell

```cpp
const auto text = pluss::note::make("");
std::cout << std::boolalpha
          << "empty=" << text.empty()
          << " numel=" << text.numel()
          << " scalar=" << text.is_scalar() << '\n';
```

### Output

```text
empty=false numel=1 scalar=true
```

## Example 2: Zero Extents Preserve Their Geometry

```cpp
const auto value = pluss::note::make(
    std::vector<std::string>{}, std::array<std::int64_t, 2>{2, 0});
std::cout << std::boolalpha
          << "empty=" << value.empty()
          << " dim=" << value.dim()
          << " sizes=[" << value.size(0) << ", " << value.size(1) << "]\n";
```

### Output

```text
empty=true dim=2 sizes=[2, 0]
```

## Example 3: Distinguish Empty From Absent

```cpp
const auto empty_value = pluss::note::make(
    std::vector<std::string>{}, std::array<std::int64_t, 1>{0});
const auto absent = pluss::Note::undefined();
std::cout << std::boolalpha
          << "empty value: defined=" << empty_value.defined()
          << " empty=" << empty_value.empty() << '\n'
          << "absent: defined=" << absent.defined()
          << " empty=" << absent.empty() << '\n';
```

### Output

```text
empty value: defined=true empty=true
absent: defined=false empty=true
```

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dtype.html"><span class="label">Previous</span><span class="title">dtype</span></a>
  <a class="next-link" href="end.html"><span class="label">Next</span><span class="title">end</span></a>
</div>
<!-- pluss-hand-authored -->
