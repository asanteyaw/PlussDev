<!-- pluss-hand-authored -->
# Note operators

[Back to Note API Reference](../../note_api_reference)

## Signatures

```cpp
Note Note::operator+(const Note& other) const
Note Note::operator+(std::string_view suffix) const
Note operator+(std::string_view prefix, const Note& note)
Note& Note::operator+=(const Note& other)
Note& Note::operator+=(std::string_view suffix)

Note Note::operator*(std::int64_t times) const
Note operator*(std::int64_t times, const Note& note)
Note& Note::operator*=(std::int64_t times)

Note Note::operator==(const Note& other) const
Note Note::operator!=(const Note& other) const
Note Note::operator<(const Note& other) const
Note Note::operator<=(const Note& other) const
Note Note::operator>(const Note& other) const
Note Note::operator>=(const Note& other) const

Note Note::operator==(std::string_view other) const
Note Note::operator!=(std::string_view other) const
Note Note::operator<(std::string_view other) const
Note Note::operator<=(std::string_view other) const
Note Note::operator>(std::string_view other) const
Note Note::operator>=(std::string_view other) const

Note operator==(std::string_view lhs, const Note& rhs)
Note operator!=(std::string_view lhs, const Note& rhs)
Note operator<(std::string_view lhs, const Note& rhs)
Note operator<=(std::string_view lhs, const Note& rhs)
Note operator>(std::string_view lhs, const Note& rhs)
Note operator>=(std::string_view lhs, const Note& rhs)

Note Note::operator&(const Note& other) const
Note Note::operator|(const Note& other) const
Note Note::operator^(const Note& other) const
Note Note::operator~() const

std::ostream& operator<<(std::ostream& stream, const Note& note)
```

## Value Algebra Without Leaving Note

The operator surface lets ordinary C++ expressions build text and Boolean
pipelines while preserving Note shape, dtype, and ownership. The symbols do
not all apply to every dtype:

| Expression family | Accepted dtype | Meaning | Result |
|---|---|---|---|
| `left + right` | matching Text or matching Bytes | concatenate paired cells | Text or Bytes Note |
| `text * count` | Text or Bytes | repeat each cell `count` times | same dtype and shape |
| comparisons | two Notes of the same dtype | compare paired lane values | Bool Note |
| comparison with a string | Text or Bytes Note | compare each cell with one text/byte value | Bool Note |
| `&`, `|`, `^`, `~` | Bool | elementwise Boolean composition | Bool Note |
| `stream << note` | every dtype, including undefined | write the public representation | the same stream |

`+` is concatenation, not numeric addition. Int64 and Float64 Notes can be
compared but cannot be added through this surface. `*` repeats text; it is not
numeric multiplication. Boolean composition uses the eager bitwise symbols
because C++ logical `&&` and `||` cannot return a lane-wise Note result.

## Pairing And One-Element Broadcast

Binary Note operators accept equal shapes, or they broadcast an operand whose
`numel()` is one across the other operand. A rank-zero Note is the clearest
scalar spelling, but a one-element vector also participates in this rule. The
result takes the non-singleton operand's shape. If both operands contain one
element, the left shape is retained.

There is no general singleton-axis broadcasting. Shapes `[2, 1]` and `[1, 2]`
both contain two cells, but they are different shapes and neither operand has
one cell, so pairing rejects them. This prevents accidental row-major pairing
between structurally different Notes.

All operator results are newly allocated Notes. Text concatenation and
repetition build a new packed arena because cell bytes change. Comparisons and
Boolean operations allocate contiguous one-byte Bool storage. Compound
assignment first computes the checked out-of-place result and then replaces
the left handle; another Note sharing the old storage is not rewritten.

Undefined operands reject rather than propagating. Empty Notes are valid when
their shapes satisfy the pairing rule and produce an empty result with that
shape.

## Concatenation

### Example 1: Pair Two Notes Cell By Cell

```cpp
const auto names = pluss::note::make({"Ada", "Grace", "Linus"});
const auto punctuation = pluss::note::make({"!", "?", "."});

std::cout << pluss::note::print(names + punctuation) << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["Ada!", "Grace?", "Linus."]
```

Cells pair by position. Concatenation never joins neighboring cells or changes
the number of cells.

### Example 2: Put A Literal On Either Side

```cpp
const auto names = pluss::note::make({"Ada", "Grace", "Linus"});

std::cout << "suffix\n" << pluss::note::print(names + "!") << '\n';
std::cout << "prefix\n" << pluss::note::print("Dr. " + names) << '\n';
```

### Output

