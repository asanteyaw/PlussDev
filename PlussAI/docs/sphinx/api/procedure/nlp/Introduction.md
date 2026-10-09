# `procedure::nlp` Introduction

## What This Category Owns

`nlp` owns the one way to wrap an [`nlp`](../../plussai_nlp.md)
`Workflow` as a [`Step`](../core/Step.md): a single [`step`](step.md)
overload that routes on [`RunOptions::mode`](../core/RunOptions.md) and
is persistable by construction.

## Input Types Accepted

Unlike [`lymph`](../lymph.md)/[`ice`](../ice.md), an `nlp` step accepts
several genuinely different input shapes depending on mode and content:
in `Learn`/`Assess` mode, a text vector, a `pluss::Note`, or a
`std::vector<nlp::Doc>`; in the default `Infer` mode, a single
`std::string`, a text vector, or a `pluss::Note`. This mirrors
`nlp::Workflow`'s own overloaded `learn`/`assess`/`infer` methods
directly -- the step adds no new input-shape rules of its own.

## Related Pages

- [`nlp`](../../plussai_nlp.md) -- the framework `Workflow` belongs to.
- [`lymph`](../lymph.md) / [`ice`](../ice.md) -- the sibling adapters
  for other engines.
