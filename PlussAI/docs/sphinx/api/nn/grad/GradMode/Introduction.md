# `pluss::ai::nn::GradMode` Introduction

`GradMode` owns one piece of state: a `thread_local bool`, shared across the
whole engine (not scoped to `nn`), that decides whether an operation on a
`Series` that tracks its slope is allowed to record a reverse-mode graph
node. It has no per-instance lifecycle to speak of -- both its methods are
`static`, there is no object to construct, and the state it reads and
writes outlives any single call.

The state defaults to `true` on every new thread, so recording happens by
default and a caller opts *out* for a scope (via
[`AutoGradMode`](../AutoGradMode.md)/[`NoGradGuard`](../NoGradGuard.md), or
directly via `set_enabled(false)`) rather than opting in. It is `ANDed`
with a `Series`' own per-value `tracks_slope()` flag, never a replacement
for it: a tracked parameter run while `GradMode::is_enabled()` is `false`
records nothing, and an untracked plain `Series` run while it is `true`
also records nothing, since there is no sensitivity to attach a node to.
Both gates have to hold.

Being `thread_local` rather than a single process-wide flag means one
thread disabling recording (say, a background thread running inference)
never affects what another thread concurrently doing a training step
observes -- each thread's `GradMode::is_enabled()` reflects only guards and
`set_enabled` calls made on that same thread.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="is_enabled.html">
    <span class="label">Next</span>
    <span class="title">is_enabled</span>
  </a>
</div>
<!-- apiref:nav:end -->
