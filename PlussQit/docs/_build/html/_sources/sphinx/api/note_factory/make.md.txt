<!-- pluss-hand-authored -->
# pluss::note::make

[Back to Note Factory API Reference](../../note_factory_api_reference)

## Signatures

```cpp
Note pluss::note::make(text_source values)
Note pluss::note::make(string_range flat, integer_range sizes)
Note pluss::note::make(bool value)
Note pluss::note::make(signed_integer value)
Note pluss::note::make(floating_point value)
Note pluss::note::make(native_values)
Note pluss::note::make(native_values, integer_range sizes)
Note pluss::make_note(text_source values)
```

## The General Typed Entrance To `Note`

`make()` constructs a homogeneous Note from text, Bool, signed integer, or
floating-point input. The source category selects `Text`, `Bool`, `Int64`, or
`Float64`; numeric input is converted once into the Note's canonical native
lane width and is never encoded as text. This makes construction the beginning
of a closed Note pipeline rather than an escape into unrelated containers.

For native values, a scalar produces rank zero, a vector or braced list
produces rank one, and `(flat, sizes)` supplies rank-two or rank-three geometry.
All signed integer widths converge to Int64 and all floating widths converge to
Float64. Bool is stored as contiguous canonical `0`/`1` lanes rather than
`std::vector<bool>` proxies. Unsigned integers are deliberately not accepted:
silently narrowing their full range into Int64 would violate the accuracy-first
contract.

`text_source` is a single converted parameter that accepts every reasonable
call-site spelling of "some text":

- a bare string literal, `std::string_view`, `std::string`, or a single
  `char` — one scalar cell, rank `0`
- a braced list of strings — a rank-`1` column
- a nested braced list, two or three levels deep — a rank-`2` or rank-`3`
  block
- a contiguous range of `std::string` or `std::string_view` — a rank-`1`
  column

The shape is implied by the call form: nesting depth becomes rank, and the
lengths at each level become the sizes. Text is stored as-is, cell by cell;
`make()` does not trim, normalize, case-fold, or re-encode. Multi-byte UTF-8
(accents, CJK, emoji) is preserved intact.

The **text overloads** always produce `NoteDType::Text`
(`encoding=utf8`). They label the content as text but do not validate that
every byte is well-formed UTF-8. If the source bytes are not valid text, use
[`bytes`](bytes.md) instead. Bool, signed-integer, and floating-point overloads
produce their native dtypes as described above; they do not pass through this
text path.

### The `(flat, sizes)` overload

When text arrives already flattened from another system — a `std::vector` of
cells plus a separate shape — pass the two ranges directly:

```cpp
pluss::note::make(flat_range, sizes_range)
```

Two call-site arguments cannot fold into the single converted `text_source`
parameter, so this is a distinct overload. The flat count must equal the
product of `sizes`; a mismatch is rejected before a `Note` is built.

### `pluss::make_note`

`pluss::make_note` is the one flat (non-namespaced) spelling kept as a
permanent shortcut for the common construct-from-data case, mirroring
`pluss::make_series` and `pluss::make_timeline`. It takes the same argument
forms as `pluss::note::make` and produces the same result. Every other flat
spelling has been retired in favor of `pluss::note::*`.

### Rank ceiling

`Note` is at most 3-dimensional. A four-level nested list, or a `sizes` range
longer than three, is rejected.

The same ceiling and cardinality check applies to native lanes. For example,
four values cannot be paired with shape `[3, 2]`; construction rejects before
publishing a partially formed Note.

## Native Example: Scalars Keep Their Meaning

```cpp
const auto ready = pluss::note::make(true);
const auto count = pluss::note::make(std::int32_t{42});
const auto score = pluss::note::make(0.875F);

std::cout << std::boolalpha << ready.bool_item() << ' '
          << count.int_item() << ' ' << score.float_item() << '\n';
```

### Output

```text
true 42 0.875
```

## Native Example: Flat Values Plus Shape

```cpp
const auto grid = pluss::note::make(
    std::vector<double>{0.5, 1.5, 2.5, 3.5},
    std::array<std::int64_t, 2>{2, 2});

std::cout << grid << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, dtype=Float64)
[
  [0.5, 1.5],
  [2.5, 3.5]
]
```

## Example 1: A Scalar From A String Literal

```cpp
std::cout << pluss::note::print(pluss::note::make("Final Exam Review")) << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"Final Exam Review"
```

The shape is `[]` (rank `0`), not `[1]`. A scalar `Note` holds exactly one
cell and broadcasts against any shape in later operations.

## Example 2: A One-Dimensional Batch

```cpp
const pluss::Note names = pluss::note::make({"Ada Lovelace", "Grace Hopper", "Linus"});
std::cout << names << "\n";  // operator<< uses print()'s default format
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["Ada Lovelace", "Grace Hopper", "Linus"]
```

