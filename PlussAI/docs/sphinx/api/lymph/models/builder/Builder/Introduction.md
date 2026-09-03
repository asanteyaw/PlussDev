# `lymph::models::Builder` Introduction

## What This Type Represents

`Builder` is the candidate registry you reach for before spotchecking,
benchmarking, or search.

It solves a coordination problem, not a fitting problem:

- stable model names
- duplicate protection
- registry inspection
- remembered comparison outcomes

## Why It Lives Under `models::builder`

This object still belongs to the `models` family because it owns candidate
model identity. The `builder` subfamily exists only to narrow that concern from
"one trainable artifact" to "many candidate artifacts under one named
workspace."

## Reading Order

1. `inject`
2. `has`
3. `get`
4. `report`
5. `benchmark`
6. `best`
7. `worst`

## Notes

### Registry

`Builder` is not a generic map. It is a candidate-model workspace. That is why
names matter so much here: they become the stable identity used by reports and
comparison results.

### Benchmark memory

The central semantic point is non-destructive benchmark memory: the registry
keeps the originally stored candidates, `benchmark()` stores comparison outcomes
separately, `best()` and `worst()` return fitted remembered artifacts, and
`get(name)` still returns the original registered entry.

### Inspection by cost

`has()` for presence, `names()` for full identity, `list()` for a quick preview,
`report()` for tabular inspection.

## Related Pages

- [builder subfamily](../Introduction.md)
- [Model type](../../Model.md)

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="benchmark.html">
    <span class="label">Next</span>
    <span class="title">benchmark</span>
  </a>
</div>
<!-- apiref:nav:end -->
