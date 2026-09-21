<!-- pluss-hand-authored -->
# NoteNlpAccessor::remove_boilerplate

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note NoteNlpAccessor::remove_boilerplate() const
```

## Delete Recognizable Policy And Rights Lines

`remove_boilerplate()` removes complete lines that look like recurring
rights, policy, terms, or disclaimer material. It exists for document bodies
whose useful prose has been mixed with site or publication furniture. The
method is intentionally a small deterministic filter, not a classifier: it
removes only the line forms in the table below.

| Recognition rule | Examples that match |
|---|---|
| Trimmed line starts with the complete word `copyright` | `Copyright 2026 Pluss` |
| Trimmed line starts with `©` | `© 2026 Pluss` |
| Line contains `all rights reserved` | `Pluss, all rights reserved.` |
| Line contains `privacy policy` | `Read our Privacy Policy` |
| Line contains `terms of service` | `Terms of Service apply` |
| Trimmed line starts with the complete word `disclaimer` | `Disclaimer: preliminary` |

Matching is ASCII case-insensitive. The `copyright` and `disclaimer` boundary
rules matter: `copyrighted study` and `disclaimer_value` are not treated as the
standalone trigger words. Likewise, isolated words such as `privacy`, `terms`,
or `rights` do not match. This narrow vocabulary makes the operation
predictable, but it cannot discover paraphrased notices or decide whether a
matching policy sentence is substantively important. Do not use it where such
lines must be retained as evidence.

When at least one line matches, all matching lines are deleted and the
remaining ASCII whitespace is collapsed to single spaces, with edge whitespace
stripped. This turns a surviving multiline document into one compact cell. If
no line matches, the original cell is returned byte-for-byte, including line
breaks and edge spaces. A cell made entirely from recognized lines becomes the
empty string; the cell is not removed from the Note.

The receiver is unchanged. The result is a fresh Text Note with identical
rank, extents, and cell order. Empty Text Notes remain empty, undefined
propagates, and Bytes, Bool, Int64, and Float64 Notes reject. Static trigger
prechecks avoid line processing for cells that contain none of the relevant
vocabulary.

## Example 1: Remove Several Recognized Line Forms

```cpp
const auto source = pluss::note::make({
    "Project note\nCopyright 2026 Pluss\nPrivacy Policy\n"
    "This copyrighted study remains.\nTerms of service apply\n"
    "Disclaimer: draft\nAll rights reserved",
});
std::cout << pluss::note::print(
    source.nlp().remove_boilerplate()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["Project note This copyrighted study remains."]
```

The ordinary sentence survives because `copyrighted` is not the complete
trigger word at the beginning of its trimmed line.

## Example 2: A No-Match Cell Is Exactly Preserved

```cpp
std::cout << pluss::note::print(
    pluss::note::make({
        " Privacy engineering\nand contractual terms remain unchanged. "})
        .nlp().remove_boilerplate()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
[" Privacy engineering\nand contractual terms remain unchanged. "]
```

Neither the newline nor edge spaces are normalized because no line was
removed.

## Example 3: Removing A Whole Cell Does Not Change Shape

```cpp
const auto matrix = pluss::note::make(
    std::vector<std::string>{"Copyright 2026", "a (draft)",
                             "- item", "use `x` now"},
    std::array<std::int64_t, 2>{2, 2});
std::cout << pluss::note::print(
    matrix.nlp().remove_boilerplate()) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["", "a (draft)"], 
  ["- item", "use `x` now"]
]
```

## Example 4: Idempotence

```cpp
const auto once = source.nlp().remove_boilerplate();
std::cout << "boilerplate idempotent=" << std::boolalpha
          << (once.nlp().remove_boilerplate() == once).all().bool_item()
          << '\n';
```

### Output

```text
boilerplate idempotent=true
```

## See Also

- [`remove_bullets`](remove_bullets.md): retain list items while deleting their markers.
- [`remove_code`](remove_code.md): remove code-shaped spans and lines.
- `NoteNlpAccessor::remove_headers`: remove heading lines rather than policy furniture.
- `NoteNlpAccessor::remove_footers`: remove page-number and footer-shaped lines.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="readability_level.html"><span class="label">Previous</span><span class="title">readability_level</span></a>
  <a class="next-link" href="remove_brackets.html"><span class="label">Next</span><span class="title">remove_brackets</span></a>
</div>
