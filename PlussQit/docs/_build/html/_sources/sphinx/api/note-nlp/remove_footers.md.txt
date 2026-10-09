<!-- pluss-hand-authored -->
# Note::NlpAccessor::remove_footers

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note Note::NlpAccessor::remove_footers() const
```

## Remove Repeated Page And Rights Lines

`remove_footers()` deletes complete lines that match a compact footer
grammar. It is useful after page extraction, where page labels or publication
notices recur inside otherwise useful prose. Unlike
`remove_boilerplate()`, it does not search for policy phrases in arbitrary
positions; its rights and confidentiality forms must begin the trimmed line.

| Footer family | Complete line rule |
|---|---|
| Page number | `page`, one or more ASCII digits, and optionally `of` plus one or more digits |
| Copyright | Trimmed line starts with the complete word `copyright` or with `©` |
| Rights | Trimmed line starts with `all rights reserved` as complete words |
| Confidentiality | Trimmed line starts with the complete word `confidential` |

Matching is ASCII case-insensitive and permits surrounding whitespace. The
page-number rule requires numeric digits: `page three` remains. Word boundaries
protect `copyrighted` and `confidentiality`, so ordinary prose beginning with
those longer words is not discarded. Once a rights-style prefix matches, the
rest of that line may contain years, owners, punctuation, or qualifiers.

If at least one footer line is deleted, surviving lines are joined through
collapsed ASCII whitespace and stripped. If none matches, the original cell is
returned exactly. A footer-only cell becomes `""` while retaining its position
in the Note. Processing is out-of-place, shape-preserving, cell-independent,
and Text-only; undefined, empty, and invalid-dtype behavior matches the other
NLP removers.

## Example 1: All Footer Families

```cpp
std::cout << pluss::note::print(
    pluss::note::make({
        "Body line\nPage 3 of 9\nConfidential - internal\n"
        "© 2026 Pluss\nCopyright 2026 Pluss\n"
        "All rights reserved\nClosing"})
        .nlp().remove_footers()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["Body line Closing"]
```

## Example 2: Similar Vocabulary Is Not Enough

```cpp
std::cout << pluss::note::print(
    pluss::note::make({"  page three\nconfidentiality matters  "})
        .nlp().remove_footers()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["  page three\nconfidentiality matters  "]
```

## Example 3: Footer-Only Cells Become Empty, Not Absent

```cpp
const auto matrix = pluss::note::make(
    std::vector<std::string>{"a@b.io", "Page 1", "#tag", "<b>bold</b>"},
    std::array<std::int64_t, 2>{2, 2});
std::cout << pluss::note::print(matrix.nlp().remove_footers()) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["a@b.io", ""], 
  ["#tag", "<b>bold</b>"]
]
```

## See Also

- [`remove_boilerplate`](remove_boilerplate.md): recognize policy and terms lines in addition to rights notices.
- `Note::NlpAccessor::remove_headers`: remove hash-marked heading lines.
- [`remove_html`](remove_html.md): strip markup while retaining element text.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="remove_emails.html"><span class="label">Previous</span><span class="title">remove_emails</span></a>
  <a class="next-link" href="remove_hashtags.html"><span class="label">Next</span><span class="title">remove_hashtags</span></a>
</div>
