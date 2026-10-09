# `pluss::ai::nlp` Runtime

The runtime is the neural half of the text pipeline: it turns documents into id sequences, the ids into
vectors, the vectors into contextual encodings, and the encodings into one vector per document. Each stage
is a separate object, so a model can use the whole path or any part of it, with the same vocabulary and
batch layout throughout.

The path runs in four steps. An [`Indexer`](runtime/Indexer.md) learns the vocabulary and builds padded
[`SequenceBatch`](runtime/SequenceBatch.md) values. An [`EmbeddingTable`](runtime/EmbeddingTable.md) turns the ids into
vectors. A [`TransformerTextEncoder`](runtime/TransformerTextEncoder.md) or an
[`LstmTextEncoder`](runtime/LstmTextEncoder.md) contextualizes them. [`mean_pool`](runtime/mean_pool.md) or
[`last_pool`](runtime/last_pool.md) reduces each sequence to one vector.

Configuration lives in three option structs: [`SequenceOptions`](runtime/SequenceOptions.md) for how text becomes
ids, [`EmbeddingOptions`](runtime/EmbeddingOptions.md) for the vectors, and the neural options of the encoder.
Every stage that reads a batch checks that the batch's layout matches its own, so a mismatch fails where it
happens rather than producing silently wrong results.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

runtime/Introduction
runtime/SequenceOptions
runtime/SequenceBatch
runtime/Indexer
runtime/EmbeddingOptions
runtime/EmbeddingTable
runtime/TransformerTextEncoder
runtime/LstmTextEncoder
runtime/mean_pool
runtime/last_pool
```
<!-- apiref:toctree:end -->
