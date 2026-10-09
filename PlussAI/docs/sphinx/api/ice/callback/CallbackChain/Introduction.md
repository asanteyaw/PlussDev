# `pluss::ai::ice::CallbackChain` Introduction

## What `CallbackChain` Is

`CallbackChain` is the composite in `Callback`'s own hierarchy: a
`Callback` that holds other callbacks and overrides every hook to call
that same hook on each held callback, in order. It is how
[`Coach`](../../coach/Coach.md) runs more than one `Callback` during a
single `learn` call -- `Coach` itself keeps only a flat
`std::vector<std::shared_ptr<Callback>>` (added one at a time via
[`add_callback`](../../coach/Coach.md)), and assembles a `CallbackChain`
from it (plus a built-in
[`EarlyStopCallback`](../EarlyStopCallback.md) when applicable) once,
at the start of every run.

## Why It's A `Callback` Itself

Because `CallbackChain` *is* a `Callback` (public inheritance, not a
separate container type), one chain can be nested inside another, or
handed anywhere a single `Callback` is expected -- including
[`Coach::add_callback`](../../coach/Coach.md)'s own argument. Grouping a
related set of callbacks (a logger, a learning-rate scheduler, a custom
metric publisher) into one named chain and adding that one chain is
equivalent to adding each of them individually; the only difference is
the single grouped object is easier to pass around, reuse, or write a
`state_key` for as a unit.

## What It Does Not Override

`state_key`/`save_state`/`load_state` are left at `Callback`'s own
no-op defaults -- a `CallbackChain` carries no state of its own to
persist. `Coach::learn`'s checkpoint logic walks
[`callbacks()`](callbacks.md) directly, calling each held callback's own
`state_key`/`save_state`/`load_state` individually, rather than treating
the chain as one persistable unit.

## Related Pages

- [`Callback`](../Callback.md) -- every hook this type overrides, and
  what each one means.
- [`EarlyStopCallback`](../EarlyStopCallback.md) -- the one built-in
  callback `Coach` adds to its own internal chain automatically.
- [`Coach`](../../coach/Coach.md) -- builds and runs the chain this type
  assembles.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="add.html">
    <span class="label">Next</span>
    <span class="title">add</span>
  </a>
</div>
<!-- apiref:nav:end -->
