# `pluss::ai::nlp` Vectorization

Vectorization turns prepared documents into numeric feature matrices for classical
learning. It is a separate layer from workflow annotation because it changes
representation: the output is a dense or sparse feature batch, not another field on
[`Doc`](core/Doc.md). A document must already be tokenized, and it benefits from
lemmas, but vectorization itself runs no stage.

## Mapper Or Hasher

| Property | [`Mapper`](vectorize/Mapper.md) | [`Hasher`](vectorize/Hasher.md) |
| --- | --- | --- |
| Requires `learn()` | Yes | No |
| Column identity | A learned vocabulary term or n-gram | A hash bucket |
| Feature width | The vocabulary size, capped by `max_features` | The fixed `n_features` |
| Supports TF-IDF | Yes | No |
| Unknown term at inference | Dropped | Mapped to whatever bucket it hashes to |
| Collisions | None between vocabulary terms | Distinct terms can share a bucket |

Use `Mapper` when the columns must be named and inspectable, when TF-IDF weighting is
wanted, or when the vocabulary is small enough to store. Use `Hasher` when the input
vocabulary is open or streamed and storing a vocabulary is not worth it. The hasher
trades inspectability for a fixed width: two terms that share a bucket are
indistinguishable in the output.

[`DocMapOptions`](vectorize/DocMapOptions.md) decides which form of each token becomes a
feature and whether punctuation is kept. Requesting lemmas does not run a lemmatizer; a
token with an empty lemma falls back to its surface form, so the lemmatize stage must run
first to make the option do anything.

[`describe`](vectorize/describe.md) summarizes a corpus's size and term distribution
without building a feature matrix. Its [`CorpusStats`](vectorize/CorpusStats.md) result
is the cheapest way to choose `max_features` or `min_document_frequency` before learning.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

vectorize/Introduction
vectorize/DocMapOptions
vectorize/CorpusStats
vectorize/Mapper
vectorize/Hasher
vectorize/describe
```
<!-- apiref:toctree:end -->
