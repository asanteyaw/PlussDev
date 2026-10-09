<!-- pluss-hand-authored -->
<!-- pluss-verifier: PlussAI/examples/NodeB/nodeb_embed_docs_reference.cpp -->
<!-- pluss-signature-hash: 5d6f0c37f740 -->
# `pluss::ai::nlp::embed::ContextualEmbedder::encode`

[Back to `ContextualEmbedder`](../ContextualEmbedder.md)

## Signature

```cpp
pluss::ai::nlp::StatusOr<std::vector<std::vector<double>>>
pluss::ai::nlp::embed::ContextualEmbedder::encode(
    const pluss::ai::nlp::Doc& doc) const;
```

## What It Does

`encode()` indexes the document under the learned preprocessing policy, runs the causally masked Transformer, removes beginning/end markers and padding, and copies one contextual host row per surviving content token. Row order follows document order, and every row contains `width()` doubles.

Unknown terms use the learned unknown-token id; they do not enlarge the vocabulary. Punctuation may disappear under `skip_punct`, so output row indices correspond to surviving tokens rather than blindly to `doc.tokens`. A fixed model and document produce bitwise-identical rows. Different contexts can produce different rows for the same token identity.

The call fails if the model is unready or no content row survives. Attention cost grows quadratically with sequence length.

## Parameters

`doc`
: One prepared document. Output rows correspond to its surviving content tokens in order.

## Practical Examples

### Shape

```cpp
const auto rows = model.encode(sentence).value();
std::cout << rows.size() << 'x' << rows.front().size() << '\n';
```

Output

```text
5x16
```

### Context changes an occurrence

```cpp
const auto financial = model.encode(prep.infer("the bank raised interest rates")).value();
const auto geographic = model.encode(prep.infer("the river bank flooded overnight")).value();
std::cout << std::boolalpha << (financial[1] != geographic[2]) << '\n';
```

Output

```text
true
```

### Deterministic repeat

```cpp
const auto first = model.encode(sentence).value();
const auto second = model.encode(sentence).value();
std::cout << std::boolalpha << (first == second) << '\n';
```

Output

```text
true
```

### No content

```cpp
std::cout << model.encode(nlp::Doc{}).status().message << '\n';
```

Output

```text
nlp::embed::ContextualEmbedder::encode: document produced no content tokens.
```

## See Also

- [`embed()`](embed.md) mean-pools these rows.
- [`generate()`](generate.md) uses the final encoded position for next-token logits.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="embed.html">
    <span class="label">Previous</span>
    <span class="title">embed</span>
  </a>
  <a class="next-link" href="generate.html">
    <span class="label">Next</span>
    <span class="title">generate</span>
  </a>
</div>
<!-- apiref:nav:end -->
