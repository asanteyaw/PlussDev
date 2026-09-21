<!-- pluss-hand-authored -->
# Note::host

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note::HostAny Note::host() const
```

## Materialize An Owning Standard-Library Representation

`host()` copies every lane into its matching native C++ storage and returns a
rank-aware `Note::HostAny`. It is the explicit escape hatch for code that must
own ordinary C++ values independently of a Note. Text is copied to
`std::string`; Bool remains Bool lanes; Int64 remains `std::int64_t`; and
Float64 remains `double`. No numeric value is formatted as text during export.

The payload form is selected from the receiver's rank:

| Note rank | `HostAny::Kind` | Accessor | Stored representation |
|---|---|---|---|
| 0 | `Scalar` | dtype-specific scalar accessor | native scalar |
| 1 | `Vector` | dtype-specific vector accessor | native contiguous vector |
| 2 or 3 | `Array` | `array()` | flat dtype-specific lanes plus exact extents |

The text accessors retain their established names, `scalar()` and `vector()`.
Native payloads use `bool_scalar()`, `int_scalar()`, `float_scalar()`,
`bool_vector()`, `int_vector()`, and `float_vector()`. `dtype()` identifies the
active alternative before dispatch. For rank two and three, `HostArray::data`
stores Text or Bytes, while `bool_data`, `int_data`, or `float_data` stores the
corresponding native dtype.

`HostArray::data` is flat rather than recursively nested. Its `sizes` preserve
the original geometry, and `shape()`, `dim()`, and `numel()` query that copied
payload. `contiguous` is `true` because host extraction always writes a fresh
row-major array. A rank-2 or rank-3 Note stays an `Array` even when an extent is
zero; a rank-1 empty Note remains a `Vector`.

Each kind accessor validates the active payload and throws `pluss::Error` when
the wrong one is requested. Check `is_scalar()`, `is_vector()`, or `is_array()`
before branching. Calling `host()` on an undefined Note also throws because
there is no rank or storage to materialize.

The operation allocates and copies `O(numel() + total text bytes)`. It preserves
embedded zero bytes and raw-byte content because `std::string` is length-aware;
it does not transcode, parse, or validate. Prefer `matrix()` or `slabs()` when
the goal is decomposition without copying.

## Example 1: Scalar Payload

```cpp
const auto host = pluss::note::make("alpha").host();
std::cout << std::boolalpha
          << "scalar kind=" << host.is_scalar()
          << " value=" << host.scalar() << '\n';
```

### Output

```text
scalar kind=true value=alpha
```

The string remains owned by `host`, not by the temporary Note.

## Example 2: Vector Payload

```cpp
const auto host = pluss::note::make({"a", "b"}).host();
std::cout << std::boolalpha
          << "vector kind=" << host.is_vector()
          << " size=" << host.vector().size()
          << " last=" << host.vector().back() << '\n';
```

### Output

```text
vector kind=true size=2 last=b
```

## Example 3: Matrix Payload Is Flat Plus Shape

```cpp
const auto host = pluss::note::make(
    std::vector<std::string>{"a", "b", "c", "d"},
    std::array<std::int64_t, 2>{2, 2}).host();
const auto& array = host.array();

std::cout << std::boolalpha
          << "array kind=" << host.is_array()
          << " shape=[" << array.sizes[0] << ", " << array.sizes[1] << ']'
          << " values=[\"" << array.data[0] << "\", \"" << array.data[1]
          << "\", \"" << array.data[2] << "\", \"" << array.data[3] << "\"]"
          << " contiguous=" << array.contiguous << '\n';
```

### Output

```text
array kind=true shape=[2, 2] values=["a", "b", "c", "d"] contiguous=true
```

Cell `[row, column]` occupies `data[row * sizes[1] + column]`.

## Example 4: Three-Dimensional Geometry

```cpp
const auto host = pluss::note::make(
    std::vector<std::string>{"000", "001", "010", "011"},
    std::array<std::int64_t, 3>{1, 2, 2}).host();
const auto& array = host.array();
std::cout << "dim=" << array.dim()
          << " numel=" << array.numel()
          << " shape=[" << array.sizes[0] << ", " << array.sizes[1]
          << ", " << array.sizes[2] << "]\n";
```

### Output

```text
dim=3 numel=4 shape=[1, 2, 2]
```

## Example 5: Empty Geometry Retains Its Rank

```cpp
const auto host = pluss::note::make(
    std::vector<std::string>{},
    std::array<std::int64_t, 2>{3, 0}).host();
std::cout << "shape=[" << host.array().sizes[0] << ", "
          << host.array().sizes[1] << "] values="
          << host.array().numel() << '\n';
```

### Output

```text
shape=[3, 0] values=0
```

This is an empty matrix, not an empty vector and not an absent Note.

## Example 6: The Copy Survives And Can Be Duplicated

```cpp
pluss::Note::HostAny retained;
{
  const auto temporary = pluss::note::make({"kept", "copy"});
  retained = temporary.host();
}
auto editable = retained.vector();
editable[0] = "changed";

std::cout << retained.vector()[0] << ' ' << editable[0] << '\n';
```

### Output

```text
kept changed
```

The accessor itself returns a const reference. Copy it into a mutable standard
container when edits are needed.

## Example 7: Wrong Kinds And Undefined Notes Reject

```cpp
try { (void)pluss::Note::undefined().host(); }
catch (const pluss::Error&) { std::cout << "host undefined: rejected\n"; }

const auto vector_host = pluss::note::make({"a", "b"}).host();
try { (void)vector_host.scalar(); }
catch (const pluss::Error&) { std::cout << "wrong host accessor: rejected\n"; }
```

### Output

```text
host undefined: rejected
wrong host accessor: rejected
```

## Example 8: Numeric Export Does Not Pass Through Text

```cpp
const auto scalar = pluss::note::make(42).host();
const auto values = pluss::note::make({0.25, 0.5, 0.75}).host();

std::cout << "integer=" << scalar.int_scalar()
          << " dtype=" << pluss::coden::dtype_name(scalar.dtype()) << '\n'
          << "last=" << values.float_vector().back()
          << " dtype=" << pluss::coden::dtype_name(values.dtype()) << '\n';
```

### Output

```text
integer=42 dtype=Int64
last=0.75 dtype=Float64
```

## See Also

- [`at`](at.md): borrow one cell without copying.
- [`cell`](cell.md): retain one cell by sharing the source arena.
- [`sizes`](sizes.md): inspect geometry without copying cell content.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="hex_encode.html"><span class="label">Previous</span><span class="title">hex_encode</span></a>
  <a class="next-link" href="html_escape.html"><span class="label">Next</span><span class="title">html_escape</span></a>
</div>
