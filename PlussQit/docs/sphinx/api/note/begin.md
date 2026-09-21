<!-- pluss-hand-authored -->
# Note::begin

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note::const_iterator Note::begin() const
```

## Enter The Flat Text Range

`begin()` returns a forward iterator positioned at the first Text or Bytes
cell. It is the entry point used by range-based `for`, standard algorithms,
and explicit iterator loops. Iteration is read-only: dereferencing produces a
`std::string_view`, never a mutable string and never a `Note` slice.

The range is deliberately flat. Rank controls how cells are addressed, not
how a C++ iterator nests. A matrix or rank-three Note is visited in row-major
logical order, exactly the same order used by `at(linear)`, `flatten()`,
`copy_flat_to()`, and `for_each_flat()`.

| Receiver | Number of dereferenceable positions | Order |
|---|---:|---|
| scalar | `1` | its only cell |
| vector | `numel()` | index order |
| matrix or rank three | `numel()` | row-major logical order |
| defined empty Note | `0` | `begin() == end()` |
| undefined Note | `0` | `begin() == end()` |

`begin()` supports Text and Bytes because its value type is
`std::string_view`. For Bool, Int64, and Float64 Notes, traverse the native
spans returned by `bools()`, `integers()`, and `reals()`; attempting to call
`begin()` on those dtypes throws before an iterator is returned.

## Ownership And Validity

The iterator stores an intrusive reference to the Note's backing arena. The
iterator therefore keeps the storage alive even if the original `Note` handle
is destroyed. A `std::string_view` obtained by dereferencing is still a
borrowed view: keep either the iterator or a Note sharing that arena alive
while using the view. Construct `std::string(*it)` when independent ownership
is required.

Incrementing a past-the-end iterator or dereferencing `end()` is invalid.
Comparisons are meaningful only for iterators into the same backing Note.

## Example 1: Range-Based Iteration

```cpp
const auto words = pluss::note::make({"north", "east", "south", "west"});
std::cout << '[';
std::size_t index = 0;
for (const std::string_view word : words)
  std::cout << (index++ ? ", " : "") << '"' << word << '"';
std::cout << "] count=" << std::distance(words.begin(), words.end()) << '\n';
```

### Output

```text
["north", "east", "south", "west"] count=4
```

## Example 2: Higher Ranks Flatten Row-Major

```cpp
const auto grid = pluss::note::make(
    std::vector<std::string>{"r0c0", "r0c1", "r0c2", "r1c0", "r1c1", "r1c2"},
    std::array<std::int64_t, 2>{2, 3});

for (const auto cell : grid) std::cout << cell << ' ';
std::cout << '\n';
```

### Output

```text
r0c0 r0c1 r0c2 r1c0 r1c1 r1c2 
```

The iterator does not emit row objects. Use `matrix()` when each row must
remain a Note.

## Example 3: Scalar And Empty Ranges

```cpp
const auto scalar = pluss::note::make("solo");
const auto empty = pluss::note::make(std::vector<std::string>{});
std::cout << "scalar distance=" << std::distance(scalar.begin(), scalar.end())
          << " value=" << *scalar.begin() << '\n'
          << "empty distance=" << std::distance(empty.begin(), empty.end())
          << " equal=" << std::boolalpha << (empty.begin() == empty.end()) << '\n';
```

### Output

```text
scalar distance=1 value=solo
empty distance=0 equal=true
```

## Example 4: An Iterator Keeps Its Arena Alive

```cpp
auto kept = [] {
  const auto temporary = pluss::note::make({"kept", "alive"});
  return temporary.begin();
}();
std::cout << *kept << ' ';
++kept;
std::cout << *kept << '\n';
```

### Output

```text
kept alive
```

## Example 5: Native Dtypes Use Native Spans

```cpp
const auto values = pluss::note::make(std::vector<std::int64_t>{3, 1, 4});
try { (void)values.begin(); }
catch (const pluss::Error&) { std::cout << "Int64 begin: rejected\n"; }
std::cout << "integers=";
for (const auto value : values.integers()) std::cout << value << ' ';
std::cout << '\n';
```

### Output

```text
Int64 begin: rejected
integers=3 1 4 
```

## See Also

- [`end`](end.md): obtain the non-dereferenceable range sentinel.
- [`copy_flat_to`](copy_flat_to.md): send every borrowed cell to an output iterator.
- [`for_each_flat`](for_each_flat.md): invoke a callback without writing an iterator loop.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="base64_encode.html"><span class="label">Previous</span><span class="title">base64_encode</span></a>
  <a class="next-link" href="bool_item.html"><span class="label">Next</span><span class="title">bool_item</span></a>
</div>
