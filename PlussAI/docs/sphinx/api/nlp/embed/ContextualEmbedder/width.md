<!-- pluss-hand-authored -->
<!-- pluss-verifier: PlussAI/examples/NodeB/nodeb_embed_docs_reference.cpp -->
<!-- pluss-signature-hash: c8ba4c4efa4f -->
# `pluss::ai::nlp::embed::ContextualEmbedder::width`

[Back to `ContextualEmbedder`](../ContextualEmbedder.md)

## Signature

```cpp
std::int64_t pluss::ai::nlp::embed::ContextualEmbedder::width() const noexcept;
```

## What It Does

`width()` returns the configured hidden dimension used by token rows, pooled vectors, attention, and the output projection. It is available before training and restored from saved architecture. It does not imply readiness.

The width must be positive and divisible by `num_heads` when training begins.

## Practical Examples

### Configured width

```cpp
embed::ContextualEmbedder model({.width = 16, .num_heads = 4});
std::cout << model.width() << ' ' << std::boolalpha << model.ready() << '\n';
```

Output

```text
16 false
```

### Match both output forms

```cpp
const auto rows = model.encode(sentence).value();
const auto pooled = model.embed(sentence).value();
std::cout << model.width() << ' ' << rows.front().size() << ' ' << pooled.size() << '\n';
```

Output

```text
16 16 16
```

### Loaded architecture

```cpp
std::cout << embed::ContextualEmbedder::load(path).width() << '\n';
```

Output

```text
16
```

## See Also

- [`ContextualEmbedOptions`](../ContextualEmbedOptions.md) defines width/head compatibility.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="save.html">
    <span class="label">Previous</span>
    <span class="title">save</span>
  </a>
</div>
<!-- apiref:nav:end -->
