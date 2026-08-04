# Note::nlp_remove_urls

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note::nlp_remove_urls() const -> Note
```

## What It Does

Removes URL-like spans from each text cell.

This is one of the practical first-step cleanup methods when your text comes
from scraped pages, prompts, notes, or discussion logs where links carry noise
you do not want in the downstream token stream.

The method removes the URLs themselves and leaves the surrounding sentence
structure in place as much as possible.

That makes it useful as an early cleanup stage before tokenization, keyword
extraction, or export, especially when links are pure noise for the task at
hand.

## Parameters

This method takes no parameters.

## Returns

`Note`
    A new `Note` with URL-like substrings removed.

## Examples

Example 1. Remove links while keeping the surrounding text.

```cpp
auto dirty = pluss::make_note({
    "Visit https://pluss.dev and email team@pluss.dev",
    "Mirror http://docs.pluss.dev for release notes",
});

auto out = dirty.nlp_remove_urls();
std::cout << "nlp_remove_urls:\n";
for (std::int64_t i = 0; i < out.numel(); ++i) {
  std::cout << "  [" << i << "] " << out.at(i) << "\n";
}
```

### Output

```text
nlp_remove_urls:
  [0] Visit and email team@pluss.dev
  [1] Mirror for release notes
```

Example 2. Inspect the cleaned result when you want to confirm the URL markers
are actually gone before later tokenization.

```cpp
auto cleaned = pluss::make_note({
    "Visit https://pluss.dev and email team@pluss.dev",
    "Mirror http://docs.pluss.dev for release notes",
}).nlp_remove_urls();

auto mask = cleaned.contains("http");
std::cout << "nlp_remove_urls_contains_http:";
for (std::uint8_t v : mask) {
  std::cout << " " << static_cast<int>(v);
}
std::cout << "\n";
```

### Output

```text
nlp_remove_urls_contains_http: 0 0
```

## Notes

- `nlp_remove_urls()` removes URLs, not email addresses. The first example keeps
  `team@pluss.dev` intentionally.
- This is a better first cleanup primitive than broad "do everything" cleanup
  when you want to preserve control over which noise classes are removed.
- In layered text pipelines, explicit small cleanup passes are often easier to
  reason about than one monolithic cleaner.

## See Also

- [`Note::split_whitespace`](split_whitespace.md): tokenize the cleaned text.
- [`Note::contains`](contains.md): check whether URL markers are still present
  after cleanup.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lower.html">
    <span class="label">Previous</span>
    <span class="title">lower</span>
  </a>
  <a class="next-link" href="regex_findall.html">
    <span class="label">Next</span>
    <span class="title">regex_findall</span>
  </a>
</div>
