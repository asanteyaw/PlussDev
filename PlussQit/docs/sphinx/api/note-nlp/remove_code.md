<!-- pluss-hand-authored -->
# NoteNlpAccessor::remove_code

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note NoteNlpAccessor::remove_code() const
```

## Delete Code-Shaped Spans Without Parsing A Language

`remove_code()` removes common prose-embedded code forms: inline backtick
spans, fenced blocks, and indented code lines. It is appropriate when natural
language surrounding code is the desired result. It is not a programming-
language parser and does not inspect syntax inside the removed region.

| Code form | Recognition rule | Removed extent |
|---|---|---|
| Inline | A pair of single backticks on one line | Both backticks and all enclosed bytes. |
| Backtick fence | A closed pair of triple-backtick fences | Opening fence, optional language label, body, and closing fence. |
| Tilde fence | A closed pair of triple-tilde fences | Opening fence, body, and closing fence. |
| Indented line | At least four leading ASCII spaces, or a leading tab | The complete line. |

Fenced matching may cross line boundaries and selects the shortest closed
span. Inline matching never crosses a newline. An unclosed backtick or fence is
not removed: without a closing delimiter, the method preserves the original
cell exactly. This conservative rule avoids discarding a document suffix after
truncated markup. The method does not recognize language-specific comments,
HTML elements, dollar-delimited mathematics, or bare expressions without one
of the structural forms above.

After a real removal, surviving ASCII whitespace is collapsed and stripped,
so blank lines left by a deleted block do not remain. With no removal, every
byte is retained. Multiple code forms in one cell are handled in one call.

The output is a fresh, shape-preserving Text Note; code removal never changes
rank or cell count. A code-only cell becomes an empty text cell. Empty Text
Notes remain empty, undefined propagates, and Bytes, Bool, Int64, and Float64
Notes reject. Cheap byte and indentation prechecks bypass regular-expression
work for cells that cannot contain a supported code form.

## Example 1: Inline, Fenced, And Indented Code Together

```cpp
const auto source = pluss::note::make({
    "Keep `inline()` here.\n```cpp\nint x;\n```\n"
    "    indented();\n\talso_indented();\nplain",
});
std::cout << pluss::note::print(source.nlp().remove_code()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["Keep here. plain"]
```

## Example 2: An Indented Block May Begin On The First Line

```cpp
std::cout << pluss::note::print(
    pluss::note::make({"    hidden();\nvisible"})
        .nlp().remove_code()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["visible"]
```

## Example 3: Tilde Fences Are Also Code Fences

```cpp
std::cout << pluss::note::print(
    pluss::note::make({"~~~\nhidden();\n~~~\nvisible"})
        .nlp().remove_code()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["visible"]
```

## Example 4: Unmatched Syntax Is A True No-Op

```cpp
std::cout << pluss::note::print(
    pluss::note::make({"  an unmatched `\nremains  "})
        .nlp().remove_code()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["  an unmatched `\nremains  "]
```

## Example 5: Process Cells Independently Without Flattening

```cpp
const auto matrix = pluss::note::make(
    std::vector<std::string>{"Copyright 2026", "a (draft)",
                             "- item", "use `x` now"},
    std::array<std::int64_t, 2>{2, 2});
std::cout << pluss::note::print(matrix.nlp().remove_code()) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["Copyright 2026", "a (draft)"], 
  ["- item", "use now"]
]
```

## Example 6: Empty, Undefined, And Invalid Dtypes

```cpp
std::cout << pluss::note::print(
    pluss::note::make(std::vector<std::string>{}).nlp().remove_code())
          << '\n';
std::cout << "undefined defined=" << std::boolalpha
          << pluss::Note::undefined().nlp().remove_code().defined() << '\n';
try {
  (void)pluss::note::bytes(
      std::vector<std::string>{"`x`"},
      std::array<std::int64_t, 1>{1}).nlp().remove_code();
} catch (const pluss::Error&) {
  std::cout << "Bytes: rejected\n";
}
```

### Output

```text
Note(shape=[0], dim=1, numel=0, encoding=utf8)
[]
undefined defined=false
Bytes: rejected
```

Bool, Int64, and Float64 Notes reject for the same reason.

## See Also

- [`remove_brackets`](remove_brackets.md): remove balanced bracketed spans.
- `NoteNlpAccessor::remove_markdown`: remove links, emphasis, headings, and Markdown code together.
- `NoteNlpAccessor::remove_math`: remove common math-delimiter forms rather than code forms.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="remove_bullets.html"><span class="label">Previous</span><span class="title">remove_bullets</span></a>
  <a class="next-link" href="remove_emails.html"><span class="label">Next</span><span class="title">remove_emails</span></a>
</div>
