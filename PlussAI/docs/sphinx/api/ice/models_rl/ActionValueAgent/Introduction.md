# `pluss::ai::ice::models::ActionValueAgent` Introduction

## What `ActionValueAgent` Is

`ActionValueAgent` is ice's double-DQN-style discrete action-value
agent: an online Q-network and a periodically-synced target
Q-network, trained from a batch of transitions
(`observation`/`action`/`reward`/`next_observation`/`done`) against
a TD-error objective. It is the only `models_rl` catalog entry, and
the only `models_*` entry with no `data::`-family integration at
all -- there is no `data::DataSet`/`data::TimeSeriesDataSet` path
into it, only [`ActionValueBatch`](../ActionValueBatch.md) built
directly from transition tensors.

## Lifecycle

An agent is constructed once from
[`ActionValueAgentOptions`](../ActionValueAgentOptions.md) (fixing
every shape/hyperparameter) and starts with [`ready()`](ready.md)
`false`. [`learn`](learn.md) trains it in place -- re-initializing
both Q-networks from scratch each call, rather than continuing from
a previous training run; from then on
[`predict`](predict.md)/[`act`](act.md)/[`assess`](assess.md)/
[`save`](save.md) are all available. [`load`](load.md) is the other
way to reach a `ready()` agent, reconstructing both Q-networks'
architecture and weights from a saved artifact with no call to
`learn` at all. [`to`](to.md) genuinely moves both networks' live
parameters between platforms at any point in this lifecycle without
affecting `ready()`.

## `predict` vs `act`

[`predict`](predict.md) is the raw building block -- Q-values for
every action, at every row of a `[N, observation_size]` batch, with
no result type wrapping it. [`act`](act.md) is the usual
single-observation entry point: epsilon-greedy over
[`predict`](predict.md)'s own output, accepting either a flat
`[observation_size]` vector or a `[1, observation_size]` row (where
`predict` itself requires the 2D shape).

## Related Pages

- [`ActionValueAgentOptions`](../ActionValueAgentOptions.md) -- the
  constructor's configuration.
- [`ActionValueBatch`](../ActionValueBatch.md) -- the transition
  batch type every `learn`/`assess` call reads.
- [`ActionValueTrainOptions`](../ActionValueTrainOptions.md) -- every
  [`learn`](learn.md) knob, including the Huber-vs-MSE loss toggle.
- [`ActionValueReport`](../ActionValueReport.md) -- what
  [`assess`](assess.md) returns.
- [`models_ts`](../../models_ts.md) / [`models_nlp`](../../models_nlp.md)
  -- the other two `models_*` families, both with `data::`-family
  integration this one lacks.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="act.html">
    <span class="label">Next</span>
    <span class="title">act</span>
  </a>
</div>
<!-- apiref:nav:end -->
