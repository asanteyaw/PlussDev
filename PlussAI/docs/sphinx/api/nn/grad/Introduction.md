# `nn::grad` Introduction

## What This Category Owns

`grad` owns exactly one piece of global state -- a `thread_local` boolean
that decides whether an operation on a `Series` that
[tracks its slope](../module/Module.md) records a reverse-mode graph node --
and the two RAII guards built to flip it safely for a scope:

- [`GradMode`](GradMode.md) -- the switch itself: `is_enabled()` reads it,
  `set_enabled(bool)` writes it. Both `static`; there is nothing to
  construct.
- [`AutoGradMode`](AutoGradMode.md) -- constructs by setting the switch to a
  fixed value, remembers whatever value was active before it, and restores
  that value when it goes out of scope.
- [`NoGradGuard`](NoGradGuard.md) -- the common case, spelled out as its own
  type: `AutoGradMode` fixed to `false`. Reach for this whenever the scope's
  intent is simply "don't record a graph here", which is most of the time.

## Why A Global Switch, Not A Per-`Series` Flag

A `Series` already has its own per-value opt-in
(`tracks_slope_(true)`/`tracks_slope()`) -- so why a second, global gate on
top of it? Because "does this value want gradients" and "is this a stretch
of code where gradients should be recorded at all" are different questions
answered by different code. A model's parameters are marked
`tracks_slope_(true)` once, at construction, and stay that way for the
model's entire lifetime. Whether a *particular call* into that model --
one more training step, versus a validation pass, versus plain inference in
production -- should build a graph changes constantly, and is a property of
the call site, not the parameter. `GradMode` is how a call site expresses
that without having to walk every parameter and flip its per-value flag and
flip it back afterward.

An op only actually records a node when **both** conditions hold: at least
one input tracks its slope, *and* `GradMode::is_enabled()` is true at the
moment the op runs. Either gate alone is not enough --
a tracked parameter run under `NoGradGuard` builds nothing; an untracked
plain `Series` run with grad mode enabled also builds nothing, since there
is no sensitivity to attach a node to.

## Why The Guards, Not Just `set_enabled`

`GradMode::set_enabled(bool)` is a plain global write -- nothing stops a
caller from disabling recording and then returning early, or throwing,
before the matching call that would re-enable it, leaving grad mode
disabled for the rest of the thread. `AutoGradMode`/`NoGradGuard` exist so
that restore is a destructor, not a second statement the caller has to
remember and get past every early exit. Reach for `set_enabled` directly
only in the rare case where a stretch of code has no single enclosing scope
to attach a guard to; reach for a guard everywhere else.

## Reading Order

1. [`GradMode`](GradMode.md) -- the switch and its two methods; read this
   first, everything else is built on it.
2. [`AutoGradMode`](AutoGradMode.md) -- the general save/restore guard.
3. [`NoGradGuard`](NoGradGuard.md) -- the specific, far more common
   disable-and-restore guard built on `AutoGradMode`.

## Related Pages

- [`module` Introduction](../module/Introduction.md) -- parameter
  registration and `tracks_slope_`, the other half of "does this op record
  a node".
- [`optim`](../optim.md) -- optimizers read whatever a graph recorded while
  `GradMode` was enabled; a parameter updated entirely under
  `NoGradGuard` has no sensitivity for `step()` to consume.
- [`data`](../data.md) -- `pluss::tils::DataLoader`'s own batch-construction
  code wraps itself in `NoGradGuard`, since building a batch is bookkeeping
  that must never itself become part of a training graph.
