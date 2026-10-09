# `pluss::ai::Step` Introduction

## What `Step` Is

`Step` is the unit [`Procedure`](../Procedure.md) is built from: a name,
a declared contract ([`StepOptions`](../StepOptions.md) -- capabilities
and port names), an optional persistence recipe
([`StepPersistSpec`](../StepPersistSpec.md)), and a runner closure that
produces one [`Artifact`](../Artifact.md) from a list of input
`Artifact`s. `Step` itself knows nothing about *which* engine a runner
wraps -- that knowledge lives entirely inside the closure
[`procedure::step`](../../build/step.md)'s adapter overloads build. A
`Step` built from the [`lymph`](../../lymph.md) adapter and one built from
the [`module`](../../module.md) adapter are both, as far as `Procedure`
and `Step` are concerned, nothing more than "a name, a contract, and a
closure."

## Capabilities Are Declarations, Not Dispatch

`StepOptions::capabilities` is a set of [`StepCapability`](../Introduction.md)
flags a step advertises about itself. Nothing in `Step` or `Procedure`
reads a capability to decide *how* to run a step -- that is entirely the
runner closure's own business. Capabilities exist so [`check`](../Procedure/check.md)
can reject a step that declares none, so [`has`](has.md) lets a caller or
a future scheduler query intent, and so the whole graph stays
self-describing without needing a separate manifest of what each step is
for.

## Persistence Is Opt-In, Per Step

A `Step` with no [`StepPersistSpec`](../StepPersistSpec.md) (the default
for a plain [`callable`](../../callable.md)/[`module`](../../module.md)
step) is simply not persistable -- [`persistable()`](persistable.md)
returns `false`, and [`Procedure::save`](../Procedure/save.md) refuses to
save a graph containing one. Every built-in engine adapter
([`lymph`](../../lymph.md)/[`ice`](../../ice.md)/[`nlp`](../../nlp.md))
fills in a `StepPersistSpec` automatically; a plain callable needs the
[`registry`](../../registry.md) family's `CustomPersist` overload to
become persistable at all. See [`persistable`](persistable.md)'s own page
for the exact rule, and [`persist`](../../persist.md) for what a saved
step's component artifact actually looks like on disk.

## Related Pages

- [`Procedure`](../Procedure.md) -- the DAG a `Step` is one node of.
- [`StepOptions`](../StepOptions.md) / [`StepPersistSpec`](../StepPersistSpec.md)
  -- the two structs most `Step` accessors just forward.
- [`build`](../../build.md) -- `procedure::step`'s generic overloads.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="can_change_row_count.html">
    <span class="label">Next</span>
    <span class="title">can_change_row_count</span>
  </a>
</div>
<!-- apiref:nav:end -->
