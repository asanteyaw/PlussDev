# `pluss::ai::Procedure` Introduction

## What `Procedure` Is

`Procedure` is a validated, immutable DAG: a list of named
[`Step`](../Step.md)s and a list of [`Link`](../Link.md)s connecting their
named inputs/outputs. It is PlussAI's production coordination layer --
`PlussAI/PROCEDURE_DESIGN.md` frames the question it answers as "how does
a deployable system admit data, run the
right components in the right order, record what happened, enforce safety
rules, and emit results?", distinct from any one engine's own "how does
this one path turn input into a prediction?" A `Procedure` may wire
together steps built from several different engines at once -- a
[`lymph`](../../lymph.md) preprocessing policy feeding an
[`ice`](../../ice.md) model feeding an [`nlp`](../../nlp.md) workflow --
without any of those engines knowing about the others.

## Lifecycle

A `Procedure` is built once, via [`procedure::make`](../../build/make.md)
(by far the common path) or the constructor directly, from a step list and
a link list. Construction validates the graph immediately --
[`check`](check.md) runs automatically unless
[`ProcedureOptions::validate_on_make`](../ProcedureOptions.md) is set
`false` -- so a `Procedure` value is either internally consistent or it
never exists; there is no "build it, then validate it later" state in
between. From then on a `Procedure` is immutable: [`run`](run.md) can be
called any number of times (each call is independent -- no state carries
between runs), and [`save`](save.md) can be called at any point to persist
the whole wired system. [`procedure::load`](../../persist/load.md) is the
other way to reach a `Procedure` value, reconstructing one from a bundle a
prior `save()` wrote.

## Why Validation Is Automatic, Not Optional-By-Default

Every other PlussAI object that validates user input does so lazily, on
first use. `Procedure` validates eagerly, on construction, because a
`Procedure` is typically built once at deployment time and then run many
times against live data -- a cyclic graph or a dangling link is a wiring
mistake that should fail loud and immediately, not on the first production
request that happens to reach the broken step.
[`ProcedureOptions::validate_on_make`](../ProcedureOptions.md) exists only
for callers who want to construct a `Procedure` value and inspect
[`check_message`](check_message.md) themselves before deciding whether to
treat it as usable -- see its own page for exactly that pattern.

## Related Pages

- [`Step`](../Step.md) -- what one entry in `steps()` actually is.
- [`Link`](../Link.md) -- what one entry in `links()` actually is.
- [`ProcedureResult`](../ProcedureResult.md) -- what `run()` returns.
- [`ProcedureOptions`](../ProcedureOptions.md) -- the construction policy
  struct passed to `make()`/the constructor.
- [`build`](../../build.md) -- `step`/`sink`/`link`/`input`/`make`, the
  free functions used to build a `Procedure` in practice.
- [`persist`](../../persist.md) -- the bundle format behind `save`/`load`.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="check.html">
    <span class="label">Next</span>
    <span class="title">check</span>
  </a>
</div>
<!-- apiref:nav:end -->
