<!-- pluss-hand-authored -->
# Note::NlpAccessor::remove_parentheticals

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note Note::NlpAccessor::remove_parentheticals() const
```

## Remove Balanced Parenthetical Asides

`remove_parentheticals()` removes balanced `(...)` spans, including nested content, while leaving unmatched delimiters in place. Whitespace is normalized after removal.

Bracketed and braced spans follow separate structural cleanup operations; URL parentheses can also be structural content, so inspect the intended corpus.

The operation is elementwise over cells and returns a newly allocated, shape-preserving Text `Note`; rank, extents, and row-major cell order match the receiver exactly. It does not mutate or flatten the source.

An undefined receiver propagates undefined. A defined empty Note keeps its original empty shape. Bool, Int64, Float64, and Bytes receivers are rejected because linguistic analysis is defined on UTF-8 Text cells.

The implementation processes cells independently and preserves their order. Its work is linear in the input bytes plus any ordering or ranking step described above; returned text is owned by the result, so it remains valid after the receiver or temporary accessor leaves scope.

## Example 1: Representative Input And Exact Boundaries

```cpp
const auto markup = pluss::note::make({
    "Keep **bold** [label](https://x.test) and $x+y$; mention @ada.",
    "Quote \"remove me\" (draft) then keep.\\section{Hidden} Visible"});
const auto result = markup.nlp().remove_parentheticals();
std::cout << pluss::note::print(result) << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["Keep **bold** [label] and $x+y$; mention @ada.", "Quote \"remove me\" then keep.\\section{Hidden} Visible"]
```

The result is a shape-preserving Text `Note`, not a `TokenBatch`: each cell's rewritten text replaces the original cell in place, and the printed shape matches the receiver's shape exactly.

## Example 2: Absence And Dtype Validation

```cpp
const auto absent = pluss::Note::undefined().nlp().remove_parentheticals();
std::cout << "defined=" << std::boolalpha << absent.defined() << '\n';
try {
  (void)pluss::note::bytes(
      std::vector<std::string>{"text"},
      std::array<std::int64_t, 1>{1}).nlp().remove_parentheticals();
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

- [`remove_brackets`](remove_brackets.md) / [`remove_quotes`](remove_quotes.md): the sibling aside-stripping methods.
- [`clean`](clean.md): run every structural cleanup method in one pass.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="remove_mentions.html"><span class="label">Previous</span><span class="title">remove_mentions</span></a>
  <a class="next-link" href="remove_quotes.html"><span class="label">Next</span><span class="title">remove_quotes</span></a>
</div>
