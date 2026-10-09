# `pluss::ai::nlp::IStage` Introduction

## What A Stage Is

A stage is one step in a pipeline: it reads a [`Doc`](../Doc.md), adds or
changes annotations, and returns control. The pipeline runs stages in order
over each document, so a later stage can rely on everything an earlier
stage wrote. Tokenization comes first because every other stage needs
`tokens`; sentence detection and tagging follow; syntax needs tags; and
textcat needs tokens and lemmas.

## Three Ways A Stage Participates

A stage uses the interface in one of three shapes, and the rest of its
members stay at their defaults:

- **Inference only.** Override `name` and `apply`. The stage transforms
  documents and never learns or saves anything. A rule-based sentence
  splitter is this shape.
- **Trainable.** Also override `learn` to read labeled or raw documents
  before inference, then `apply` uses what `learn` recorded. A tagger
  trained on tagged text is this shape, and it also needs the state methods
  below if it is to survive a save.
- **Persistent.** Also override `dump_state_lines` and `load_state_lines`,
  so the stage's configuration and learned state can be written into a saved
  workflow and restored in a fresh process.

## Ordering Within A Pipeline

Stages run in list order, and `apply` is called once per document per stage.
A stage must not assume it sees the documents of a particular batch, since
`Workflow::infer` processes each input independently. Write `apply` so that
the result for one document depends only on that document and the stage's
own state.

## Related Pages

- [`Workflow`](../../workflow/Workflow.md) -- runs an ordered stage list.
- [`tasks`](../../tasks.md) -- the built-in stage catalog, all implementing this
  interface.
- [`registry`](../../registry.md) -- how a custom stage is found again by name
  when a saved workflow is loaded.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="apply.html">
    <span class="label">Next</span>
    <span class="title">apply</span>
  </a>
</div>
<!-- apiref:nav:end -->
