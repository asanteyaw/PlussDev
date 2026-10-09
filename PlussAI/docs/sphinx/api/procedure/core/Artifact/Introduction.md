# `pluss::ai::Artifact` Introduction

## What `Artifact` Is

`Artifact` is the one value type every [`Step`](../Step.md) consumes and
produces: a name plus a type-erased payload, drawn from a closed
`std::variant` listing every type `procedure` knows how to carry --
`Series`, `pluss::table::DataFrame`, `pluss::Note`, `pluss::table::Slate`,
every `lymph`/`ice`/`nlp` payload and report type, plus `std::string`,
`std::vector<std::string>`, `double`, `std::int64_t`, and `bool`. A
`Procedure` holds no artifacts of its own; [`Procedure::run`](../Procedure/run.md)
builds a fresh pool of them for each call, seeded from its `inputs`
argument and grown by each step's own result as it runs.

## Why A Closed Variant, Not A Generic `std::any`

Every type an `Artifact` can hold is a type some adapter family in
`procedure` actually passes to or receives from a real engine --
[`lymph`](../../lymph.md)'s `FeatureBatch`/`LabeledBatch`,
[`ice`](../../ice.md)'s `Batch`/`AssessReport`, and so on. Keeping the
list closed (rather than opening it up to any type via `std::any`) means
[`ArtifactTrace`](../ArtifactTrace.md) can give a real, bounded type name
and -- for the types that have one -- a row/column/element count, without
needing any per-type registration step. The tradeoff is that `Artifact`
cannot carry an arbitrary user type; a value that does not fit the
variant has to be wrapped (as a `std::string`, serialized, or similar)
before it can travel through a `Procedure`.

## Related Pages

- [`procedure::input`](../../build/input.md) -- building a named
  `Artifact` directly.
- [`Step::run`](../Step/run.md) -- where a step's own `Artifact` result
  comes from.
- [`ArtifactTrace`](../ArtifactTrace.md) -- the lightweight shape/type
  snapshot an audit record keeps instead of the artifact itself.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="empty.html">
    <span class="label">Next</span>
    <span class="title">empty</span>
  </a>
</div>
<!-- apiref:nav:end -->
