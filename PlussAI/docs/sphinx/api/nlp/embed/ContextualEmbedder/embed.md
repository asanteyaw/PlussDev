<!-- pluss-hand-authored -->
<!-- pluss-verifier: PlussAI/examples/NodeB/nodeb_embed_docs_reference.cpp -->
<!-- pluss-signature-hash: e630023ff8e7 -->
# `pluss::ai::nlp::embed::ContextualEmbedder::embed`

[Back to `ContextualEmbedder`](../ContextualEmbedder.md)

## Signature

```cpp
pluss::ai::nlp::StatusOr<std::vector<double>>
pluss::ai::nlp::embed::ContextualEmbedder::embed(
    const pluss::ai::nlp::Doc& doc) const;
```

## What It Does

`embed()` obtains the same per-token rows as [`encode()`](encode.md) and returns their coordinate-wise arithmetic mean. The result is one owned host vector of length `width()`, suitable when a downstream model needs one fixed-size feature row per document.

Pooling removes token positions and gives every surviving token equal weight. Repeated terms count repeatedly; padding and internal boundary markers are excluded. Use `encode()` when order or individual occurrences matter.

The call fails before training and when preprocessing leaves no content token. Its compute cost is dominated by the Transformer forward pass; averaging adds only linear work.

## Parameters

`doc`
: One prepared document whose contextual rows will be averaged.

## Practical Examples

### Document vector

```cpp
const auto result = model.embed(sentence);
std::cout << std::boolalpha << result.ok() << " width=" << result.value().size() << '\n';
```

Output

```text
true width=16
```

### One vector versus token rows

```cpp
const auto rows = model.encode(sentence).value();
const auto pooled = model.embed(sentence).value();
std::cout << "rows=" << rows.size() << " pooled=" << pooled.size() << '\n';
```

Output

```text
rows=5 pooled=16
```

### Empty content

```cpp
const auto result = model.embed(nlp::Doc{});
std::cout << result.ok() << '\n';
```

Output

```text
false
```

## See Also

- [`encode()`](encode.md) preserves one row per token.
- [`Embedder::embed()`](../Embedder/embed.md) pools inexpensive static rows.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="encode.html">
    <span class="label">Next</span>
    <span class="title">encode</span>
  </a>
</div>
<!-- apiref:nav:end -->