```text
suffix
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["Ada!", "Grace!", "Linus!"]
prefix
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["Dr. Ada", "Dr. Grace", "Dr. Linus"]
```

Operand order determines byte order. A right literal is a suffix; a left
literal is a prefix.

### Example 3: Broadcast A One-Element Note And Preserve Rank

```cpp
const auto matrix = pluss::note::make(
    std::vector<std::string>{"a", "b", "c", "d"},
    std::array<std::int64_t, 2>{2, 2});

std::cout << pluss::note::print(matrix + pluss::note::make("!")) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["a!", "b!"], 
  ["c!", "d!"]
]
```

The scalar contributes bytes to each cell; the matrix supplies the result
geometry.

### Example 4: Compound Assignment Rebinds Only The Left Handle

```cpp
const auto original = pluss::note::make({"Ada", "Grace", "Linus"});
auto changed = original;
changed += pluss::note::make({"!", "?", "."});
changed += "#";

std::cout << "changed\n" << pluss::note::print(changed) << '\n';
std::cout << "original\n" << pluss::note::print(original) << '\n';
```

### Output

```text
changed
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["Ada!#", "Grace?#", "Linus.#"]
original
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["Ada", "Grace", "Linus"]
```

Copying a Note copies an intrusive handle. `+=` does not mutate the shared
arena; it assigns a fresh result to `changed`. If validation or allocation
fails, that assignment does not occur.

### Example 5: Bytes Stay Bytes, Including Embedded Zero Bytes

```cpp
const auto raw = pluss::note::bytes(
    std::vector<std::string>{std::string("A\0", 2), "B"},
    std::array<std::int64_t, 1>{2});
const auto joined = raw + pluss::note::bytes("!");

std::cout << "bytes=" << std::boolalpha << joined.is_bytes()
          << " sizes=" << joined.at(0).size() << ','
          << joined.at(1).size() << '\n';
```

### Output

```text
bytes=true sizes=3,2
```

Concatenation is length-aware; it does not stop at an embedded zero byte.
Text and Bytes cannot be mixed because that would silently assign an encoding
interpretation to raw bytes.

### Example 6: Empty Shapes Remain Empty

```cpp
const auto empty = pluss::note::make(
    std::vector<std::string>{}, std::array<std::int64_t, 2>{2, 0});
std::cout << pluss::note::print(empty + empty) << '\n';
```

### Output

```text
Note(shape=[2, 0], dim=2, numel=0, encoding=utf8)
[]
```

### Example 7: Invalid Concatenation Rejects Before A Result Is Returned

```cpp
const auto matrix = pluss::note::make(
    std::vector<std::string>{"a", "b", "c", "d"},
    std::array<std::int64_t, 2>{2, 2});

try { (void)(matrix + pluss::note::make({"1", "2", "3", "4"})); }
catch (const pluss::Error&) { std::cout << "shape mismatch: rejected\n"; }

try { (void)(pluss::note::make("text") + pluss::note::bytes("raw")); }
catch (const pluss::Error&) { std::cout << "mixed encoding: rejected\n"; }

try {
  (void)(pluss::note::make(std::vector<std::int64_t>{1, 2}) +
         pluss::note::make(std::vector<std::int64_t>{3, 4}));
} catch (const pluss::Error&) { std::cout << "native dtype: rejected\n"; }

try { (void)(pluss::note::make("x") + pluss::Note::undefined()); }
catch (const pluss::Error&) { std::cout << "undefined: rejected\n"; }
```

### Output

```text
shape mismatch: rejected
mixed encoding: rejected
native dtype: rejected
undefined: rejected
```

## Repetition

Repetition duplicates the bytes *inside each cell*. It does not repeat cells
or axes. Therefore shape and `numel()` never change. Use the structural
[`repeat`](repeat.md) or [`repeat_interleave`](repeat_interleave.md) when the
structure rather than cell content must grow.

The count must be nonnegative. Zero produces an empty string in every existing
cell. Before reserving each destination string, the implementation checks that
`cell_size * times` fits the implementation's maximum string size, preventing
size arithmetic from wrapping.

### Example 8: Multiply From Either Side

```cpp
std::cout << "right\n"
          << pluss::note::print(pluss::note::make({"ha", "go"}) * 3) << '\n';
std::cout << "left\n"
          << pluss::note::print(2 * pluss::note::make({"ab", "x"})) << '\n';
```

### Output

```text
right
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["hahaha", "gogogo"]
left
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["abab", "xx"]
```

### Example 9: Zero And Compound Repetition

