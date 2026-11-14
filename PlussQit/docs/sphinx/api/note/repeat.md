<!-- pluss-hand-authored -->
# Note::repeat

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::repeat(pluss::RangeView<std::int64_t> repeats) const
```

## Copy Complete Patterns Across Dimensions

`repeat(repeats)` constructs a new Note by repeating the receiver's complete
value pattern along each output axis. It changes structure, not cell content.
For a vector `{"a", "b"}`, `repeat({2})` produces
`{"a", "b", "a", "b"}`; it does not produce
`{"a", "a", "b", "b"}`. The latter is the job of
[`repeat_interleave`](repeat_interleave.md).

Do not confuse structural repetition with text multiplication from
[`operators`](operators.md). `note * 3` keeps the same shape and repeats bytes
inside each cell. `note.repeat({3})` triples the axis extent and copies cells.

## Shape Calculation

`repeats` supplies one nonnegative copy count per output axis. Its length must
be at least the receiver rank and at most three, the maximum Note rank. When it
contains more entries than the receiver has axes, conceptual leading axes of
extent one are prepended to the receiver.

For output rank `r`, let `padded_shape` be the receiver shape left-padded with
ones to length `r`. Then:

```text
output_shape[d] = padded_shape[d] * repeats[d]
```

| Receiver shape | Repeats | Output shape |
|---|---|---|
| `[2]` | `{3}` | `[6]` |
| `[2, 2]` | `{2, 1}` | `[4, 2]` |
| `[2]` | `{2, 3}` | `[2, 6]` |
| `[]` | `{2, 3}` | `[2, 3]` |
| `[]` | `{}` | `[]` |

Each product and the final cardinality are checked in signed 64-bit arithmetic
before output allocation. A negative count, too few dimensions, rank above
three, or overflowing shape rejects.

A zero count makes the corresponding output extent zero. The result remains a
defined Note with the calculated shape and receiver dtype, but contains no
lanes. Undefined input propagates as undefined before repeat-list validation.

## Storage And Cost

Every result is an independent Note. For Text and Bytes, repetition allocates
new span metadata while all spans continue to reference the receiver's packed
arena; cell bytes are not duplicated. Bool, Int64, and Float64 allocate new
aligned contiguous lane storage because their values are stored directly.

The kernel writes destination storage directly from calculated source
coordinates. It does not allocate an additional output-sized index array.
Runtime and destination storage are `O(output.numel())`; text payload size does
not multiply, but text span metadata does.

## Example 1: Repeat A Vector Pattern

```cpp
const auto values = pluss::note::make({"a", "b"});
std::cout << pluss::note::print(values.repeat({2})) << '\n';
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["a", "b", "a", "b"]
```

## Example 2: Repeat Complete Matrix Rows

```cpp
const auto matrix = pluss::note::make(
    std::vector<std::string>{"a", "b", "c", "d"},
    std::array<std::int64_t, 2>{2, 2});
std::cout << pluss::note::print(matrix.repeat({2, 1})) << '\n';
```

### Output

```text
Note(shape=[4, 2], dim=2, numel=8, encoding=utf8)
[
  ["a", "b"], 
  ["c", "d"], 
  ["a", "b"], 
  ["c", "d"]
]
```

The first count repeats the entire two-row block. A count of one leaves the
column axis unchanged.

## Example 3: Repeat The Column Pattern

```cpp
const auto matrix = pluss::note::make(
    std::vector<std::string>{"a", "b", "c", "d"},
    std::array<std::int64_t, 2>{2, 2});
