# Note::debug

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
std::string Note::debug(std::int64_t max_items = 12) const
```

## Build A Bounded Diagnostic Description

`debug()` returns one owning string that identifies the Note's state, geometry,
dtype, and a bounded prefix of its values. It is intended for logs, assertions,
debuggers, and failure messages where printing an entire large Note would hide
the useful context. Unlike `operator<<`, it does not emit to a stream, and
unlike [`pack()`](pack.md), its human-readable result is not a persistence
format and must not be parsed back into a Note.

`max_items` is the maximum number of flat, row-major lanes included. The
default is `12`. If fewer lanes are shown than the Note contains, `...` marks
the omission. Zero deliberately shows no lanes, and negative values are
clamped to zero; both therefore produce `values=[...]` for a non-empty Note.
The limit counts lanes rather than bytes, so a displayed text cell may itself
be long.

The prefix follows flat storage order regardless of rank. Shape and dtype are
always reported, so truncation cannot make a matrix look like a vector or an
Int64 Note look textual. Text lanes escape quotes, backslashes, and control
characters while retaining valid non-ASCII text. Bytes lanes additionally
escape non-ASCII octets as `\xHH`, preventing arbitrary binary data from being
written directly into a terminal or log.

An undefined handle has the stable description `Note(undefined)`. This makes
`debug()` safe before `defined()` checks, unlike dtype and element accessors.
The method does not mutate the Note. It allocates the returned string and
examines at most `max_items` lanes, although the cost of rendering those lanes
also depends on their byte lengths.

## Example 1: Default And Explicit Truncation

```cpp
const auto words = pluss::note::make({"alpha", "line\n\"q\"", "omega"});
std::cout << words.debug(2) << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8, values=["alpha", "line\n\"q\"", ...])
```

The newline and quotes belong to the second lane; escaping keeps the diagnostic
on one physical line.

## Example 2: Native Dtypes Keep Their Identity

```cpp
std::cout
    << pluss::note::make(std::vector<std::int64_t>{-2, 0, 7}).debug() << '\n'
    << pluss::note::make(std::vector<bool>{true, false}).debug() << '\n'
    << pluss::note::make(std::vector<double>{-0.0, 1.25}).debug() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, dtype=Int64, values=[-2, 0, 7])
Note(shape=[2], dim=1, numel=2, dtype=Bool, values=[true, false])
Note(shape=[2], dim=1, numel=2, dtype=Float64, values=[-0, 1.25])
```

Values are formatted from typed storage; they are not converted into Text
lanes first. The sign of floating-point zero remains visible.

## Example 3: Suppress All Values Without Losing Metadata

```cpp
const auto value = pluss::note::make({"a", "b", "c"});
std::cout << value.debug(0) << '\n'
          << value.debug(-3) << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8, values=[...])
Note(shape=[3], dim=1, numel=3, encoding=utf8, values=[...])
```

This call form is useful when a log should disclose structure but not contents.

## Example 4: Binary Lanes Are Log-Safe

```cpp
const std::string raw{"A\0\xFF", 3};
std::cout << pluss::note::bytes(std::vector<std::string>{raw}).debug() << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=bytes, values=["A\x00\xFF"])
```

## Example 5: Undefined Is Directly Inspectable

```cpp
std::cout << pluss::Note::undefined().debug() << '\n';
```

### Output

```text
Note(undefined)
```

## See Also

- [`operator<<`](operators.md): stream a Note using its normal public display.
- [`pack`](pack.md): create a stable, machine-readable binary representation.
- [`defined`](defined.md): branch explicitly on handle state.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="count_substring.html"><span class="label">Previous</span><span class="title">count_substring</span></a>
  <a class="next-link" href="dedent.html"><span class="label">Next</span><span class="title">dedent</span></a>
</div>
<!-- pluss-hand-authored -->
