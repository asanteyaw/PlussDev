<!-- pluss-hand-authored -->
<!-- pluss-verifier: PlussAI/examples/NodeB/nodeb_embed_docs_reference.cpp -->
<!-- pluss-signature-hash: 295fa37553f8 -->
# `pluss::ai::nlp::embed::ContextualEmbedder::ready`

[Back to `ContextualEmbedder`](../ContextualEmbedder.md)

## Signature

```cpp
bool pluss::ai::nlp::embed::ContextualEmbedder::ready() const noexcept;
```

## What It Does

`ready()` is the constant-time lifecycle query for complete contextual state. It is false after construction and true after successful training or loading. It does not measure accuracy or vocabulary coverage.

## Practical Examples

### New object

```cpp
embed::ContextualEmbedder model(options);
std::cout << std::boolalpha << model.ready() << '\n';
```

Output

```text
false
```

### Trained object

```cpp
const auto status = model.learn(docs);
std::cout << std::boolalpha << status.ok() << ' ' << model.ready() << '\n';
```

Output

```text
true true
```

### Loaded object

```cpp
std::cout << std::boolalpha << embed::ContextualEmbedder::load(path).ready() << '\n';
```

Output

```text
true
```

## See Also

- [`learn()`](learn.md) and [`load()`](load.md) produce ready state.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load.html">
    <span class="label">Previous</span>
    <span class="title">load</span>
  </a>
  <a class="next-link" href="save.html">
    <span class="label">Next</span>
    <span class="title">save</span>
  </a>
</div>
<!-- apiref:nav:end -->
