# Note::nlp_mask_pii

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note::nlp_mask_pii() const -> Note
Note::nlp_mask_pii(std::int64_t flags) const -> Note
```

## What It Does

Masks several classes of personally identifiable or sensitive text-like spans
with placeholder markers.

This is one of the most practically useful `Note` preprocessing methods when
you want to keep the sentence structure but neutralize emails, dates, times,
currency amounts, mentions, hashtags, and related surface forms before export,
modeling, or logging.

The important distinction from deletion-based cleanup is that masking preserves
the sentence shell. Downstream code can still see roughly what kind of content
was present and where it appeared.

## Parameters

**flags**

Optional masking-policy flags.

Use the zero-argument form when you want the library’s default masking policy.
Reach for the flagged overload only when you already know the implementation
flag contract you want. This page avoids inventing undocumented flag semantics.

## Returns

`Note`
    A shape-preserving `Note` with supported sensitive spans replaced by
    placeholder markers.

## Examples

Example 1. Mask multiple sensitive span classes while preserving the sentence
shell.

```cpp
auto pii = pluss::make_note({
    "Email bob@example.com at 9:30am on 2025-01-03, pay $12.50 to @alice #ops",
    "Backup contact: jane@pluss.dev",
});

auto out = pii.nlp_mask_pii();
std::cout << "nlp_mask_pii:\n";
for (std::int64_t i = 0; i < out.numel(); ++i) {
  std::cout << "  [" << i << "] " << out.at(i) << "\n";
}
```

### Output

```text
nlp_mask_pii:
  [0] Email [EMAIL] at [TIME] on [DATE], pay [CUR] to [MENTION] [HASHTAG]
  [1] Backup contact: [EMAIL]
```

Example 2. Inspect the masked result when you want to confirm a specific
placeholder class was emitted.

```cpp
auto masked = pluss::make_note({
    "Email bob@example.com at 9:30am on 2025-01-03, pay $12.50 to @alice #ops",
    "Backup contact: jane@pluss.dev",
}).nlp_mask_pii();

auto email_slots = masked.contains("[EMAIL]");
std::cout << "nlp_mask_pii_contains_email:";
for (std::uint8_t v : email_slots) {
  std::cout << " " << static_cast<int>(v);
}
std::cout << "\n";
```

### Output

```text
nlp_mask_pii_contains_email: 1 1
```

## Notes

- `nlp_mask_pii()` keeps the sentence skeleton readable while masking detected
  sensitive fragments.
- This is usually preferable to full deletion when downstream consumers still
  benefit from the surrounding context or syntax.
- If your only goal is link removal, [`nlp_remove_urls`](nlp_remove_urls.md) is
  the smaller and more targeted primitive.

## See Also

- [`Note::nlp_remove_urls`](nlp_remove_urls.md): remove only URL-like spans.
- [`Note::tokenize_words`](tokenize_words.md): tokenize the masked text after
  cleanup.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tokenize_words.html">
    <span class="label">Previous</span>
    <span class="title">tokenize_words</span>
  </a>
  <a class="next-link" href="base64_encode.html">
    <span class="label">Next</span>
    <span class="title">base64_encode</span>
  </a>
</div>