```cpp
const auto matrix = pluss::note::make(
    std::vector<std::string>{"a", "b", "c", "d"},
    std::array<std::int64_t, 2>{2, 2});
std::cout << "zero\n" << pluss::note::print(matrix * 0) << '\n';

auto chant = pluss::note::make({"na", "batman"});
chant *= 2;
std::cout << "compound\n" << pluss::note::print(chant) << '\n';
```

### Output

```text
zero
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["", ""], 
  ["", ""]
]
compound
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["nana", "batmanbatman"]
```

### Example 10: Repetition Failures

```cpp
try { (void)(pluss::note::make("ha") * -1); }
catch (const pluss::Error&) { std::cout << "negative: rejected\n"; }

try { (void)(pluss::note::make(std::vector<bool>{true, false}) * 2); }
catch (const pluss::Error&) { std::cout << "Bool: rejected\n"; }
```

### Output

```text
negative: rejected
Bool: rejected
```

## Comparisons

All six relations are elementwise and return a shape-preserving Bool Note.
Note-to-Note comparison requires identical dtypes. Text and Bytes use exact
unsigned bytewise lexicographic order: there is no case folding, locale-aware
collation, normalization, or numeric parsing. Int64 and Float64 use native
numeric order; Bool uses `false < true`.

Float64 follows ordinary floating-point comparison. A NaN is unequal to every
value, including itself; `<`, `<=`, `>`, and `>=` are also false when either
operand lane is NaN.

String operands are available only for Text and Bytes Notes. Numeric scalar
comparison remains explicit: construct a rank-zero Note with the matching
dtype. This avoids parsing strings or silently promoting native dtypes.

### Example 11: Exercise Every Relation

```cpp
const auto left = pluss::note::make({"a", "b", "c"});
const auto right = pluss::note::make({"a", "x", "b"});

std::cout << "==\n" << pluss::note::print(left == right) << '\n';
std::cout << "!=\n" << pluss::note::print(left != right) << '\n';
std::cout << "<\n"  << pluss::note::print(left < right) << '\n';
std::cout << "<=\n" << pluss::note::print(left <= right) << '\n';
std::cout << ">\n"  << pluss::note::print(left > right) << '\n';
std::cout << ">=\n" << pluss::note::print(left >= right) << '\n';
```

### Output

```text
==
Note(shape=[3], dim=1, numel=3, dtype=Bool)
[true, false, false]
!=
Note(shape=[3], dim=1, numel=3, dtype=Bool)
[false, true, true]
<
Note(shape=[3], dim=1, numel=3, dtype=Bool)
[false, true, false]
<=
Note(shape=[3], dim=1, numel=3, dtype=Bool)
[true, true, false]
>
Note(shape=[3], dim=1, numel=3, dtype=Bool)
[false, false, true]
>=
Note(shape=[3], dim=1, numel=3, dtype=Bool)
[true, false, true]
```

### Example 12: Literal Orientation Changes The Question

```cpp
const auto values = pluss::note::make({"a", "b", "c"});
std::cout << "values >= b\n" << pluss::note::print(values >= "b") << '\n';
std::cout << "b < values\n" << pluss::note::print("b" < values) << '\n';
```

### Output

```text
values >= b
Note(shape=[3], dim=1, numel=3, dtype=Bool)
[false, true, true]
b < values
Note(shape=[3], dim=1, numel=3, dtype=Bool)
[false, false, true]
```

The first expression includes equality; the second asks strictly whether the
literal precedes each cell.

### Example 13: Broadcast A Typed Scalar Note

```cpp
const auto values = pluss::note::make(std::vector<std::int64_t>{2, 10, -1});
const auto limit = pluss::note::make(std::int64_t{2});
std::cout << pluss::note::print(values >= limit) << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, dtype=Bool)
[true, true, false]
```

The result keeps the vector shape while changing dtype from Int64 to Bool.

### Example 14: Higher Rank Is Preserved

```cpp
const auto matrix = pluss::note::make(
    std::vector<std::string>{"a", "b", "c", "d"},
    std::array<std::int64_t, 2>{2, 2});
std::cout << pluss::note::print(matrix > "b") << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, dtype=Bool)
[
  [false, false], 
  [true, true]
]
```

### Example 15: Native Dtypes And NaN

```cpp
const auto reals = pluss::note::make(std::vector<double>{
    1.0, std::numeric_limits<double>::quiet_NaN(), 3.0});
const auto flags = pluss::note::make(std::vector<bool>{false, true, true});

std::cout << "NaN equality\n" << pluss::note::print(reals == reals) << '\n';
std::cout << "Bool order\n"
          << pluss::note::print(flags > pluss::note::make(false)) << '\n';
```

