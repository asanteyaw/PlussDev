# `pluss::ai::nlp::runtime::EmbeddingTable`

The learned lookup that turns a [`SequenceBatch`](SequenceBatch.md) of ids into a batch of vectors. Each
vocabulary id owns one row of weights, and each position owns one row of position weights; a token's vector
is the sum of the two, with padding forced to zero. The table is the first trainable layer of the neural
runtime, and its output feeds an encoder or a pooling step.

## Signature

```cpp
pluss::ai::nlp::runtime::EmbeddingTable::EmbeddingTable(
    pluss::ai::nlp::runtime::EmbeddingOptions options = {});
```

## Construction

The constructor stores the [`EmbeddingOptions`](EmbeddingOptions.md) and allocates nothing. A new table has
no weights, and [`forward`](EmbeddingTable/forward.md) refuses to run until [`reset`](EmbeddingTable/reset.md)
or [`load_state_lines`](EmbeddingTable/load_state_lines.md) has supplied them.

## State

A reset table holds a token weight matrix shaped `[vocab size, width]`, a position weight matrix shaped
`[max_positions, width]` when positions are used, the vocabulary size, and the pad id. The pad row of the
token matrix is zero, so an embedded padding entry carries no signal even before the mask is applied.

The state round-trips through [`dump_state_lines`](EmbeddingTable/dump_state_lines.md) and
[`load_state_lines`](EmbeddingTable/load_state_lines.md), which write the options and both matrices.

## Methods

Each method has its own page.

| Method | Returns | Covered on |
| --- | --- | --- |
| `reset(vocab_size, pad_id)` | `void` | [`reset`](EmbeddingTable/reset.md) |
| `forward(batch) const` | `Series` | [`forward`](EmbeddingTable/forward.md) |
| `options() const` | `const EmbeddingOptions&` | [`options`](EmbeddingTable/options.md) |
| `weights() const` | `const Series&` | [`weights`](EmbeddingTable/weights.md) |
| `position_weights() const` | `const Series&` | [`position_weights`](EmbeddingTable/position_weights.md) |
| `vocab_size() const` | `std::int64_t` | [`vocab_size`](EmbeddingTable/vocab_size.md) |
| `pad_id() const` | `std::int64_t` | [`pad_id`](EmbeddingTable/pad_id.md) |
| `dump_state_lines() const` | `std::vector<std::string>` | [`dump_state_lines`](EmbeddingTable/dump_state_lines.md) |
| `load_state_lines(lines)` | `Status` | [`load_state_lines`](EmbeddingTable/load_state_lines.md) |

## See Also

- [`Indexer`](Indexer.md) -- supplies the vocabulary size and the pad id.
- [`TransformerTextEncoder`](TransformerTextEncoder.md) and [`LstmTextEncoder`](LstmTextEncoder.md) -- consume the output.

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

EmbeddingTable/Introduction
EmbeddingTable/dump_state_lines
EmbeddingTable/forward
EmbeddingTable/load_state_lines
EmbeddingTable/options
EmbeddingTable/pad_id
EmbeddingTable/position_weights
EmbeddingTable/reset
EmbeddingTable/vocab_size
EmbeddingTable/weights
```
<!-- apiref:toctree:end -->
