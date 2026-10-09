<!-- pluss-hand-authored -->
<!-- pluss-verifier: PlussAI/examples/NodeB/nodeb_embed_docs_reference.cpp -->
<!-- pluss-signature-hash: b956ee93b680 -->
# `pluss::ai::nlp::embed::Embedder::contains`

[Back to `Embedder`](../Embedder.md)

## Signature

```cpp
bool pluss::ai::nlp::embed::Embedder::contains(std::string_view token) const noexcept;
```

## What It Does

`contains()` answers whether `token` is an exact key in the learned vocabulary. It does not tokenize a phrase, apply the model's case or lemma policy to the supplied string, or substitute the unknown token. Pass the same canonical term spelling represented by `vocabulary()`.

The query is a hash lookup and does not allocate model output. An untrained model has no vocabulary and returns `false`; absence is ordinary query data rather than an error. Use [`vector_of()`](vector_of.md) when absence must carry a diagnostic.

## Parameter And Return

`token`
: One complete vocabulary term. The view is borrowed only for the duration of the call.

Returns `true` for an exact stored term and `false` otherwise. The model is unchanged.

## Practical Examples

### Learned, case-sensitive membership

```cpp
std::cout << std::boolalpha
          << "cat=" << model.contains("cat")
          << " CAT=" << model.contains("CAT")
          << " moon=" << model.contains("moon") << '\n';
```

Output

```text
cat=true CAT=false moon=false
```

### Before training

```cpp
embed::Embedder configured({.width = 12});
std::cout << std::boolalpha << configured.contains("cat") << '\n';
```

Output

```text
false
```

### Vocabulary-safe retrieval

```cpp
if (model.contains(term)) {
  const auto vector = model.vector_of(term).value();
  std::cout << vector.size() << '\n';
}
```

Output

```text
12
```

## See Also

- [`vocabulary()`](vocabulary.md) exposes every exact key.
- [`vector_of()`](vector_of.md) returns the learned row or a failed status.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="embed.html">
    <span class="label">Next</span>
    <span class="title">embed</span>
  </a>
</div>
<!-- apiref:nav:end -->
