<!-- pluss-hand-authored -->
# NoteNlpAccessor::remove_bullets

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note NoteNlpAccessor::remove_bullets() const
```

## Keep List Items While Removing Their Markers

`remove_bullets()` removes a recognized list marker from the beginning of
each line while retaining the item text. It is useful when list membership is
presentation detail and the words should become ordinary prose for matching,
search, or tokenization.

Leading spaces and tabs may precede a marker. The accepted markers are:

| Family | Accepted form | Boundary requirement |
|---|---|---|
| Unordered ASCII | `-`, `*`, `+` | Followed by a space, tab, or end of line. |
| Unordered Unicode | `•`, `‣`, `⁃` | Followed by a space, tab, or end of line. |
| Ordered | One or more ASCII digits followed by `.` or `)` | Marker punctuation is followed by a space, tab, or end of line. |

The boundary requirement protects ordinary text. `-word` is not a list item,
`3.14 value` is not mistaken for item 3, and a line beginning with an unrelated
non-ASCII character remains valid UTF-8 text. Marker recognition is performed
only at line starts after indentation; a dash in the middle of a sentence is
never considered.

If one or more markers are removed, item lines and ordinary lines in that cell
are joined by normalized ASCII whitespace. Blank lines disappear as part of
that compaction. If the cell contains no recognized marker, it is returned
byte-for-byte, including newlines and indentation. Thus the method is a list
projection, not an unconditional line joiner.

The result is a newly allocated, shape-identical Text Note. A line containing
only a marker contributes no text, and a cell containing only empty marked
items becomes an empty string without changing the Note's shape. Undefined
propagates; non-Text dtypes reject.

## Example 1: Every Marker Family And Its Boundaries

```cpp
const auto items = pluss::note::make({
    "- alpha\n* beta\n+ gamma\n1. first\n2) second\n"
    "• round\n‣ triangle\n⁃ hyphen\n-word\n3.14 value\nÉclair",
});
std::cout << pluss::note::print(items.nlp().remove_bullets()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["alpha beta gamma first second round triangle hyphen -word 3.14 value Éclair"]
```

## Example 2: Unicode Marker Recognition Does Not Need An ASCII Marker

```cpp
std::cout << pluss::note::print(
    pluss::note::make({"• standalone"}).nlp().remove_bullets()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["standalone"]
```

## Example 3: Ordinary Multiline Text Is Preserved Exactly

```cpp
std::cout << pluss::note::print(
    pluss::note::make({"First line.\n-word stays"})
        .nlp().remove_bullets()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["First line.\n-word stays"]
```

## Example 4: Shape Is Independent Of Line Count

```cpp
const auto matrix = pluss::note::make(
    std::vector<std::string>{"Copyright 2026", "a (draft)",
                             "- item", "use `x` now"},
    std::array<std::int64_t, 2>{2, 2});
std::cout << pluss::note::print(
    matrix.nlp().remove_bullets()) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["Copyright 2026", "a (draft)"], 
  ["item", "use `x` now"]
]
```

## See Also

- [`remove_boilerplate`](remove_boilerplate.md): delete entire recognized policy lines.
- `NoteNlpAccessor::remove_headers`: delete entire heading lines rather than retaining their text.
- `NoteNlpAccessor::remove_markdown`: remove a wider Markdown syntax surface.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="remove_brackets.html"><span class="label">Previous</span><span class="title">remove_brackets</span></a>
  <a class="next-link" href="remove_code.html"><span class="label">Next</span><span class="title">remove_code</span></a>
</div>
