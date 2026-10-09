# `pluss::ai::nlp::embed::EmbedOptions`

`EmbedOptions` defines both the vocabulary policy and the skip-gram training problem used by [`Embedder`](Embedder.md). The constructor copies this value, so changing the original structure later does not reconfigure an existing model. Every field takes effect on the next `learn()`; the three text-policy fields also govern later `vector_of()`, `embed()`, and restored-model behavior.

## Fields

| Field | Type | Meaning | Default |
| --- | --- | --- | --- |
| `use_lemmas` | `bool` | Use `Token::lemma` rather than `Token::orth`. A missing lemma falls back according to the document's token annotations. | `true` |
| `lowercase` | `bool` | Lowercase the selected term before counting and lookup. Disable it when case carries meaning. | `true` |
| `skip_punct` | `bool` | Exclude tokens marked as punctuation from vocabulary and context pairs. | `true` |
| `width` | `std::int64_t` | Number of learned coordinates per term. Larger values increase capacity, parameter memory, and dot-product work linearly. | `100` |
| `window` | `std::int64_t` | Maximum token distance on either side of a center term used to form positive context pairs. | `5` |
| `negative_samples` | `std::int64_t` | Noise terms drawn for each positive pair. More negatives sharpen discrimination but increase training work proportionally. | `5` |
| `min_count` | `std::int64_t` | Minimum corpus occurrence count required for a term to enter the vocabulary. | `1` |
| `epochs` | `std::int64_t` | Complete passes over the generated center/context pairs. | `5` |
| `batch_size` | `std::int64_t` | Number of positive pairs optimized together. The final batch may be smaller. | `256` |
| `learning_rate` | `double` | Adam step size. It must be finite and greater than zero. | `0.01` |
| `seed` | `std::uint64_t` | Reproducible weight initialization, pair shuffling, and negative sampling seed. | `0` |

All integral training controls must be positive. `min_count` filters before training-pair construction; it can therefore leave an empty vocabulary or documents too short to produce a pair. `width`, `negative_samples`, and `batch_size` are the main memory/throughput controls, while `window`, `epochs`, and corpus size primarily determine total work.

## Configuration Examples

### Compact exploratory model

```cpp
pluss::ai::nlp::embed::EmbedOptions options{};
options.width = 32;
options.window = 2;
options.epochs = 10;
options.seed = 7;

pluss::ai::nlp::embed::Embedder model(options);
```

This favors quick iteration and local relationships. The seed makes repeated training over the same prepared documents reproducible.

### Case-sensitive surface vocabulary

```cpp
pluss::ai::nlp::embed::EmbedOptions options{};
options.use_lemmas = false;
options.lowercase = false;
options.skip_punct = false;
```

`Bank`, `bank`, inflected forms, and punctuation become distinct candidates. This can be useful for source text where capitalization and symbols are semantic, but it grows the vocabulary.

### Filtering a larger corpus

```cpp
pluss::ai::nlp::embed::EmbedOptions options{};
options.width = 128;
options.window = 5;
options.negative_samples = 10;
options.min_count = 5;
options.epochs = 20;
options.batch_size = 1024;
options.learning_rate = 0.005;
```

Frequency filtering prevents singletons from consuming rows. A larger batch amortizes vectorized training work; it does not alter the meaning of an epoch.

## See Also

- [`Embedder::learn`](Embedder/learn.md) validates and consumes these fields.
- [`ContextualEmbedOptions`](ContextualEmbedOptions.md) configures the contextual model instead.

