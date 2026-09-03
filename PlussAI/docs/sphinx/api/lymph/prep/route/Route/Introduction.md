# `lymph::prep::Route` Introduction

## What This Type Represents

`Route` is a declarative feature-selection object. It is not the resolved answer
itself — it is the reusable rule that becomes concrete column positions only when
[`resolve(...)`](resolve.md) is called against a real frame and a feature order.

It separates three concerns that otherwise get tangled at every stage call:

- **construction** of the selection rule — a
  [`route::*`](../../route.md) builder
- **resolution** against a specific frame — [`resolve`](resolve.md), run fresh
  for every frame or cross-validation fold
- **remainder policy** — [`with_remainder`](with_remainder.md), what happens to
  the columns the rule did not claim

## Why It Stays A Value

`Route` holds no frame reference and no fitted state — just a kind tag, a name
list or predicate, and the [`remainder`](with_remainder.md) enum. So it copies
freely, can be built once and stored, and resolving it is a cheap host-side
lookup. Build the intent once; apply it everywhere.

## Notes

Routes do not own a universal column order — they resolve against one. That is
why the explicit `resolve(frame, ordered_names)` overload matters: the meaning of
position `2` comes from the feature order the route was told to respect. When one
route is the complement of another, prefer
[`route::remaining_of(other_route)`](../remaining_of.md) over duplicating the
complement name list by hand — it is harder to drift and easier to audit.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="resolve.html">
    <span class="label">Next</span>
    <span class="title">resolve</span>
  </a>
</div>
<!-- apiref:nav:end -->
