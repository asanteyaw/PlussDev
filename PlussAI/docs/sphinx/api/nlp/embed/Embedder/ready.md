<!-- pluss-hand-authored -->
<!-- pluss-verifier: PlussAI/examples/NodeB/nodeb_embed_docs_reference.cpp -->
<!-- pluss-signature-hash: 87802a211d28 -->
# `pluss::ai::nlp::embed::Embedder::ready`

[Back to `Embedder`](../Embedder.md)

## Signature

```cpp
bool pluss::ai::nlp::embed::Embedder::ready() const noexcept;
```

## What It Does

`ready()` reports whether a complete learned state is installed. Construction alone leaves it false. Successful `learn()` or `load()` makes it true. A failed training call does not make an untrained object ready.

This is an O(1), non-throwing state query. It says nothing about model quality, corpus coverage, or whether a particular term exists; use `contains()` for term membership.

## Practical Examples

### Configured but untrained

```cpp
embed::Embedder model({.width = 12});
std::cout << std::boolalpha << model.ready() << '\n';
```

Output

```text
false
```

### After training

```cpp
const auto status = model.learn(docs);
std::cout << std::boolalpha << status.ok() << ' ' << model.ready() << '\n';
```

Output

```text
true true
```

### After loading

```cpp
const auto restored = embed::Embedder::load(path);
std::cout << std::boolalpha << restored.ready() << '\n';
```

Output

```text
true
```

## See Also

- [`learn()`](learn.md) and [`load()`](load.md) are the two state-producing operations.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nearest.html">
    <span class="label">Previous</span>
    <span class="title">nearest</span>
  </a>
  <a class="next-link" href="save.html">
    <span class="label">Next</span>
    <span class="title">save</span>
  </a>
</div>
<!-- apiref:nav:end -->
