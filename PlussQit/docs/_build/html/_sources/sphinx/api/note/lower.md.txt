# Note::lower

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note::lower() const -> Note
```

## What It Does

Converts every text cell to lowercase.

This is the straightforward case-normalization pass for batch text. It is
useful before substring checks, de-duplication, token comparison, and any
pipeline where `"Exam"` and `"exam"` should be treated the same.

`lower()` does not trim whitespace or remove punctuation. It only changes case.

That narrow contract is useful: if later output still contains padding or
punctuation, you know `lower()` is not the stage that changed it.

## Parameters

This method takes no parameters.

## Returns

`Note`
    A new `Note` with the same shape as the input, with each cell lowercased.

## Examples

Example 1. Lowercase a batch of names while preserving spaces and tabs.

```cpp
auto note = pluss::make_note({"  Ada Lovelace  ", "\tGRACE HOPPER\t", "Linus"});
auto out = note.lower();
std::cout << "lower:\n" << out << "\n";
```

### Output

```text
lower:
  [0]   ada lovelace  
  [1] 	grace hopper	
  [2] linus
```

Example 2. Trim first when the real goal is normalized text rather than
lowercased-but-still-padded text.

```cpp
auto note = pluss::make_note({"  Ada Lovelace  ", "\tGRACE HOPPER\t", "Linus"});
auto cleaned = note.strip().lower();
std::cout << "lower_strip:\n" << pluss::print_note(cleaned) << "\n";
```

### Output

```text
lower_strip:
  [0] ada lovelace
  [1] grace hopper
  [2] linus
```

## Notes

- `lower()` preserves shape.
- It also preserves boundary whitespace exactly as-is.
- In real pipelines, `lower()` is often paired with [`strip`](strip.md) when you
  want both case normalization and boundary cleanup.

## See Also

- [`Note::strip`](strip.md): remove leading and trailing whitespace or specific
  boundary characters.
- [`Note::replace`](replace.md): rewrite substrings after case normalization if
  needed.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lengths_bytes.html">
    <span class="label">Previous</span>
    <span class="title">lengths_bytes</span>
  </a>
  <a class="next-link" href="nlp_remove_urls.html">
    <span class="label">Next</span>
    <span class="title">nlp_remove_urls</span>
  </a>
</div>
