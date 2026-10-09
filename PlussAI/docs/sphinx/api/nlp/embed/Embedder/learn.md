<!-- pluss-hand-authored -->
<!-- pluss-verifier: PlussAI/examples/NodeB/nodeb_embed_docs_reference.cpp -->
<!-- pluss-signature-hash: 0a9d7091184b -->
# `pluss::ai::nlp::embed::Embedder::learn`

[Back to `Embedder`](../Embedder.md)

## Signature

```cpp
pluss::ai::nlp::Status pluss::ai::nlp::embed::Embedder::learn(
    const std::vector<pluss::ai::nlp::Doc>& docs);
```

## What It Does

`learn()` trains a complete static embedding state from scratch. It derives terms under [`EmbedOptions`](../EmbedOptions.md), filters by frequency, forms center/context pairs, and optimizes positive and negative examples in vectorized mini-batches. A successful call replaces any prior vocabulary and vectors; it never incrementally extends them.

The input must already be tokenized. If lemma selection is enabled, supply documents whose lemma annotations express the intended normalization. The model does not retain the documents after training.

## Parameters

`docs`
: The prepared training corpus. Its token annotations are read under the model's `EmbedOptions` policy and are not retained.

## Return, Failure, And State

Returns `Status::success()` after learned host vectors have been installed. Validation occurs before optimization. Empty input, non-positive integral controls, non-finite or non-positive learning rate, an empty filtered vocabulary, or a corpus that produces no context pair returns a failure status. Numerical exceptions during optimization become a numerical-failure status.

On success `ready()` is true. Callers should check the returned status before querying. Training cost scales with generated pair count, epochs, negative samples, and width; `batch_size` controls vectorized work per optimizer step.

## Practical Examples

### Train and inspect lifecycle

```cpp
embed::Embedder model(options);
std::cout << std::boolalpha << "before=" << model.ready() << '\n';
const auto status = model.learn(docs);
std::cout << "ok=" << status.ok() << " after=" << model.ready()
          << " vocab=" << model.vocabulary().size() << '\n';
```

Output

```text
before=false
ok=true after=true vocab=8
```

### Frequency filtering

```cpp
embed::EmbedOptions filtered = options;
filtered.min_count = 2;
embed::Embedder model(filtered);
model.learn(prep.infer(std::vector<std::string>{"red blue", "red green"}));
std::cout << std::boolalpha << model.contains("red") << ' '
          << model.contains("blue") << '\n';
```

Output

```text
true false
```

### Invalid configuration

```cpp
embed::EmbedOptions bad = options;
bad.learning_rate = std::numeric_limits<double>::quiet_NaN();
const auto status = embed::Embedder(bad).learn(docs);
std::cout << status.message << '\n';
```

Output

```text
nlp::embed::Embedder::learn: width, window, negative_samples, min_count, epochs, batch_size, and learning_rate must all be positive.
```

### Replacement rather than continuation

```cpp
model.learn(prep.infer(std::vector<std::string>{"cat dog", "dog cat"}));
std::cout << model.contains("cat") << '\n';
model.learn(prep.infer(std::vector<std::string>{"red blue", "blue red"}));
std::cout << model.contains("cat") << ' ' << model.contains("red") << '\n';
```

Output

```text
true
false true
```

## See Also

- [`EmbedOptions`](../EmbedOptions.md) defines the complete training policy.
- [`save()`](save.md) persists a successfully trained state.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="embed.html">
    <span class="label">Previous</span>
    <span class="title">embed</span>
  </a>
  <a class="next-link" href="load.html">
    <span class="label">Next</span>
    <span class="title">load</span>
  </a>
</div>
<!-- apiref:nav:end -->
