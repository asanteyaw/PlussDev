# `nn::LSTM` Introduction

## What This Object Is

`LSTM` is a gated recurrent layer with an explicit **cell state**,
carried alongside the hidden state and updated through a dedicated
forget/input gate mechanism -- the design most directly aimed at
preserving information over long sequences. Four learned projections
(input, forget, cell-candidate, output gates) decide, at every timestep,
how much of the previous cell state to keep, how much new information to
write into it, and how much of the (possibly transformed) cell state to
expose as the hidden output.

## The Per-Timestep Formula

With `split_bias == false` (the default) and `proj_size == 0`, letting
`gates = x_t @ weight_ih^T + bias + h_{t-1} @ weight_hh^T` (split into
four `hidden_size`-wide chunks, in `(input, forget, cell, output)`
order):

```text
i = sigmoid(gates_i)
f = sigmoid(gates_f)
g = tanh(gates_c)
o = sigmoid(gates_o)
c_t = f * c_{t-1} + i * g
h_t = o * tanh(c_t)
```

When `options().proj_size > 0`, one more step runs after `h_t` above:

```text
h_t = h_t @ weight_hr^T
```

`weight_hr` has shape `[proj_size, hidden_size]` -- it projects the raw,
`hidden_size`-wide `o * tanh(c_t)` down to `proj_size`. Note that **only**
the hidden/output state is projected; `c_t` stays `hidden_size`-wide
regardless of `proj_size`, and the *next* timestep's `h_{t-1} @
weight_hh^T` term uses the already-projected (`proj_size`-wide) `h_t`, so
`weight_hh` itself is `[4 * hidden_size, proj_size]` when projecting.

With `split_bias == true`, `gates` splits into two separately-biased
terms (`bias_ih` on the input-side projection, `bias_hh` on the
hidden-side one) instead of one shared `bias` folded only into the
input-side term.

## Shape Conventions Shared With `RNN`/`GRU`

Input and output follow the same layout convention as
[`RNN`'s own conventions](../RNN/Introduction.md). The one
`LSTM`-specific difference: there are **two** recurrent states instead of
one, `(hidden, cell)`, and (when `proj_size > 0`) they are different
widths -- `hidden` is `proj_size`-wide, `cell` stays `hidden_size`-wide.

## Choosing `LSTM` Over `RNN`/`GRU`

Reach for `LSTM` specifically when a model already benefits from
[`GRU`](../GRU.md)-style gating but needs either more capacity in its
memory (a separate cell state, decoupled from what gets exposed as
output) or a cheaper recurrent connection than its memory capacity alone
would imply (via `proj_size`). For most new models without a specific
reason to reach further, [`GRU`](../GRU.md) is the simpler, cheaper
default -- `LSTM` costs one more gate's worth of projection per timestep
and carries a second state to manage.

## Related Pages

- [`module` Introduction](../../module/Introduction.md) -- the `Module`
  contract `LSTM` implements.
- [`RNN`](../RNN.md) / [`GRU`](../GRU.md) -- the ungated and
  simpler-gated alternatives.
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
