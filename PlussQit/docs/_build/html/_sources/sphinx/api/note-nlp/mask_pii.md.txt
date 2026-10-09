<!-- pluss-hand-authored -->
# Note::NlpAccessor::mask_pii

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note Note::NlpAccessor::mask_pii(std::int64_t flags = 0) const
```

## Replace Identifying Spans With Category Placeholders

`mask_pii()` scans each text cell for common identifying patterns and
replaces every hit with a bracketed placeholder that names the category. Use
it to sanitize free text before it lands in logs, analytics, prompts, or a
shared dataset, while keeping the surrounding prose readable and the record
count stable.

The eight built-in categories, with their placeholders:

| Bit (value) | Category | Placeholder | Examples it catches |
| --- | --- | --- | --- |
| `1 << 0` (1) | [numbers](mask_numbers.md) | `[NUM]` | `42`, `555-0100`, `3.14` |
| `1 << 1` (2) | [emails](mask_emails.md) | `[EMAIL]` | `bob@example.com` |
| `1 << 2` (4) | [URLs](mask_urls.md) | `[URL]` | `https://x.io/a`, `www.x.io` |
| `1 << 3` (8) | [@-mentions](mask_mentions.md) | `[MENTION]` | `@alice` |
| `1 << 4` (16) | [#-hashtags](mask_hashtags.md) | `[HASHTAG]` | `#ship` |
| `1 << 5` (32) | [dates](mask_dates.md) | `[DATE]` | `2026-01-03`, `Jan 2, 2026` |
| `1 << 6` (64) | [times](mask_times.md) | `[TIME]` | `09:30`, `7pm` |
| `1 << 7` (128) | [currency](mask_currency.md) | `[CUR]` | `$12.50`, `€20`, `99 GBP` |

The categories are detected by pattern, not by a model, so the guarantee is
"masks these shapes," not "removes all PII." Names, street addresses, and
free-form identifiers are **not** masked. Treat it as a strong first pass,
not a compliance control.

### `flags`

`flags` is a bitmask that selects which categories to mask. `flags <= 0`
(the default) means **all** categories. A positive `flags` masks only the
selected bits; `flags` with no recognized bit set leaves the cell unchanged.
So `mask_pii(1)` masks numbers only, and `mask_pii(2 | 4)` masks
emails and URLs and nothing else.

When several categories are active they are applied in a fixed order (emails,
URLs, mentions, hashtags, currency, dates, times, numbers), so a `$12.50`
becomes `[CUR]` before the bare-number pass can touch its digits.

That order is part of the contract. It protects structured spans from broader
recognizers: an email is removed before its `@` can look like a mention, and a
currency/date/time span is removed before its digits can become `[NUM]`.

The operation accepts only Text Notes. It is shape-preserving,
cell-independent, non-mutating, and returns a fresh packed Text `Note`. A cell
with no detected pattern is copied through unchanged. Undefined input remains
undefined; a defined empty shape remains defined and empty. Bytes, Bool,
Int64, and Float64 reject before scanning. Every placeholder is stable under
another masking pass, making the complete transformation idempotent.

## Example 1: Mask Everything

```cpp
const auto n = pluss::make_note(
    {"email bob@x.io url https://a.b @alice #ship on 2026-01-03 at 09:30 for $12.50 id 42",
     "call jane 555-0100 tomorrow"});
std::cout << pluss::note::print(n.nlp().mask_pii()) << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["email [EMAIL] url [URL] [MENTION] [HASHTAG] on [DATE] at [TIME] for [CUR] id [NUM]", "call jane [NUM] tomorrow"]
```

Every recognized span is replaced; the connecting words stay. `555-0100` is
masked as `[NUM]` — there is no separate phone category.

## Example 2: Numbers Only

```cpp
const auto n = pluss::make_note(
    {"email bob@x.io url https://a.b @alice #ship on 2026-01-03 at 09:30 for $12.50 id 42",
     "call jane 555-0100 tomorrow"});
const pluss::Note out = n.nlp().mask_pii(1);
std::cout << out << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["email bob@x.io url https://a.b @alice #ship on [NUM] at [NUM] for $[NUM] id [NUM]", "call jane [NUM] tomorrow"]
```

`flags = 1` leaves the email, URL, mention, and hashtag intact and masks
every digit run.

## Example 3: A Subset — Emails And URLs

```cpp
const auto n = pluss::make_note(
    {"email bob@x.io url https://a.b @alice #ship on 2026-01-03 at 09:30 for $12.50 id 42",
     "call jane 555-0100 tomorrow"});
std::cout << pluss::note::print(n.nlp().mask_pii(2 | 4)) << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["email [EMAIL] url [URL] @alice #ship on 2026-01-03 at 09:30 for $12.50 id 42", "call jane 555-0100 tomorrow"]
```

## Example 4: Nothing To Mask

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"plain prose with no identifiers"}).nlp().mask_pii())
    << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["plain prose with no identifiers"]
