<!-- pluss-hand-authored -->
<!-- pluss-verifier: PlussAI/examples/NodeB/nodeb_embed_docs_reference.cpp -->
<!-- pluss-signature-hash: 6fbd5667c605 -->
# `pluss::ai::nlp::embed::Embedder::vector_of`

[Back to `Embedder`](../Embedder.md)

## Signature

```cpp
pluss::ai::nlp::StatusOr<std::vector<double>>
pluss::ai::nlp::embed::Embedder::vector_of(std::string_view token) const;
```

## What It Does

`vector_of()` copies the learned static row for one exact vocabulary key. It does not tokenize, lowercase, or lemmatize the supplied string; use the spelling exposed by `vocabulary()` or check with `contains()` first.

The returned vector has `width()` elements and owns its storage. Mutating it cannot alter future model queries. Copying is intentional at this host boundary; use `nearest()` when the goal is ranking rather than acquiring coordinates.

## Parameters

`token`
: One exact vocabulary key. The string view is borrowed only during the lookup.

## Errors

The status fails if the model is not ready or if the key is absent. Those are distinct diagnostics, so callers do not need to infer model state from an empty vector.

## Practical Examples

### Retrieve a row

```cpp
const auto result = model.vector_of("cat");
std::cout << "ok=" << result.ok() << " width=" << result.value().size() << '\n';
```

Output

```text
ok=true width=12
```

### Returned storage is independent

```cpp
auto first = model.vector_of("cat").value();
const double learned = first.front();
first.front() = 999.0;
std::cout << std::boolalpha
          << (model.vector_of("cat").value().front() == learned) << '\n';
```

Output

```text
true
```

### Unknown key

```cpp
std::cout << model.vector_of("moon").status().message << '\n';
```

Output

```text
nlp::embed::Embedder::vector_of: 'moon' is out of vocabulary.
```

### Untrained model

```cpp
embed::Embedder configured({.width = 12});
std::cout << configured.vector_of("cat").status().message << '\n';
```

Output

```text
nlp::embed::Embedder::vector_of: model has not been trained.
```

## See Also

- [`embed()`](embed.md) averages rows for documents.
- [`contains()`](contains.md) checks membership without copying a row.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="similarity.html">
    <span class="label">Previous</span>
    <span class="title">similarity</span>
  </a>
  <a class="next-link" href="vocabulary.html">
    <span class="label">Next</span>
    <span class="title">vocabulary</span>
  </a>
</div>
<!-- apiref:nav:end -->
