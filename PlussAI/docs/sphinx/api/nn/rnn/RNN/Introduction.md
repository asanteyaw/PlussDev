# `nn::RNN` Introduction

## What This Object Is

`RNN` is the plain (Elman) recurrent layer: at every timestep, it combines
that timestep's input and its own previous hidden state through **one**
learned linear projection, then applies a nonlinearity (`tanh` or `relu`).
That single-projection-per-step design is what distinguishes it from
[`GRU`](../GRU.md) (three projections per step, gated) and
[`LSTM`](../LSTM.md) (four projections per step, gated, plus a separate
cell state) -- `RNN` is the cheapest of the three per timestep, and also
the one most prone to vanishing/exploding gradients over long sequences,
since nothing in its formula is designed to preserve gradient magnitude
across many timesteps the way a gate can.

## The Per-Timestep Formula

With `split_bias == false` (the default):

```text
h_t = nonlinearity(x_t @ weight_ih^T + bias + h_{t-1} @ weight_hh^T)
```

With `split_bias == true`, the single `bias` becomes two independent
terms:

```text
h_t = nonlinearity(x_t @ weight_ih^T + bias_ih + h_{t-1} @ weight_hh^T + bias_hh)
```

Both forms are mathematically capable of the same function (a combined
bias is just the sum of what two split biases would have been), but they
are **not interchangeable weight-for-weight** -- switching `split_bias`
after training discards whatever bias values existed, since the
underlying tensors are different shapes/roles entirely.

## Shape Conventions Shared With `GRU`/`LSTM`

- Input: `[seq_len, batch, input_size]` by default (time-major), or
  `[batch, seq_len, input_size]` when `options().batch_first == true`.
- Hidden state (`hx`, the optional second `forward` argument, and the
  second element of the returned tuple): `[num_layers * dirs, batch,
  hidden_size]`, where `dirs` is `2` if `bidirectional` else `1`.
- Output: same layout convention as the input (time-major or batch-first,
  matching `batch_first`), with the last axis `hidden_size * dirs` wide.

[`GRU`](../GRU.md) follows this exact same convention for its single
hidden state; [`LSTM`](../LSTM.md) follows it for *both* of its two
states (hidden and cell).

## Choosing `RNN` Over `GRU`/`LSTM`

There is rarely a reason to prefer plain `RNN` over `GRU` for a new model
-- `GRU` costs three times the per-step projections but is dramatically
more resistant to vanishing gradients on longer sequences, which usually
outweighs the extra compute. Reach for `RNN` specifically when:

1. The sequences are short enough that vanishing gradients are not a
   practical concern, and the cheaper per-step cost matters.
2. You are deliberately reproducing or comparing against a plain-RNN
   baseline.

## Related Pages

- [`module` Introduction](../../module/Introduction.md) -- the `Module`
  contract `RNN` implements.
- [`GRU`](../GRU.md) / [`LSTM`](../LSTM.md) -- the gated alternatives.
- [`PackedSequence`](../PackedSequence.md) -- the variable-length-batch
  input form `forward` also accepts.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="forward.html">
    <span class="label">Next</span>
    <span class="title">forward</span>
  </a>
</div>
<!-- apiref:nav:end -->
