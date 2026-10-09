<!-- pluss-hand-authored -->
# Note::NlpAccessor::remove_tables

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note Note::NlpAccessor::remove_tables() const
```

## Remove Markdown Table Blocks

`remove_tables()` recognizes pipe-delimited table rows and separator rows, removes the complete table block, and retains prose before and after it with normalized whitespace.

Ordinary prose containing an isolated `|` is not enough to establish a table block.

The operation is elementwise over cells and returns a newly allocated, shape-preserving Text `Note`; rank, extents, and row-major cell order match the receiver exactly. It does not mutate or flatten the source.

An undefined receiver propagates undefined. A defined empty Note keeps its original empty shape. Bool, Int64, Float64, and Bytes receivers are rejected because linguistic analysis is defined on UTF-8 Text cells.

The implementation processes cells independently and preserves their order. Its work is linear in the input bytes plus any ordering or ranking step described above; returned text is owned by the result, so it remains valid after the receiver or temporary accessor leaves scope.

## Example 1: Representative Input And Exact Boundaries

```cpp
const auto input = pluss::note::make({"before\n| A | B |\n|---|---|\n| 1 | 2 |\nafter"});
const auto result = input.nlp().remove_tables();
std::cout << pluss::note::print(result) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["before after"]
```

The result is a shape-preserving Text `Note`, not a `TokenBatch`: each cell's rewritten text replaces the original cell in place, and the printed shape matches the receiver's shape exactly.

## Example 2: Absence And Dtype Validation

```cpp
const auto absent = pluss::Note::undefined().nlp().remove_tables();
std::cout << "defined=" << std::boolalpha << absent.defined() << '\n';
try {
  (void)pluss::note::bytes(
      std::vector<std::string>{"text"},
      std::array<std::int64_t, 1>{1}).nlp().remove_tables();
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

- [`remove_markdown`](remove_markdown.md) / [`remove_html`](remove_html.md): the sibling markup-stripping methods.
- [`clean`](clean.md): run every structural cleanup method in one pass.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="remove_stopwords.html"><span class="label">Previous</span><span class="title">remove_stopwords</span></a>
  <a class="next-link" href="remove_urls.html"><span class="label">Next</span><span class="title">remove_urls</span></a>
</div>
