# `pluss::ai::ice::Batch` Introduction

`Batch` owns the shape of data a training step actually computes
against: a flat list of named `Series` fields, plus a separately-tracked
`targets` field. [`Model::forward(const Batch&)`](../../model/Model.md)
and every step method ([`learn_step`](../../model/Model.md)/
[`valid_step`](../../model/Model.md)/
[`test_step`](../../model/Model.md)/
[`infer_step`](../../model/Model.md)) take one; the default `forward`
override reads `inputs()` and calls `forward(const Series&)`.

## Why `inputs`/`targets` Are Named Specially

Every `Model` needs exactly one "the thing to run `forward` on" field and
(outside inference) one "the thing to compare `forward`'s output
against" field -- common enough to get their own typed accessors
(`inputs`/`has_inputs`/`set_inputs`, `targets`/`has_targets`/
`set_targets`) rather than requiring every caller to spell
`field("inputs")` by name. Everything else -- a second or third input
tensor, an auxiliary mask, whatever a particular architecture needs --
goes through the general `field`/`has`/`set` trio instead. The concrete
case: [`DataSetSource::do_transfer_batch`](../../source/DataSetSource.md)
copies every one of a [`data::Batch`](../../data/Batch.md)'s own named
fields straight through by name (aliasing only the field literally named
`"features"` to `set_inputs` as well) -- so a custom `Model` subclass fed
from a [`TimeSeriesDataSet`](../../data/TimeSeriesDataSet.md)-backed
`DataSetSource` would see `static_inputs`/`encoder_inputs`/
`decoder_inputs`/`encoder_target` as ordinary named fields here, read via
`field`/`has`, not through `inputs`. None of the `models_ts` catalog
entries themselves go through `Model`/`Batch` this way -- their own
`learn`/`predict`/`assess` take their own `*Batch` types directly (see
`PlussAI/Ice/ICE_BLUEPRINT.md` Phase 2 for why they don't fit `Model`'s
generic shape) -- this general field path exists for a hand-written
`Model` subclass built around the same multi-tensor data, not for the
catalog entries.

## Who Constructs One

Almost never a caller directly. The common path is
[`Source::transfer_batch`](../../source/Source.md), which converts a
[`data::Batch`](../../data/Batch.md) (what a [`Source`](../../source/Source.md)'s
`learn_batches()`/etc. actually hold) into one of these, already
containing `inputs`/`targets` under those exact names for the common
single-tensor-in, single-tensor-out case. A hand-built `Batch` (via
[`set`](set.md)/[`set_inputs`](set_inputs.md)/
[`set_targets`](set_targets.md)) matters mainly when calling a `Model`
subclass's step methods directly, outside the normal `learn`/`infer`/
`assess` loop -- the same pattern this page's own
[`has`](has.md)/[`field`](field.md)/[`has_targets`](has_targets.md)/
[`targets`](targets.md) examples use.

## Related Pages

- [`Model`](../../model/Model.md) -- `forward`/`compute_loss`/the four
  step methods, all `Batch`-taking.
- [`data::Batch`](../../data/Batch.md) -- the different, Layout-tagged
  batch type a `Source`'s own `learn_batches()`/etc. hold, and what
  `Source::transfer_batch` converts from.
- [`StepContext`](../StepContext.md) / [`StepResult`](../StepResult.md)
  -- the read-only context passed alongside a `Batch`, and the result a
  step method returns.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="field.html">
    <span class="label">Next</span>
    <span class="title">field</span>
  </a>
</div>
<!-- apiref:nav:end -->
