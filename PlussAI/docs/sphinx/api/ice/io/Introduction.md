# `ice::io` Introduction

## What This Category Owns

`io` owns the two serialization tiers behind every save/load path in
`ice`: a weights-only pair
([`save_parameters`](save_parameters.md)/[`load_parameters`](load_parameters.md))
and a full, training-resumable pair
([`save_checkpoint`](save_checkpoint.md)/[`load_checkpoint`](load_checkpoint.md)),
plus the payload type the latter two exchange
([`CheckpointState`](CheckpointState.md)).

## Two Tiers, One Relationship

The weights-only tier is a strict subset of the full-checkpoint tier --
[`save_checkpoint`](save_checkpoint.md) calls `model.save(...)`
internally (itself built on [`save_parameters`](save_parameters.md)),
then adds optimizer state, [`CheckpointState`](CheckpointState.md), and
callback state on top. Reach for the weights-only pair whenever only
the trained values matter (deploying a model for inference); reach for
the full pair whenever training itself needs to resume (the loop's own
epoch/step position, optimizer momentum, and any stateful callback's
own bookkeeping all need to come back too, not just the weights).

## `load_checkpoint`'s Callback Restoration Is The Caller's Job

`save_checkpoint` writes every passed-in callback's own state directly.
`load_checkpoint`, despite taking the same `callbacks` parameter for
signature symmetry, does **not** restore it for you -- see
[`load_checkpoint`'s own page](load_checkpoint.md) for why
(`on_run_begin`-equivalent resets, which a stateful callback like
`EarlyStopCallback` always runs, would just overwrite an
earlier restoration) and the manual pattern `Coach::learn` itself uses
to apply `CheckpointState::callback_state` correctly, after its own
`on_run_begin`.

## Reading Order

1. [`save_parameters`](save_parameters.md) / [`load_parameters`](load_parameters.md)
   -- the simpler, weights-only pair.
2. [`CheckpointState`](CheckpointState.md) -- the payload the full pair
   exchanges, field by field.
3. [`save_checkpoint`](save_checkpoint.md) / [`load_checkpoint`](load_checkpoint.md)
   -- the full, training-resumable pair, and the correct callback
   restoration pattern.

## Related Pages

- [`model`](../model.md) -- `Model::save`/`load`, built on
  `save_parameters`/`load_parameters`.
- [`coach`](../coach.md) -- `Coach::learn`'s own checkpoint write/
  resume, built on `save_checkpoint`/`load_checkpoint`.
- [`callback`](../callback.md) -- `on_checkpoint_save`/
  `on_checkpoint_load`, the hooks around `Coach`'s own calls into this
  family.
