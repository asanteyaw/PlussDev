<!-- pluss-hand-authored -->
# NoteNlpAccessor::mask_hashtags

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note NoteNlpAccessor::mask_hashtags() const
```

## Retain The Tag Role, Remove The Tag Value

`mask_hashtags()` replaces standalone hashtag tokens with `[HASHTAG]`.
The token begins with `#`, contains one or more ASCII letters, digits, or
underscores, and must not be immediately preceded by another ASCII word byte.
That boundary recognizes tags after whitespace and punctuation while leaving
embedded language tokens such as `C#` and `mid#word` unchanged.

The placeholder replaces only the `#name` span. Surrounding brackets, commas,
parentheses, and slashes remain exactly where they were. A bare `#` has no tag
body and is not matched. Non-ASCII tag characters terminate the recognized
ASCII body rather than being decoded as part of it.

Use this operation when all tag values should collapse to one category. Use a
token extractor when the actual tag identities are needed. The method accepts
Text only, preserves every extent, returns a fresh packed Text Note, and never
mutates the receiver. Empty and undefined behavior follows the normal Note
transformation contract. `[HASHTAG]` is stable under repeated application.

## Example 1: Start, Punctuation, And Embedded Boundaries

```cpp
std::cout << pluss::note::print(
    pluss::note::make({"#root (#ship); [#release_2] C# mid#word"})
        .nlp().mask_hashtags()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["[HASHTAG] ([HASHTAG]); [[HASHTAG]] C# mid#word"]
```

## Example 2: Empty, Undefined, And Bytes

```cpp
const auto empty = pluss::note::make(std::vector<std::string>{})
                       .nlp().mask_hashtags();
std::cout << "hashtags empty dim=" << empty.dim()
          << " numel=" << empty.numel() << '\n';
std::cout << "hashtags undefined=" << std::boolalpha
          << pluss::Note::undefined().nlp().mask_hashtags().defined() << '\n';
try {
  (void)pluss::note::bytes(
      std::vector<std::string>{"#tag"}, std::array<std::int64_t, 1>{1})
      .nlp().mask_hashtags();
} catch (const pluss::Error&) { std::cout << "hashtags Bytes: rejected\n"; }
```

### Output

```text
hashtags empty dim=1 numel=0
hashtags undefined=false
hashtags Bytes: rejected
```

## See Also

- [`mask_mentions`](mask_mentions.md): apply the corresponding rule to `@` handles.
- [`mask_pii`](mask_pii.md): mask hashtags together with other selected categories.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mask_emails.html"><span class="label">Previous</span><span class="title">mask_emails</span></a>
  <a class="next-link" href="mask_mentions.html"><span class="label">Next</span><span class="title">mask_mentions</span></a>
</div>
