<!-- pluss-hand-authored -->
# Note::NlpAccessor::mask_mentions

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note Note::NlpAccessor::mask_mentions() const
```

## Mask Standalone Handles Without Damaging Email Addresses

`mask_mentions()` replaces an `@` handle with `[MENTION]` when the `@` is
at the beginning of a cell or follows a non-word byte. The handle body must
contain one or more ASCII letters, digits, or underscores. This permits
`@root`, `[@alice]`, and `tag:@ann`, while protecting `x@y.io` and
`mid@name` because their `@` follows a word byte.

The boundary byte is retained, as is punctuation after the handle. A bare `@`
is unchanged. The method does not resolve whether a handle exists, decode
non-ASCII account names, or infer a person from ordinary prose. It performs a
deterministic syntax replacement.

The operation is Text-only, shape-preserving, non-mutating, and idempotent.
It creates a fresh packed Text result. Cells without `@` take a fast no-match
path. Undefined input remains undefined; a defined empty shape remains empty.

## Example 1: Valid Handles And Protected Email Syntax

```cpp
std::cout << pluss::note::print(
    pluss::note::make({"@root [@alice], tag:@ann; x@y.io mid@name"})
        .nlp().mask_mentions()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["[MENTION] [[MENTION]], tag:[MENTION]; x@y.io mid@name"]
```

## Example 2: Empty, Undefined, And Bytes

```cpp
const auto empty = pluss::note::make(std::vector<std::string>{})
                       .nlp().mask_mentions();
std::cout << "mentions empty dim=" << empty.dim()
          << " numel=" << empty.numel() << '\n';
std::cout << "mentions undefined=" << std::boolalpha
          << pluss::Note::undefined().nlp().mask_mentions().defined() << '\n';
try {
  (void)pluss::note::bytes(
      std::vector<std::string>{"@ann"}, std::array<std::int64_t, 1>{1})
      .nlp().mask_mentions();
} catch (const pluss::Error&) { std::cout << "mentions Bytes: rejected\n"; }
```

### Output

```text
mentions empty dim=1 numel=0
mentions undefined=false
mentions Bytes: rejected
```

## See Also

- [`mask_emails`](mask_emails.md): mask complete mailbox syntax first when both categories are needed.
- [`mask_hashtags`](mask_hashtags.md): mask corresponding `#` tokens.
- [`mask_pii`](mask_pii.md): apply the built-in category precedence automatically.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mask_hashtags.html"><span class="label">Previous</span><span class="title">mask_hashtags</span></a>
  <a class="next-link" href="mask_numbers.html"><span class="label">Next</span><span class="title">mask_numbers</span></a>
</div>
