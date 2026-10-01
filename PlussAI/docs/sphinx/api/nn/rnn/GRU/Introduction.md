# `nn::GRU` Introduction

## What This Object Is

`GRU` is a gated recurrent layer: at every timestep, three learned
projections combine that timestep's input with the previous hidden state
to compute a **reset** gate, an **update** gate, and a **candidate**
hidden value, then blend the candidate with the previous state according
to the update gate. This gating is what makes `GRU` far more resistant to
vanishing gradients over long sequences than plain [`RNN`](../RNN.md),
at the cost of three projections per timestep instead of one.

## The Per-Timestep Formula

With `split_bias == false` (the default), letting `gi = x_t @ weight_ih^T
+ bias` and `gh = h_{t-1} @ weight_hh^T` (each split into three
`hidden_size`-wide chunks, in `(reset, update, candidate)` order):

```text
r = sigmoid(gi_r + gh_r)
z = sigmoid(gi_z + gh_z)
n = tanh(gi_n + r * gh_n)
h_t = n + z * (h_{t-1} - n)
```

With `split_bias == true`, `gi` uses `bias_ih` and `gh` uses `bias_hh`
instead of a single shared `bias` folded only into `gi`.

Reading the last line the other way: `h_t` is a blend of the candidate
`n` and the previous state `h_{t-1}`, weighted by the update gate `z` --
`z` near `0` means "mostly take the new candidate", `z` near `1` means
"mostly keep the old state". This is the mechanism that lets a `GRU`
preserve information across many timesteps when the update gate learns
to stay close to `1`.

## Shape Conventions Shared With `RNN`/`LSTM`

Identical to [`RNN`'s own conventions](../RNN/Introduction.md):
input `[seq_len, batch, input_size]` (or `[batch, seq_len, input_size]`
with `batch_first`), hidden state `[num_layers * dirs, batch,
hidden_size]`, output's last axis `hidden_size * dirs` wide.

## Choosing `GRU` Over `RNN`/`LSTM`

`GRU` is a reasonable default recurrent layer for most new models --
noticeably cheaper per timestep than [`LSTM`](../LSTM.md) (three
projections instead of four, and no separate cell state to carry), while
still far more capable than plain [`RNN`](../RNN.md) on longer sequences.
Reach for `LSTM` instead specifically when a separate cell state (and,
optionally, projection) genuinely helps your architecture; reach for
`RNN` only when the sequences are short enough that gating is not worth
its extra cost.

## Related Pages

- [`module` Introduction](../../module/Introduction.md) -- the `Module`
  contract `GRU` implements.
- [`RNN`](../RNN.md) / [`LSTM`](../LSTM.md) -- the ungated and
  cell-state-carrying alternatives.
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
