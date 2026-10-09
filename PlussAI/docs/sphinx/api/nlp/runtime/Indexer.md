# `pluss::ai::nlp::runtime::Indexer`

The vocabulary and batching step of the neural runtime. An indexer learns a vocabulary from a corpus,
assigning each distinct element an integer id, and then turns documents into padded
[`SequenceBatch`](SequenceBatch.md) values of those ids. It is the bridge between text and the embedding
table: nothing downstream sees a word, only the ids the indexer produced.

## Signature

```cpp
pluss::ai::nlp::runtime::Indexer::Indexer(
    pluss::ai::nlp::runtime::SequenceOptions options = {});
```

## Construction

The constructor stores the [`SequenceOptions`](SequenceOptions.md) and does no work. A new indexer has no
vocabulary, and [`batch`](Indexer/batch.md) refuses to run until [`learn`](Indexer/learn.md) or
[`load_state_lines`](Indexer/load_state_lines.md) has been called.

## State

A learned indexer holds its options, its vocabulary in id order, and the ids of its special tokens. The
vocabulary always begins with the pad token at id `0` and the unknown token at id `1`. The bos and eos
tokens follow when the options add them, and every other element is appended in the order it was first
seen during learning.

The state round-trips through [`dump_state_lines`](Indexer/dump_state_lines.md) and
[`load_state_lines`](Indexer/load_state_lines.md). Options are saved with the vocabulary, so a restored
indexer produces the same batches as the original.

## Methods

Each method has its own page.

| Method | Returns | Covered on |
| --- | --- | --- |
| `learn(docs)` | `Status` | [`learn`](Indexer/learn.md) |
| `batch(docs) const` | `StatusOr<SequenceBatch>` | [`batch`](Indexer/batch.md) |
| `options() const` | `const SequenceOptions&` | [`options`](Indexer/options.md) |
| `vocabulary() const` | `const std::vector<std::string>&` | [`vocabulary`](Indexer/vocabulary.md) |
| `pad_id() const` | `std::int64_t` | [`pad_id`](Indexer/pad_id.md) |
| `unk_id() const` | `std::int64_t` | [`unk_id`](Indexer/unk_id.md) |
| `bos_id() const` | `std::int64_t` | [`bos_id`](Indexer/bos_id.md) |
| `eos_id() const` | `std::int64_t` | [`eos_id`](Indexer/eos_id.md) |
| `dump_state_lines() const` | `std::vector<std::string>` | [`dump_state_lines`](Indexer/dump_state_lines.md) |
| `load_state_lines(lines)` | `Status` | [`load_state_lines`](Indexer/load_state_lines.md) |

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Indexer/Introduction
Indexer/batch
Indexer/bos_id
Indexer/dump_state_lines
Indexer/eos_id
Indexer/learn
Indexer/load_state_lines
Indexer/options
Indexer/pad_id
Indexer/unk_id
Indexer/vocabulary
```
<!-- apiref:toctree:end -->
