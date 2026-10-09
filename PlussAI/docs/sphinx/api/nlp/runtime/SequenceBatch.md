# `pluss::ai::nlp::runtime::SequenceBatch`

One padded batch of sequences, as the [`Indexer`](Indexer.md) produces it and the embedding and encoder
layers consume it. It carries the token ids, the positions, the masks that say which entries are real,
and the length of each sequence. It is a value: the indexer builds it, and callers read its fields or use
its three methods.

## Declaration

```cpp
struct pluss::ai::nlp::runtime::SequenceBatch {
  pluss::Series token_ids;
  pluss::Series position_ids;
  pluss::Series valid_mask;
  pluss::Series key_padding_mask;
  pluss::Series lengths;
  std::int64_t batch_size = 0;
  std::int64_t max_length = 0;
  bool batch_first = true;
};
```

## State

`token_ids` and `position_ids` have the layout chosen by `batch_first`, shaped `[batch, max_length]` or
`[max_length, batch]`. `valid_mask` is a boolean `[batch, max_length]` series that is true where a real
element sits. `key_padding_mask` is its complement, the form attention layers read. `lengths` holds the
number of real elements in each sequence. Padding entries carry the pad id in `token_ids` and zero in
`position_ids`.

## Methods

Each method has its own page.

| Method | Returns | Covered on |
| --- | --- | --- |
| `empty() const noexcept` | `bool` | [`empty`](SequenceBatch/empty.md) |
| `pack(values) const` | `PackedSequence` | [`pack`](SequenceBatch/pack.md) |
| `causal_mask() const` | `Series` | [`causal_mask`](SequenceBatch/causal_mask.md) |

## See Also

- [`Indexer`](Indexer.md) -- builds batches.
- [`EmbeddingTable`](EmbeddingTable.md) -- reads `token_ids` and `position_ids`.
- [`mean_pool`](mean_pool.md) and [`last_pool`](last_pool.md) -- reduce encoded batches using `lengths`.

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

SequenceBatch/Introduction
SequenceBatch/causal_mask
SequenceBatch/empty
SequenceBatch/pack
```
<!-- apiref:toctree:end -->
