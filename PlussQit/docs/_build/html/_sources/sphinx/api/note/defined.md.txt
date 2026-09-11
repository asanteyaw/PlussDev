# Note::defined

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
bool Note::defined() const noexcept
```

## Ask Whether The Handle Contains A Note

`defined()` distinguishes an actual `Note` value from the sentinel returned by
`Note::undefined()`. It tests the handle itself, not its text, shape, or element
count. A scalar containing `""` is defined. A matrix with a zero-sized axis is
also defined. Only a handle with no backing Note storage reports `false`.

This distinction matters before calling operations that need geometry or cell
storage. `sizes()`, `size()`, indexing, text transformations, and printing
reject an undefined handle. The non-throwing metadata probes `dim()`, `numel()`,
and `empty()` can inspect one, but their fallback values do not turn absence
into a scalar or an empty array. Use `defined()` when that distinction controls
program flow.

The query is constant-time, performs no allocation, never examines cell bytes,
and is safe in validation and cleanup paths because it is `noexcept`.

## Example 1: Undefined, Empty, And Empty Text Are Different

```cpp
const auto absent = pluss::Note::undefined();
const auto empty_text = pluss::note::make("");
const auto empty_matrix = pluss::note::make(
    std::vector<std::string>{}, std::array<std::int64_t, 2>{2, 0});

std::cout << std::boolalpha
          << "absent=" << absent.defined() << '\n'
          << "empty text=" << empty_text.defined() << '\n'
          << "empty matrix=" << empty_matrix.defined() << '\n';
```

### Output

```text
absent=false
empty text=true
empty matrix=true
```

The latter two are usable Notes. One contains an empty string; the other has
valid rank-2 geometry but no cells.

## Example 2: Guard A Geometry-Dependent Operation

```cpp
const auto value = pluss::Note::undefined();
if (!value.defined()) {
  std::cout << "no Note supplied\n";
} else {
  std::cout << value.sizes().size() << '\n';
}
```

### Output

```text
no Note supplied
```

## Example 3: Absence Is Not Rank Zero

```cpp
const auto value = pluss::Note::undefined();
std::cout << std::boolalpha
          << "defined=" << value.defined()
          << " dim=" << value.dim()
          << " scalar=" << value.is_scalar() << '\n';
```

### Output

```text
defined=false dim=0 scalar=false
```

`dim()` uses zero as its non-throwing fallback, so `is_scalar()` additionally
checks that the handle is defined.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="deduplicate.html"><span class="label">Previous</span><span class="title">deduplicate</span></a>
  <a class="next-link" href="diff_chars.html"><span class="label">Next</span><span class="title">diff_chars</span></a>
</div>
<!-- pluss-hand-authored -->
