<!-- pluss-hand-authored -->
# NoteNlpAccessor::remove_hashtags

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note NoteNlpAccessor::remove_hashtags() const
```

## Delete Hashtag Tokens Without Damaging Embedded Hashes

`remove_hashtags()` removes social-style hashtag tokens while preserving
the surrounding sentence and its punctuation. It is the deletion counterpart
to `mask_hashtags()`: choose removal when the tag itself contributes no
useful signal, and masking when the downstream consumer should retain the fact
that a tag occupied that position.

A recognized token has two parts:

| Part | Rule |
|---|---|
| Left boundary | Start of the cell, or one byte that is not an ASCII letter, digit, or underscore |
| Hashtag | `#` followed by one or more ASCII letters, digits, or underscores |

The left-boundary rule is important. `#release_2`, `(#team)`, and `[#ops]`
match, but the hash in `mid#word` does not begin a token because it follows an
ASCII letter. `C#` and a bare `#` also remain because there is no tag body after
the hash. The operation is deliberately ASCII-lexical: it does not infer
language-specific word boundaries or extend a tag through punctuation.

Only the hash and its tag body are deleted. Boundary punctuation remains, so
`(#team)` becomes `()` and `[#ops]` becomes `[]`. If at least one token is
removed, remaining ASCII whitespace is collapsed and stripped. If no token
matches, the cell is returned byte-for-byte, preserving newlines and edge
spaces rather than normalizing unrelated text.

The result is a fresh Text Note with the receiver's rank and shape. Cells are
processed independently; a hashtag-only cell becomes `""`, not an absent
cell. Undefined Notes propagate, empty Notes preserve their shape, and Bytes,
Bool, Int64, and Float64 Notes are rejected before any cell is transformed. A
cheap `#` precheck avoids regular-expression work for cells that cannot match.

## Example 1: Tokens At Whitespace And Punctuation Boundaries

```cpp
std::cout << pluss::note::print(
    pluss::note::make({
        "Ship #release_2 with (#team), [#ops]; keep C# mid#word and #"})
        .nlp().remove_hashtags()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["Ship with (), []; keep C# mid#word and #"]
```

The punctuation is evidence that this operation removes the token rather than
the entire punctuation-delimited region around it.

## Example 2: Embedded And Empty Hash Forms Are Exact No-Ops

```cpp
std::cout << pluss::note::print(
    pluss::note::make({"  C#\nmid#word stays  "})
        .nlp().remove_hashtags()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["  C#\nmid#word stays  "]
```

No recognized tag was removed, so the leading spaces, newline, and trailing
spaces survive exactly.

## Example 3: Removal Is Cell-Wise And Shape-Preserving

```cpp
const auto matrix = pluss::note::make(
    std::vector<std::string>{"a@b.io", "Page 1", "#tag", "<b>bold</b>"},
    std::array<std::int64_t, 2>{2, 2});
std::cout << pluss::note::print(matrix.nlp().remove_hashtags()) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["a@b.io", "Page 1"], 
  ["", "<b>bold</b>"]
]
```

The email, footer-like text, and markup belong to other removal families and
are not changed merely because they share the same Note.

## See Also

- [`mask_hashtags`](mask_hashtags.md): replace recognized tags with `[HASHTAG]`.
- `NoteNlpAccessor::remove_mentions`: delete `@`-prefixed mention tokens.
- [`remove_emails`](remove_emails.md): delete address-shaped email spans.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="remove_footers.html"><span class="label">Previous</span><span class="title">remove_footers</span></a>
  <a class="next-link" href="remove_headers.html"><span class="label">Next</span><span class="title">remove_headers</span></a>
</div>
