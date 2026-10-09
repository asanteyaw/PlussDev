<!-- pluss-hand-authored -->
<!-- pluss-verifier: PlussAI/examples/NodeB/nodeb_embed_docs_reference.cpp -->
<!-- pluss-signature-hash: f787d0ce9a8c -->
# `pluss::ai::nlp::embed::Embedder::width`

[Back to `Embedder`](../Embedder.md)

## Signature

```cpp
std::int64_t pluss::ai::nlp::embed::Embedder::width() const noexcept;
```

## What It Does

`width()` returns the configured coordinate count for every token and document vector. Unlike model-dependent queries, it is meaningful before training because construction already fixes the option. Loading restores the saved width.

The query is O(1), non-throwing, and does not imply readiness. A positive width is validated when `learn()` runs.

## Practical Examples

### Before training

```cpp
embed::Embedder model({.width = 12});
std::cout << model.width() << ' ' << std::boolalpha << model.ready() << '\n';
```

Output

```text
12 false
```

### Match query output

```cpp
std::cout << model.width() << ' ' << model.vector_of("cat").value().size() << '\n';
```

Output

```text
12 12
```

### Restored width

```cpp
const auto restored = embed::Embedder::load(path);
std::cout << restored.width() << '\n';
```

Output

```text
12
```

## See Also

- [`ready()`](ready.md) reports whether vectors are available.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="vocabulary.html">
    <span class="label">Previous</span>
    <span class="title">vocabulary</span>
  </a>
</div>
<!-- apiref:nav:end -->
