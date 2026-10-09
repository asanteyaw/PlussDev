<!-- pluss-hand-authored -->
<!-- pluss-verifier: PlussAI/examples/NodeB/nodeb_embed_docs_reference.cpp -->
<!-- pluss-signature-hash: 420a62b64ef5 -->
# `pluss::ai::nlp::embed::Embedder::embed`

[Back to `Embedder`](../Embedder.md)

## Signature

```cpp
pluss::ai::nlp::StatusOr<std::vector<double>>
pluss::ai::nlp::embed::Embedder::embed(const pluss::ai::nlp::Doc& doc) const;

pluss::ai::nlp::StatusOr<std::vector<std::vector<double>>>
pluss::ai::nlp::embed::Embedder::embed(
    const std::vector<pluss::ai::nlp::Doc>& docs) const;
```

## What It Does

The single-document overload selects each token under the model's lemma, case, and punctuation policy, ignores terms outside the learned vocabulary, and computes the coordinate-wise arithmetic mean of the remaining static vectors. Repetition therefore contributes repeatedly; token order does not affect the result.

The batch overload applies that same operation in input order and returns one row per document. It is fail-fast and atomic from the caller's perspective: if any document has no in-vocabulary term, the result carries that failure instead of a partly filled batch. An empty document vector succeeds with an empty result.

## Parameters

`doc`
: One prepared document to pool.

`docs`
: An ordered document batch; each element is pooled independently.

## Returns And Errors

The row width is exactly `width()`. Returned host vectors own their memory and may be changed without mutating the model.

- Fails with a precondition status before training or loading.
- Fails when a document contributes zero recognized terms, including an empty `Doc`.

Cost is proportional to surviving tokens times vector width; the batch overload currently maps documents sequentially.

## Practical Examples

### One document

```cpp
const auto vector = model.embed(docs.front());
std::cout << "ok=" << vector.ok() << " width=" << vector.value().size() << '\n';
```

Output

```text
ok=true width=12
```

### Ordered document batch

```cpp
const auto vectors = model.embed(std::vector<nlp::Doc>{docs[0], docs[2]});
std::cout << vectors.value().size() << 'x' << vectors.value().front().size() << '\n';
```

Output

```text
2x12
```

### No recognized content

```cpp
const auto result = model.embed(nlp::Doc{});
std::cout << result.status().message << '\n';
```

Output

```text
nlp::embed::Embedder::embed: none of the document's tokens are in vocabulary.
```

### Mean-pooling rule

```cpp
const auto cat = model.vector_of("cat").value();
const auto dog = model.vector_of("dog").value();
const auto pooled = model.embed(prep.infer("cat dog")).value();
std::cout << std::boolalpha
          << (pooled[0] == (cat[0] + dog[0]) / 2.0) << '\n';
```

Output

```text
true
```

## See Also

- [`vector_of()`](vector_of.md) retrieves rather than pools.
- [`ContextualEmbedder::embed()`](../ContextualEmbedder/embed.md) pools context-dependent rows.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="contains.html">
    <span class="label">Previous</span>
    <span class="title">contains</span>
  </a>
  <a class="next-link" href="learn.html">
    <span class="label">Next</span>
    <span class="title">learn</span>
  </a>
</div>
<!-- apiref:nav:end -->
