# `pluss::ai::nlp::embed::ContextualEmbedOptions`

`ContextualEmbedOptions` fixes the vocabulary policy and Transformer architecture of a [`ContextualEmbedder`](ContextualEmbedder.md). The model trains over the complete prepared corpus as one padded, vectorized sequence batch; there is intentionally no inert mini-batch field. Architecture fields determine saved parameter shapes and cannot be changed after loading without retraining.

## Fields

| Field | Type | Meaning | Default |
| --- | --- | --- | --- |
| `use_lemmas` | `bool` | Select token lemmas instead of surface spellings for vocabulary ids. | `true` |
| `lowercase` | `bool` | Lowercase selected token text before indexing. | `true` |
| `skip_punct` | `bool` | Exclude punctuation tokens from modeled content. | `true` |
| `width` | `std::int64_t` | Token, position, encoder, and output-hidden width. It must be divisible by `num_heads`. | `64` |
| `num_layers` | `std::int64_t` | Number of stacked Transformer encoder layers. | `2` |
| `num_heads` | `std::int64_t` | Parallel attention heads per layer. Each head receives `width / num_heads` coordinates. | `4` |
| `feedforward_width` | `std::int64_t` | Hidden width of each layer's position-wise feedforward network. | `128` |
| `epochs` | `std::int64_t` | Maximum full-corpus optimization passes. Training may stop early once loss movement falls below the convergence threshold. | `30` |
| `learning_rate` | `double` | Finite, positive Adam step size. | `0.001` |
| `seed` | `std::uint64_t` | Reproducible embedding, encoder, output-head, and training initialization seed. | `0` |

The three width/depth controls affect both capacity and cost. Attention work grows quadratically with padded sequence length, while layer count and feedforward width grow compute approximately linearly. `width % num_heads == 0` is a hard architectural requirement.

## Configuration Examples

### Small CPU model

```cpp
pluss::ai::nlp::embed::ContextualEmbedOptions options{};
options.width = 32;
options.num_layers = 1;
options.num_heads = 4;
options.feedforward_width = 64;
options.epochs = 40;
options.seed = 3;
```

This is useful for a focused vocabulary and short documents. Every head receives eight coordinates.

### Larger contextual representation

```cpp
pluss::ai::nlp::embed::ContextualEmbedOptions options{};
options.width = 192;
options.num_layers = 4;
options.num_heads = 6;
options.feedforward_width = 512;
options.epochs = 30;
options.learning_rate = 5e-4;
```

The model allocates more representational capacity and uses a smaller step size. Corpus memory still depends heavily on the longest padded sequence.

### Surface-sensitive model

```cpp
pluss::ai::nlp::embed::ContextualEmbedOptions options{};
options.use_lemmas = false;
options.lowercase = false;
options.skip_punct = false;
```

Use this only when surface distinctions and punctuation are intended model inputs. The inference workflow must produce tokens under the same policy.

## See Also

- [`ContextualEmbedder::learn`](ContextualEmbedder/learn.md) validates and consumes these fields.
- [`GenerateOptions`](GenerateOptions.md) controls decoding after training and is independent of this training configuration.

