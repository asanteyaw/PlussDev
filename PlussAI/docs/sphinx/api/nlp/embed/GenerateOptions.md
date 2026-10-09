# `pluss::ai::nlp::embed::GenerateOptions`

`GenerateOptions` controls one call to [`ContextualEmbedder::generate`](ContextualEmbedder/generate.md). It does not mutate the model or change future calls. Greedy and sampled decoding are separate modes: sampling fields are not merely ineffective under greedy decoding; they are not consulted at all.

## Fields

| Field | Type | Meaning | Default | Applies when |
| --- | --- | --- | --- | --- |
| `max_new_tokens` | `std::int64_t` | Maximum number of returned continuation tokens. Generation can stop earlier at end-of-sequence or position capacity. | `20` | Always |
| `greedy` | `bool` | Choose the highest-logit token when true; draw from a probability distribution when false. | `true` | Always |
| `temperature` | `double` | Divide logits before sampling. Values below one sharpen; values above one flatten. Must be finite and positive in sampling mode. | `1.0` | `greedy=false` |
| `top_k` | `std::int64_t` | Retain only the highest-scoring `k` tokens before sampling. Zero uses the full vocabulary. | `0` | `greedy=false` |
| `seed` | `std::uint64_t` | Seed for categorical draws. The same model, prompt, and options reproduce the same continuation. | `0` | `greedy=false` |

`max_new_tokens` must be positive in both modes. A `top_k` larger than the vocabulary is equivalent to no truncation. The returned vector contains only new token strings, never the prompt or the terminal end marker.

## Configuration Examples

### Deterministic continuation

```cpp
pluss::ai::nlp::embed::GenerateOptions options{};
options.max_new_tokens = 8;
options.greedy = true;
```

This is the stable choice for tests and repeatable inference. `temperature`, `top_k`, and `seed` are ignored.

### Reproducible focused sampling

```cpp
pluss::ai::nlp::embed::GenerateOptions options{};
options.max_new_tokens = 12;
options.greedy = false;
options.temperature = 0.8;
options.top_k = 5;
options.seed = 17;
```

Only the five strongest candidates participate at each step, and the fixed seed makes the sequence reproducible.

### Full-vocabulary sampling

```cpp
pluss::ai::nlp::embed::GenerateOptions options{};
options.max_new_tokens = 20;
options.greedy = false;
options.temperature = 1.2;
options.top_k = 0;
options.seed = 91;
```

The higher temperature spreads probability mass across the complete vocabulary, producing more varied and less conservative continuations.

## See Also

- [`ContextualEmbedder::generate`](ContextualEmbedder/generate.md) defines stopping, prompting, and error behavior.

