# `ice::platform` Introduction

## What This Category Owns

`platform` owns exactly one thing: `TrainPlatform`, the three-value
device-selection request (`CPU`/`GPU`/`Auto`) every `ice` training
surface accepts, and [`resolve_platform`](resolve_platform.md), the
function that turns a request into a concrete `pluss::Platform` by
querying this machine's real device availability.

## Why An Enum, Not A Bool

A plain `use_gpu` flag can't distinguish "let ice pick the best available
device" from "I explicitly want acceleration, tell me if you can't give
it to me" -- those two intents warrant different fallback behavior (a
warning versus silence), so they get distinct enumerators instead of one
flag plus a separate verbosity toggle:

- `CPU` (the default) -- always CPU, regardless of what's available.
- `GPU` -- CUDA if ready, else MPS if ready, else `pluss::validate::warn`
  fires and training falls back to CPU anyway.
- `Auto` -- the same fallback order as `GPU`, silently.

## Where It's Used

`TrainPlatform` appears in exactly one place across the rest of `ice`:
[`ExecutionPolicy::platform`](../options/ExecutionPolicy.md), itself one
field of [`TrainOptions`](../options/TrainOptions.md) and every model
catalog entry's own `*TrainOptions`. [`Model::learn`](../model/Model.md)
(and the models_ts/models_nlp/models_rl catalog's own `learn`/`to`
methods) resolve it exactly once per call via
[`resolve_platform`](resolve_platform.md) and move both the model and the
training/validation data there; `infer`/`assess` instead match wherever
the model's parameters currently live, so a model trained with
`TrainPlatform::GPU` stays usable afterward without the caller
re-specifying a device.

## Explicitly Out Of Scope

There is no way to select a specific GPU by index, and no distributed or
multi-GPU training support. The underlying engine (`nn`/CodeM) has no
per-device addressing or collective-communication backend at all yet, so
there is nothing here for a convenience layer to wrap -- this is a gap in
the engine `ice` sits on, not a missing `ice` convenience. See
`PlussAI/Ice/ICE_BLUEPRINT.md` for the full reasoning.

## Reading Order

1. [`resolve_platform`](resolve_platform.md) -- the only symbol in this
   category; its own page covers all three `TrainPlatform` values with
   verified output.

## Related Pages

- [`options`](../options.md) -- `ExecutionPolicy.platform` is where a
  `TrainPlatform` request actually gets set.
- [`model`](../model.md) -- `Model::learn`/`infer`/`assess`/`to` are
  where a resolved platform takes effect.
