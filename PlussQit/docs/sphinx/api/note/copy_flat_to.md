<!-- pluss-hand-authored -->
# Note::copy_flat_to

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
template<std::output_iterator<std::string_view> OutIt>
void Note::copy_flat_to(OutIt out) const
```

## Copy Borrowed Cells Into An Output Sequence

`copy_flat_to()` writes every Text or Bytes cell, as a `std::string_view`, to
an output iterator. It is the direct bridge for appending a Note's flattened
cell views to a caller-owned sequence without first allocating the temporary
`std::vector<std::string_view>` returned by `vector()`.

The word *copy* refers to copying each small view object into the destination;
the cell bytes are not copied. A destination such as
`std::vector<std::string_view>` therefore remains borrowed from the source
Note. Keep a Note sharing the arena alive while consuming those views.

The output iterator must accept `std::string_view` directly. To create owning
`std::string` values, use `std::transform` and construct each string
explicitly. This makes the ownership transition visible at the call site
rather than hiding potentially large allocations behind `copy_flat_to()`.

## Traversal And Cost

Cells are emitted once in row-major logical order. Scalar, vector, matrix, and
rank-three receivers therefore produce `numel()` writes; shape is not encoded
in the destination. The operation is `O(numel())`, performs no allocation of
its own, and does not decode Text or inspect Bytes. Embedded zero bytes are
preserved because each view carries an explicit length.

A defined empty Note performs no writes. An undefined Note and every native
Bool, Int64, or Float64 Note are rejected before the first output assignment,
so the destination is unchanged on those validation failures. For native
dtypes, copy from `bools()`, `integers()`, or `reals()` instead.

## Example 1: Append Borrowed Views

```cpp
const auto words = pluss::note::make({"north", "east", "south", "west"});
std::vector<std::string_view> borrowed;
words.copy_flat_to(std::back_inserter(borrowed));

std::cout << '[';
for (std::size_t i = 0; i < borrowed.size(); ++i)
  std::cout << (i ? ", " : "") << '"' << borrowed[i] << '"';
std::cout << "]\n";
```

### Output

```text
["north", "east", "south", "west"]
```

## Example 2: Make Ownership Explicit

```cpp
const auto words = pluss::note::make({"north", "east", "south", "west"});
std::vector<std::string> owning;
std::transform(words.begin(), words.end(), std::back_inserter(owning),
               [](std::string_view value) { return std::string(value); });

std::cout << '[';
for (std::size_t i = 0; i < owning.size(); ++i)
  std::cout << (i ? ", " : "") << '"' << owning[i] << '"';
std::cout << "]\n";
```

### Output

```text
["north", "east", "south", "west"]
```

The output looks identical, but the strings now own their bytes and remain
valid independently of `words`.

## Example 3: Append Instead Of Replace

```cpp
const auto words = pluss::note::make({"south", "west"}); // keep this alive
std::vector<std::string_view> out{"north", "east"};
words.copy_flat_to(std::back_inserter(out));
for (const auto value : out) std::cout << value << ' ';
std::cout << '\n';
```

### Output

```text
north east south west 
```

`copy_flat_to()` starts at the output iterator supplied by the caller; it
does not clear or resize a container itself. Do not replace `words` above
with a temporary expression: the appended views would outlive their arena at
the semicolon and become dangling.

## Example 4: Bytes Remain Length-Aware

```cpp
const std::array<std::byte, 5> raw{
    std::byte{'A'}, std::byte{0}, std::byte{'B'}, std::byte{0x7f}, std::byte{0xff}};
const auto bytes = pluss::note::bytes(raw);
std::vector<std::string_view> cells;
bytes.copy_flat_to(std::back_inserter(cells));
std::cout << "size=" << cells[0].size() << " hex=";
for (const unsigned char value : cells[0])
  std::cout << std::hex << std::setfill('0') << std::setw(2)
            << static_cast<unsigned int>(value);
std::cout << '\n';
```

### Output

```text
size=5 hex=4100427fff
```

## Example 5: Undefined And Typed Notes Reject

```cpp
std::vector<std::string_view> out;
try { pluss::Note::undefined().copy_flat_to(std::back_inserter(out)); }
catch (const pluss::Error&) { std::cout << "undefined copy: rejected\n"; }

const auto ints = pluss::note::make(std::vector<std::int64_t>{3, 1, 4});
try { ints.copy_flat_to(std::back_inserter(out)); }
catch (const pluss::Error&) { std::cout << "Int64 copy: rejected\n"; }
```

### Output

```text
undefined copy: rejected
Int64 copy: rejected
```

## See Also

- [`begin`](begin.md): use general standard algorithms over the same flat range.
- [`for_each_flat`](for_each_flat.md): consume cells without constructing an output iterator.
- [`host`](host.md): request an owning, dtype-aware host representation.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="contiguous.html"><span class="label">Previous</span><span class="title">contiguous</span></a>
  <a class="next-link" href="count_substring.html"><span class="label">Next</span><span class="title">count_substring</span></a>
</div>