### Output

```text
NaN equality
Note(shape=[3], dim=1, numel=3, dtype=Bool)
[true, false, true]
Bool order
Note(shape=[3], dim=1, numel=3, dtype=Bool)
[false, true, true]
```

### Example 16: Empty Comparison Preserves Empty Geometry

```cpp
const auto empty = pluss::note::make(
    std::vector<std::string>{}, std::array<std::int64_t, 2>{2, 0});
std::cout << pluss::note::print(empty == empty) << '\n';
```

### Output

```text
Note(shape=[2, 0], dim=2, numel=0, dtype=Bool)
[]
```

### Example 17: Comparison Failures

```cpp
const auto ints = pluss::note::make(std::vector<std::int64_t>{1, 2});
const auto reals = pluss::note::make(std::vector<double>{1.0, 2.0});

try { (void)(ints == reals); }
catch (const pluss::Error&) { std::cout << "mixed dtype: rejected\n"; }
try { (void)(ints == "2"); }
catch (const pluss::Error&) { std::cout << "string against Int64: rejected\n"; }
try { (void)(ints == pluss::Note::undefined()); }
catch (const pluss::Error&) { std::cout << "undefined: rejected\n"; }
```

### Output

```text
mixed dtype: rejected
string against Int64: rejected
undefined: rejected
```

## Boolean Composition

`&`, `|`, and `^` pair Bool lanes under the shared shape rule. `~` negates
every lane and preserves shape. Both operands of a binary Boolean expression
must already be Bool Notes; no truthiness conversion is performed.

The operators are eager: both operands are evaluated before lanes are
combined. They do not have the short-circuit behavior of scalar `&&` and `||`.
Parenthesize comparisons and predicates in mixed expressions so the intended
grouping remains obvious to readers.

### Example 18: Combine Two Predicate Results

```cpp
const auto names = pluss::note::make({"Ada", "Grace", "Linus"});
const auto has_a = names.contains("a");
const auto long_name = names.lengths_chars() >=
                       pluss::note::make(std::int64_t{5});

std::cout << "and\n" << pluss::note::print(has_a & long_name) << '\n';
std::cout << "or\n"  << pluss::note::print(has_a | long_name) << '\n';
std::cout << "xor\n" << pluss::note::print(has_a ^ long_name) << '\n';
std::cout << "not\n" << pluss::note::print(~has_a) << '\n';
```

### Output

```text
and
Note(shape=[3], dim=1, numel=3, dtype=Bool)
[false, true, false]
or
Note(shape=[3], dim=1, numel=3, dtype=Bool)
[true, true, true]
xor
Note(shape=[3], dim=1, numel=3, dtype=Bool)
[true, false, true]
not
Note(shape=[3], dim=1, numel=3, dtype=Bool)
[false, false, true]
```

### Example 19: Broadcast A Boolean Scalar

```cpp
const auto mask = pluss::note::make({true, true, false});
std::cout << pluss::note::print(mask & pluss::note::make(true)) << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, dtype=Bool)
[true, true, false]
```

### Example 20: Logical Failures

```cpp
const auto mask = pluss::note::make({true, false, true});
try { (void)(mask & pluss::note::make({"x", "y", "z"})); }
catch (const pluss::Error&) { std::cout << "non-Bool: rejected\n"; }

try {
  const auto row = pluss::note::make(
      std::vector<bool>{true, false, true},
      std::array<std::int64_t, 2>{1, 3});
  (void)(mask & row);
} catch (const pluss::Error&) { std::cout << "shape mismatch: rejected\n"; }
```

### Output

```text
non-Bool: rejected
shape mismatch: rejected
```

## Stream Insertion

`operator<<` writes the same default public representation as
`pluss::note::print(stream, note)`. It returns the stream, enabling normal
chaining. Rendering does not expose mutable storage or change the Note.

### Example 21: Write A Note Directly To A Stream

```cpp
const auto names = pluss::note::make({"Ada", "Grace", "Linus"});
std::cout << names << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["Ada", "Grace", "Linus"]
```

## See Also

- [`all`](all.md): reduce a Bool Note by conjunction.
- [`any`](any.md): reduce a Bool Note by disjunction.
- [`contains`](contains.md): produce a Bool Note from substring membership.
- `Note::repeat`: repeat Note structure rather than bytes inside cells.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="numel.html"><span class="label">Previous</span><span class="title">numel</span></a>
  <a class="next-link" href="owned.html"><span class="label">Next</span><span class="title">owned</span></a>
</div>
