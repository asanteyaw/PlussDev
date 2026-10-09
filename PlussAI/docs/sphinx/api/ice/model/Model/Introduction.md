# `pluss::ai::ice::Model` Introduction

## What This Category Owns

`model` owns exactly one type: `Model`, the base class every trainable
`ice` model derives from -- directly (a hand-written subclass) or
through [`Outline`](../../outline/Outline.md) (the one-liner convenience
for a plain chain of layers). None of the `models_ts`/`models_nlp`/
`models_rl` catalog entries derive from it; their own training shapes
(multi-tensor batches, architecture discovered from data, full-batch
training, a different convergence rule) don't fit `Model`'s generic
single-`Series`-in/single-`Series`-out, mini-batch assumption -- see
`PlussAI/Ice/ICE_BLUEPRINT.md` Phase 2 for why.

## The Override Ladder

`Model` is designed so a subclass overrides only the lowest level that
actually needs to change, and every default implementation above that
level delegates downward unchanged:

1. `forward(const Series&)` -- pure virtual, every subclass must
   implement this.
2. `compute_loss(predictions, y, kind)` -- defaults to dispatching on
   `kind` to the matching `nn::losses::` function; override for a
   custom objective.
3. `learn_step`/`valid_step`/`test_step`/`infer_step` -- default to
   calling `forward` then `compute_loss` (the first three) or just
   `forward` (the last); override one specifically to change what
   happens at that stage only, leaving the others' defaults in place.
4. `learn`/`infer`/`assess` -- not virtual; always run through
   [`Coach`](../../coach/Coach.md), which calls the step methods above.
   Nothing to override here -- a subclass's own `forward`/`compute_loss`/
   step overrides are what actually change `learn`'s behavior.

See [`StepContext`](../../step/StepContext.md)'s own example for a
concrete override (a logging `learn_step` that still delegates to
`Model::learn_step` for the actual work).

## Relationship To `Outline` And `Coach`

- [`Outline`](../../outline/Outline.md) is a `Model` subclass, not a
  separate thing -- it overrides only `forward`, composing a
  [`Schedule`](../../../nn/module/Schedule.md) internally. Every `Model`
  method documented here works on an `Outline` exactly as written.
- [`Coach`](../../coach/Coach.md) is where `learn`/`infer`/`assess`'s
  actual orchestration lives -- every `Model::learn`/`infer`/`assess`
  overload is a thin wrapper around `Coach(options).learn(*this, ...)`/
  `infer(*this, ...)`/`assess(*this, ...)`. `Coach` is also usable
  directly, against any `Model`, without going through these
  convenience wrappers.

## Related Pages

- [`outline`](../../outline.md) -- `Outline`, the common-case `Model`
  subclass.
- [`coach`](../../coach.md) -- the orchestration `learn`/`infer`/
  `assess` delegate to.
- [`step`](../../step.md) -- `Batch`/`StepContext`/`StepResult`, the
  types every step method's override ladder operates on.
- [`options`](../../options.md) -- `TrainOptions`, what every `learn`
  overload takes.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="assess.html">
    <span class="label">Next</span>
    <span class="title">assess</span>
  </a>
</div>
<!-- apiref:nav:end -->
