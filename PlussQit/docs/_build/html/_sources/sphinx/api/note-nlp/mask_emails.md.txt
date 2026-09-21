<!-- pluss-hand-authored -->
# NoteNlpAccessor::mask_emails

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note NoteNlpAccessor::mask_emails() const
```

## Hide Mailboxes Without Hiding Nearby Handles

`mask_emails()` replaces each ASCII email-shaped span with `[EMAIL]`.
It is narrower than [`mask_pii`](mask_pii.md): URLs, social handles,
numbers, and dates remain available for separate policy decisions.

The local part accepts ASCII letters, digits, `.`, `_`, `%`, `+`, and `-`.
The domain accepts ASCII letters, digits, dots, and hyphens and must end in a
letter-only suffix of at least two characters. Matching is case-sensitive only
in preservation terms: the entire matched address is removed, so letter case
does not affect the placeholder.

`a+b@example.co.uk` and subdomains are recognized. `a@b`, `@handle`, and
non-ASCII mailbox syntax are not. The boundary before an address prevents a
substring inside a larger email-like token from being masked accidentally.
Sentence punctuation after a valid address remains outside the replacement.

The method is shape-preserving, non-mutating, and Text-only. It allocates one
fresh packed Text result. Cells without `@` bypass recognition immediately.
Undefined input propagates; empty shapes remain defined and empty. Existing
`[EMAIL]` placeholders are idempotent.

## Example 1: Plus Addressing And Subdomains

```cpp
std::cout << pluss::note::print(
    pluss::note::make({"a+b@example.co.uk", "x.y@sub.example.io"})
        .nlp().mask_emails()) << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["[EMAIL]", "[EMAIL]"]
```

## Example 2: Punctuation, Incomplete Addresses, And Mentions

```cpp
std::cout << pluss::note::print(
    pluss::note::make({"mail x@y.io, keep a@b and @handle"})
        .nlp().mask_emails()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["mail [EMAIL], keep a@b and @handle"]
```

The comma survives, while incomplete mail syntax and the social handle remain.

## Example 3: Empty, Undefined, And Bytes

```cpp
const auto empty = pluss::note::make(std::vector<std::string>{})
                       .nlp().mask_emails();
std::cout << "emails empty dim=" << empty.dim()
          << " numel=" << empty.numel() << '\n';
std::cout << "emails undefined=" << std::boolalpha
          << pluss::Note::undefined().nlp().mask_emails().defined() << '\n';
try {
  (void)pluss::note::bytes(
      std::vector<std::string>{"a@b.io"}, std::array<std::int64_t, 1>{1})
      .nlp().mask_emails();
} catch (const pluss::Error&) { std::cout << "emails Bytes: rejected\n"; }
```

### Output

```text
emails empty dim=1 numel=0
emails undefined=false
emails Bytes: rejected
```

## See Also

- [`mask_mentions`](mask_mentions.md): mask standalone `@` handles without treating emails as handles.
- [`mask_pii`](mask_pii.md): combine email masking with other categories.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mask_dates.html"><span class="label">Previous</span><span class="title">mask_dates</span></a>
  <a class="next-link" href="mask_hashtags.html"><span class="label">Next</span><span class="title">mask_hashtags</span></a>
</div>
