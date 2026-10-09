<!-- pluss-hand-authored -->
<!-- pluss-verifier: PlussAI/examples/NodeB/nodeb_embed_docs_reference.cpp -->
<!-- pluss-signature-hash: b9abaa961745 -->
# `pluss::ai::nlp::embed::ContextualEmbedder::learn`

[Back to `ContextualEmbedder`](../ContextualEmbedder.md)

## Signature

```cpp
pluss::ai::nlp::Status pluss::ai::nlp::embed::ContextualEmbedder::learn(
    const std::vector<pluss::ai::nlp::Doc>& docs);
```

## What It Does

`learn()` builds a vocabulary, frames and pads the corpus, then trains token/position embeddings, a causal Transformer, and an output projection together by next-token prediction. It is unsupervised: each shifted input sequence supplies its own target ids.

Training is full-batch and vectorized. Every epoch processes the complete padded corpus, and optimization may stop before `epochs` when successive losses converge. A successful call replaces prior model state rather than continuing it.

## Parameters

`docs`
: Prepared training documents. They supply token ids and shifted next-token targets and are not retained after training.

## Validation And Failures

The corpus must be nonempty and produce sequences long enough for shifted next-token targets. Width, layer count, head count, feedforward width, epochs, and learning rate must be positive; the rate must be finite; width must divide evenly by head count. Training exceptions become numerical-failure status. The caller must check the returned status.

## Practical Examples

### Train a compact model

```cpp
embed::ContextualEmbedder model(options);
std::cout << std::boolalpha << model.ready() << '\n';
const auto status = model.learn(docs);
std::cout << status.ok() << ' ' << model.ready() << ' ' << model.width() << '\n';
```

Output

```text
false
true true 16
```

### Reject incompatible heads

```cpp
embed::ContextualEmbedOptions bad = options;
bad.width = 10;
bad.num_heads = 4;
std::cout << embed::ContextualEmbedder(bad).learn(docs).message << '\n';
```

Output

```text
nlp::embed::ContextualEmbedder::learn: width must be a multiple of num_heads.
```

### Reject non-finite rate

```cpp
embed::ContextualEmbedOptions bad = options;
bad.learning_rate = std::numeric_limits<double>::quiet_NaN();
std::cout << embed::ContextualEmbedder(bad).learn(docs).ok() << '\n';
```

Output

```text
false
```

## See Also

- [`ContextualEmbedOptions`](../ContextualEmbedOptions.md) defines architecture and vocabulary policy.
- [`save()`](save.md) persists completed inference state.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="generate.html">
    <span class="label">Previous</span>
    <span class="title">generate</span>
  </a>
  <a class="next-link" href="load.html">
    <span class="label">Next</span>
    <span class="title">load</span>
  </a>
</div>
<!-- apiref:nav:end -->