A braced list of strings is a rank-`1` column — the shape most text pipelines
start from.

## Example 3: A Two-Dimensional Block

```cpp
const pluss::Note grid = pluss::note::make({{"r0c0", "r0c1"}, {"r1c0", "r1c1"}});
pluss::note::print(std::cout, grid) << "\n";  // print(os, n) returns the stream
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["r0c0", "r0c1"], 
  ["r1c0", "r1c1"]
]
```

Nesting one level deeper adds one axis. Every inner list must be the same
length — see Example 10.

## Example 4: A Three-Dimensional Block

```cpp
std::cout << pluss::note::print(
    pluss::note::make({{{"a", "b"}, {"c", "d"}}, {{"e", "f"}, {"g", "h"}}})) << "\n";
```

### Output

```text
Note(shape=[2, 2, 2], dim=3, numel=8, encoding=utf8)
[
  [
    ["a", "b"], 
    ["c", "d"]
  ], 
  [
    ["e", "f"], 
    ["g", "h"]
  ]
]
```

Three levels is the deepest a `Note` goes.

## Example 5: The `pluss::make_note` Shortcut

```cpp
const pluss::Note stages = pluss::make_note({"draft", "review", "final"});
std::cout << stages << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["draft", "review", "final"]
```

Identical to `pluss::note::make({...})`; the flat spelling just reads more
naturally at some call sites.

## Example 6: A Single `char`

```cpp
std::cout << pluss::note::print(pluss::make_note('Q')) << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"Q"
```

A lone `char` is a one-character scalar cell.

## Example 7: Flat Storage Plus An Explicit Shape

```cpp
std::vector<std::string> flat{"p", "q", "r", "s", "t", "u"};
std::array<std::int64_t, 2> sizes{2, 3};
const pluss::Note n = pluss::note::make(
    pluss::msharp::RangeView::Of<std::string>(flat),
    pluss::msharp::RangeView::Of<std::int64_t>(sizes));
pluss::note::print(std::cout, n) << "\n";
```

### Output

```text
Note(shape=[2, 3], dim=2, numel=6, encoding=utf8)
[
  ["p", "q", "r"], 
  ["s", "t", "u"]
]
```

The flat cells fill the shape in row-major order. This is the form to use
when the text and its shape come from different sources.

## Example 8: Multi-Byte UTF-8 Is Preserved

```cpp
const pluss::Note n = pluss::note::make({"café", "naïve", "日本語", "🎯"});
std::cout << n << "\n";
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["café", "naïve", "日本語", "🎯"]
```

Every cell keeps its exact bytes. Character-aware length, slicing, and
case-mapping are then available as `Note` methods.

## Example 9: The Empty String Is A Valid Scalar

```cpp
std::cout << pluss::note::print(pluss::note::make("")) << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
""
```

`""` is one cell that happens to be empty — distinct from a zero-length
`Note`. [`empty`](empty.md) fills a whole shape with this value.

## Example 10: A Ragged Nested List Is Rejected

```cpp
try {
  pluss::note::make({{"a", "b"}, {"c"}});
} catch (const pluss::Error& e) {
  const std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
pluss::note::make: ragged nested initializer_list is not supported
```

Every sibling list at a given depth must have the same length; `make()` does
not pad short rows.

## Example 11: A Flat/Shape Count Mismatch Is Rejected

```cpp
std::vector<std::string> flat{"a", "b", "c"};
std::array<std::int64_t, 1> sizes{5};
try {
  pluss::note::make(pluss::msharp::RangeView::Of<std::string>(flat),
                    pluss::msharp::RangeView::Of<std::int64_t>(sizes));
} catch (const pluss::Error& e) {
  const std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Note construction: flat text count must equal the product of the requested sizes
```

Validation happens before any `Note` is allocated.

## Example 12: A Four-Dimensional List Is Rejected

```cpp
try {
  pluss::note::make({{{{"a"}}}});
} catch (const pluss::Error& e) {
  const std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
pluss::note::make: supports up to 3D nested initializer_list
```

## See Also

- [`pluss::note::bytes`](bytes.md): the same construction surface for raw
  byte content, with no UTF-8 assumption.
- [`pluss::note::full`](full.md): fill a shape with one repeated string.
- [`pluss::note::rep`](rep.md): the value-first spelling of `full`.
- [`pluss::note::from_lines`](from_lines.md) /
  [`pluss::note::from_words`](from_words.md): build a 1-D `Note` by splitting
  a single string.
- [`pluss::note::print`](print.md): render a `Note` for inspection.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="full_like.html">
    <span class="label">Previous</span>
    <span class="title">full_like</span>
  </a>
  <a class="next-link" href="perm.html">
    <span class="label">Next</span>
    <span class="title">perm</span>
  </a>
</div>