std::cout << pluss::note::print(matrix.repeat({1, 2})) << '\n';
```

### Output

```text
Note(shape=[2, 4], dim=2, numel=8, encoding=utf8)
[
  ["a", "b", "a", "b"], 
  ["c", "d", "c", "d"]
]
```

Within each row, the complete two-column sequence repeats.

## Example 4: Add A Leading Dimension

```cpp
const auto values = pluss::note::make({"a", "b"});
std::cout << pluss::note::print(values.repeat({2, 2})) << '\n';
```

### Output

```text
Note(shape=[2, 4], dim=2, numel=8, encoding=utf8)
[
  ["a", "b", "a", "b"], 
  ["a", "b", "a", "b"]
]
```

The vector shape `[2]` is padded to `[1, 2]`, then multiplied by `{2, 2}`.

## Example 5: Scalar Identity And Scalar Expansion

```cpp
const auto scalar = pluss::note::make("x");
std::cout << "identity\n" << pluss::note::print(scalar.repeat({})) << '\n';
std::cout << "matrix\n" << pluss::note::print(scalar.repeat({2, 3})) << '\n';
```

### Output

```text
identity
Note(shape=[], dim=0, numel=1, encoding=utf8)
"x"
matrix
Note(shape=[2, 3], dim=2, numel=6, encoding=utf8)
[
  ["x", "x", "x"], 
  ["x", "x", "x"]
]
```

An empty repeat list is valid only for a scalar because its length still must
not be smaller than the receiver rank.

## Example 6: Zero Produces A Defined Empty Shape

```cpp
const auto matrix = pluss::note::make(
    std::vector<std::string>{"a", "b", "c", "d"},
    std::array<std::int64_t, 2>{2, 2});
std::cout << pluss::note::print(matrix.repeat({0, 2})) << '\n';
```

### Output

```text
Note(shape=[0, 4], dim=2, numel=0, encoding=utf8)
[]
```

## Example 7: Native Dtypes Remain Native

```cpp
std::cout << "Bool\n"
          << pluss::note::print(pluss::note::make({true, false}).repeat({2}))
          << '\n';
std::cout << "Int64\n"
          << pluss::note::print(
                              pluss::note::make(std::vector<std::int64_t>{1, 2}).repeat({2}))
          << '\n';
```

### Output

```text
Bool
Note(shape=[4], dim=1, numel=4, dtype=Bool)
[true, false, true, false]
Int64
Note(shape=[4], dim=1, numel=4, dtype=Int64)
[1, 2, 1, 2]
```

No formatting to `"true"`, `"1"`, or `"2"` occurs.

## Example 8: Bytes Preserve Raw Encoding

```cpp
const auto raw = pluss::note::bytes(
    std::vector<std::string>{"A", "B"},
    std::array<std::int64_t, 1>{2});
std::cout << pluss::note::print(raw.repeat({2})) << '\n';
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=bytes)
[b"A", b"B", b"A", b"B"]
```

## Example 9: Undefined Propagates

```cpp
std::cout << std::boolalpha
          << "defined=" << pluss::Note::undefined().repeat({2}).defined()
          << '\n';
```

### Output

```text
defined=false
```

## Example 10: Invalid Repeat Specifications Reject

```cpp
#include <limits>

const auto vector = pluss::note::make({"a", "b"});
const auto matrix = vector.repeat({2, 1});

try { (void)matrix.repeat({2}); }
catch (const pluss::Error&) { std::cout << "too few dimensions: rejected\n"; }
try { (void)vector.repeat({-1}); }
catch (const pluss::Error&) { std::cout << "negative: rejected\n"; }
try { (void)vector.repeat({1, 1, 1, 1}); }
catch (const pluss::Error&) { std::cout << "rank four: rejected\n"; }
try {
  (void)pluss::note::make("x").repeat(
      {std::numeric_limits<std::int64_t>::max(), 2});
} catch (const pluss::Error&) { std::cout << "overflow: rejected\n"; }
```

### Output

```text
too few dimensions: rejected
negative: rejected
rank four: rejected
overflow: rejected
```

## See Also

- [`repeat_interleave`](repeat_interleave.md): repeat each lane consecutively
  along one selected axis.
- [`operators`](operators.md): repeat bytes inside each cell with `operator*`.
- [`reshape`](reshape.md): relabel existing lanes without copying them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="remove_symbols.html"><span class="label">Previous</span><span class="title">remove_symbols</span></a>
  <a class="next-link" href="repeat_interleave.html"><span class="label">Next</span><span class="title">repeat_interleave</span></a>
</div>
