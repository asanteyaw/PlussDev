<!-- pluss-hand-authored -->
<!-- pluss-verifier: PlussAI/examples/NodeB/nodeb_embed_docs_reference.cpp -->
<!-- pluss-signature-hash: 9d584b8449c0 -->
# `pluss::ai::nlp::embed::ContextualEmbedder::load`

[Back to `ContextualEmbedder`](../ContextualEmbedder.md)

## Signature

```cpp
static pluss::ai::nlp::embed::ContextualEmbedder
pluss::ai::nlp::embed::ContextualEmbedder::load(std::string_view filepath);
```

## What It Does

`load()` verifies and reconstructs the indexer, token/position embeddings, Transformer architecture and parameters, and output head from one artifact. The returned model is in inference mode and ready immediately. Encoding and greedy generation match the saved model exactly on the same prepared input.

Header, checksum, indexed state, parameter count/shape, and output-head presence are validated before return. Empty paths, unreadable files, corruption, malformed state, and architecture mismatches throw a public runtime diagnostic; no partial model escapes.

## Parameters

`filepath`
: Existing contextual-model artifact path.

## Practical Examples

### Exact encoding round trip

```cpp
const auto before = model.encode(sentence).value();
const auto restored = embed::ContextualEmbedder::load(path);
std::cout << std::boolalpha << restored.ready() << ' '
          << (restored.encode(sentence).value() == before) << '\n';
```

Output

```text
true true
```

### Width restoration

```cpp
std::cout << restored.width() << '\n';
```

Output

```text
16
```

### Replace a configured object

```cpp
embed::ContextualEmbedder current({.width = 32});
current = embed::ContextualEmbedder::load(path);
std::cout << current.width() << ' ' << std::boolalpha << current.ready() << '\n';
```

Output

```text
16 true
```

## See Also

- [`save()`](save.md) writes the matching artifact.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="learn.html">
    <span class="label">Previous</span>
    <span class="title">learn</span>
  </a>
  <a class="next-link" href="ready.html">
    <span class="label">Next</span>
    <span class="title">ready</span>
  </a>
</div>
<!-- apiref:nav:end -->
