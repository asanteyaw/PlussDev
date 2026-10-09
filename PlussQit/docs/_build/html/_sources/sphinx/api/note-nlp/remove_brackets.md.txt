<!-- pluss-hand-authored -->
# Note::NlpAccessor::remove_brackets

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note Note::NlpAccessor::remove_brackets() const
```

## Remove Balanced Bracketed Asides With Their Contents

`remove_brackets()` deletes balanced spans delimited by parentheses `()`,
square brackets `[]`, or braces `{}`. Both delimiters and everything between
them disappear. Use it when bracketed material is ancillary to the surrounding
text, such as editorial notes, compact metadata, or inline alternatives.
It is not the right operation when only the delimiter characters should be
removed while their contents remain.

Each delimiter family supports nesting of its own kind. A balanced outer span
also removes any differently bracketed text inside it because that text is part
of the outer span. The families are processed as parentheses, then square
brackets, then braces; surviving text retains its original order.

Malformed delimiters are handled conservatively:

| Input condition | Result |
|---|---|
| Balanced opening and closing delimiter | Entire span is deleted. |
| Opening delimiter with no matching close | Opening delimiter and following text are retained. |
| Closing delimiter with no preceding open | Closing delimiter is retained. |
| Nested balanced spans of the same family | Removed as one outer span. |

This policy prevents a truncated document from losing its entire suffix merely
because it contains an unmatched opening bracket.

After at least one balanced span is removed, ASCII whitespace in the cell is
collapsed and stripped. Punctuation outside the span is not repaired, so a
space before a surviving semicolon can remain. If no balanced span exists, the
cell is returned byte-for-byte. The operation is shape-preserving,
out-of-place, and limited to Text Notes; empty and undefined inputs follow the
shared transformation contract.

## Example 1: Balanced Spans And Mixed Delimiters

```cpp
std::cout << pluss::note::print(
    pluss::note::make({
        "Keep (draft [old]) final {meta}; literal unmatched (kept"})
        .nlp().remove_brackets()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["Keep final ; literal unmatched (kept"]
```

The square brackets vanish with the balanced outer parentheses. The final
unmatched opening parenthesis and its text survive.

## Example 2: Same-Family Nesting And An Unmatched Close

```cpp
std::cout << pluss::note::print(
    pluss::note::make({"Nested [one [two] three] end; closing ] stays"})
        .nlp().remove_brackets()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["Nested end; closing ] stays"]
```

## Example 3: Malformed-Only Input Is A True No-Op

```cpp
std::cout << pluss::note::print(
    pluss::note::make({"  unmatched ( text  "})
        .nlp().remove_brackets()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["  unmatched ( text  "]
```

## Example 4: Apply Independently Across A Matrix

```cpp
const auto matrix = pluss::note::make(
    std::vector<std::string>{"Copyright 2026", "a (draft)",
                             "- item", "use `x` now"},
    std::array<std::int64_t, 2>{2, 2});
std::cout << pluss::note::print(
    matrix.nlp().remove_brackets()) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["Copyright 2026", "a"], 
  ["- item", "use `x` now"]
]
```

## Failure And Absence

An empty Text Note returns an empty Note with the same extents. Undefined
propagates. Bytes, Bool, Int64, and Float64 Notes throw `pluss::Error`; the
method does not decode arbitrary bytes or format typed values as text first.

## See Also

- `Note::NlpAccessor::remove_parentheticals`: restrict removal to balanced parentheses.
- `Note::remove_punctuation`: retain bracket contents while deleting ASCII punctuation marks.
- [`remove_code`](remove_code.md): delete code delimiters together with code content.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="remove_boilerplate.html"><span class="label">Previous</span><span class="title">remove_boilerplate</span></a>
  <a class="next-link" href="remove_bullets.html"><span class="label">Next</span><span class="title">remove_bullets</span></a>
</div>