```

## Example 5: Shape Is Preserved

```cpp
std::cout << pluss::note::print(
    pluss::make_note({{"ping a@b.io", "code 7"}}).nlp().mask_pii())
    << "\n";
```

### Output

```text
Note(shape=[1, 2], dim=2, numel=2, encoding=utf8)
[
  ["ping [EMAIL]", "code [NUM]"]
]
```

## Example 6: Category Precedence Preserves Meaning

```cpp
const auto text = pluss::note::make({
    "mail a@b.io via https://x.io to @ann #team on 2024-01-02 at 7pm for €20 id 42"});
std::cout << pluss::note::print(text.nlp().mask_pii()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["mail [EMAIL] via [URL] to [MENTION] [HASHTAG] on [DATE] at [TIME] for [CUR] id [NUM]"]
```

Every structured category survives as its own marker. If number masking ran
first, the date, time, and currency identity would be lost.

## Example 7: Unknown Positive Bits Select Nothing

```cpp
const auto text = pluss::note::make({
    "mail a@b.io via https://x.io to @ann #team on 2024-01-02 at 7pm for €20 id 42"});
std::cout << pluss::note::print(text.nlp().mask_pii(1 << 20)) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["mail a@b.io via https://x.io to @ann #team on 2024-01-02 at 7pm for €20 id 42"]
```

A positive mask is intersected with the eight recognized bits. In contrast,
zero or any negative value requests all categories.

## Example 8: Empty, Undefined, And Bytes

```cpp
const auto empty = pluss::note::make(std::vector<std::string>{})
                       .nlp().mask_pii();
std::cout << "pii empty dim=" << empty.dim()
          << " numel=" << empty.numel() << '\n';
std::cout << "pii undefined=" << std::boolalpha
          << pluss::Note::undefined().nlp().mask_pii().defined() << '\n';
try {
  (void)pluss::note::bytes(
      std::vector<std::string>{"a@b.io"}, std::array<std::int64_t, 1>{1})
      .nlp().mask_pii();
} catch (const pluss::Error&) { std::cout << "pii Bytes: rejected\n"; }
```

### Output

```text
pii empty dim=1 numel=0
pii undefined=false
pii Bytes: rejected
```

## See Also

- [`Note::NlpAccessor::remove_urls`](remove_urls.md): delete URLs outright
  instead of replacing them with `[URL]`.
- [`Note::replace`](../note/replace.md) / [`Note::remove_all`](../note/remove_all.md):
  literal, non-pattern rewriting when you know the exact string.
- [`Note::RegexAccessor::findall`](../note-regex/findall.md): extract the spans (for review
  or an allow-list) rather than mask them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mask_numbers.html"><span class="label">Previous</span><span class="title">mask_numbers</span></a>
  <a class="next-link" href="mask_times.html"><span class="label">Next</span><span class="title">mask_times</span></a>
</div>
