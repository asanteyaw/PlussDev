<!-- pluss-hand-authored -->
# NoteNlpAccessor::remove_mentions

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note NoteNlpAccessor::remove_mentions() const
```

## Remove Social Mentions

`remove_mentions()` removes mention tokens recognized by the same boundary rules as `mentions()`, preserving surrounding text and normalizing the resulting whitespace.

Email addresses are not mention tokens and therefore remain.

The operation is elementwise over cells and returns a newly allocated, shape-preserving Text `Note`; rank, extents, and row-major cell order match the receiver exactly. It does not mutate or flatten the source.

An undefined receiver propagates undefined. A defined empty Note keeps its original empty shape. Bool, Int64, Float64, and Bytes receivers are rejected because linguistic analysis is defined on UTF-8 Text cells.

The implementation processes cells independently and preserves their order. Its work is linear in the input bytes plus any ordering or ranking step described above; returned text is owned by the result, so it remains valid after the receiver or temporary accessor leaves scope.

## Example 1: Representative Input And Exact Boundaries

```cpp
const auto markup = pluss::note::make({
    "Keep **bold** [label](https://x.test) and $x+y$; mention @ada.",
    "Quote \"remove me\" (draft) then keep.\\section{Hidden} Visible"});
const auto result = markup.nlp().remove_mentions();
std::cout << pluss::note::print(result) << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["Keep **bold** [label](https://x.test) and $x+y$; mention .", "Quote \"remove me\" (draft) then keep.\\section{Hidden} Visible"]
```

The result is a shape-preserving Text `Note`, not a `TokenBatch`: each cell's rewritten text replaces the original cell in place, and the printed shape matches the receiver's shape exactly.

## Example 2: Absence And Dtype Validation

```cpp
const auto absent = pluss::Note::undefined().nlp().remove_mentions();
std::cout << "defined=" << std::boolalpha << absent.defined() << '\n';
try {
  (void)pluss::note::bytes(
      std::vector<std::string>{"text"},
      std::array<std::int64_t, 1>{1}).nlp().remove_mentions();
} catch (const pluss::Error&) {
  std::cout << "Bytes: rejected\n";
}
```

### Output

```text
defined=false
Bytes: rejected
```

## See Also

- [`remove_hashtags`](remove_hashtags.md) / [`remove_urls`](remove_urls.md) / [`remove_emails`](remove_emails.md): the sibling social-text removers.
- [`mask_mentions`](mask_mentions.md): replace instead of deleting.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="remove_math.html"><span class="label">Previous</span><span class="title">remove_math</span></a>
  <a class="next-link" href="remove_parentheticals.html"><span class="label">Next</span><span class="title">remove_parentheticals</span></a>
</div>
