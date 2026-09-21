<!-- pluss-hand-authored -->
# NoteNlpAccessor::remove_emails

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note NoteNlpAccessor::remove_emails() const
```

## Delete Address-Shaped Email Tokens

`remove_emails()` deletes email-address spans while retaining the prose
around them. It is appropriate when an address is incidental or sensitive and
its presence does not need a replacement marker. Use `mask_emails()` when
the downstream text should still record that an email occurred.

Recognition is deliberately lexical rather than a complete mail standard:

| Component | Accepted bytes |
|---|---|
| Local part | ASCII letters, digits, `.`, `_`, `%`, `+`, and `-` |
| Separator | One `@` |
| Domain body | ASCII letters, digits, `.`, and `-` |
| Final suffix | A dot followed by at least two ASCII letters |

This covers common tagged local parts and multi-label domains, including
`a+b@example.co.uk`. It does not validate mailbox delivery, prohibit every
structurally dubious dot placement, support quoted local parts, or recognize
an address without a letter-only final suffix. In particular, `a@b`,
`@handle`, and `x@@example.com` are left alone.

When a span is removed, remaining ASCII whitespace is collapsed and stripped.
Punctuation adjacent to the address remains. If no valid span is found, the
cell is returned byte-for-byte, including newlines and edge spaces. A cell
consisting only of an address becomes an empty text cell, not an absent cell.

The method returns a new shape-identical Text Note and never mutates the
receiver. It scans cells independently, uses an `@` precheck to avoid regex
work on impossible cells, propagates undefined, preserves empty shapes, and
rejects Bytes, Bool, Int64, and Float64 Notes.

## Example 1: Tagged And Multi-Label Addresses

```cpp
std::cout << pluss::note::print(
    pluss::note::make({
        "Write a+b@example.co.uk or x.y@sub.example.io today"})
        .nlp().remove_emails()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["Write or today"]
```

## Example 2: Invalid And Non-Email `@` Forms Are Exact No-Ops

```cpp
std::cout << pluss::note::print(
    pluss::note::make({
        "  keep a@b, @handle, and x@@example.com\nexactly  "})
        .nlp().remove_emails()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["  keep a@b, @handle, and x@@example.com\nexactly  "]
```

## Example 3: Complete Removal Preserves Matrix Shape

```cpp
const auto matrix = pluss::note::make(
    std::vector<std::string>{"a@b.io", "Page 1", "#tag", "<b>bold</b>"},
    std::array<std::int64_t, 2>{2, 2});
std::cout << pluss::note::print(matrix.nlp().remove_emails()) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["", "Page 1"], 
  ["#tag", "<b>bold</b>"]
]
```

## See Also

- [`mask_emails`](mask_emails.md): replace each recognized address with `[EMAIL]`.
- [`remove_urls`](remove_urls.md): delete scheme or `www.` web links.
- [`mask_pii`](mask_pii.md): preserve several sensitive categories as named markers.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="remove_code.html"><span class="label">Previous</span><span class="title">remove_code</span></a>
  <a class="next-link" href="remove_footers.html"><span class="label">Next</span><span class="title">remove_footers</span></a>
</div>
