# `pluss::ai::nn::GradMode`

`GradMode` is the global switch that decides whether new operations on a
`Series` that tracks its slope record a reverse-mode graph node. It owns no
per-instance state -- both its methods are `static`, and the state they
read/write is a single `thread_local` boolean shared by the whole engine
(not just `nn`), so the switch is process-wide-per-thread, not scoped to any
particular `Module` or `Series`.

You rarely call `GradMode` directly. [`AutoGradMode`](AutoGradMode.md) and
[`NoGradGuard`](NoGradGuard.md) are the RAII wrappers built on top of it, and
cover the common "flip it for this scope, restore it afterward" need without
a caller having to remember the restore call itself — see
[`Introduction`](Introduction.md) for why the guards exist.

## Signature

```cpp
[[nodiscard]] static bool nn::GradMode::is_enabled() noexcept;
static void nn::GradMode::set_enabled(bool enabled) noexcept;
```

`GradMode` has no constructor of its own to call — it is a plain struct of
two static methods, never instantiated.

## Construction

Not applicable. Call `GradMode::is_enabled()` / `GradMode::set_enabled(...)`
directly; there is no object to construct.

## State

A single `thread_local bool`, defaulting to `true` on every new thread. It
is not part of any `Series`, `Module`, or `Snapshot` — it is orthogonal to
(and ANDed with) each `Series`' own per-value `tracks_slope()` flag: an op
only records a node when the relevant input tracks its slope **and**
`GradMode::is_enabled()` is true at the moment the op runs.

## Methods

| Method | Returns | Covered on |
| --- | --- | --- |
| `is_enabled()` | `bool` | [`is_enabled`](GradMode/is_enabled.md) |
| `set_enabled(bool)` | `void` | [`set_enabled`](GradMode/set_enabled.md) |

## Changelog

- **0.1** — the two-method global switch: `is_enabled()` / `set_enabled(bool)`.

## See Also

- [`AutoGradMode`](AutoGradMode.md) -- the general RAII save/restore guard
  built on `set_enabled`/`is_enabled`.
- [`NoGradGuard`](NoGradGuard.md) -- the common disable-for-this-scope case.
- [`module` Introduction](../module/Introduction.md) -- `tracks_slope_`, the
  per-value half of "does this op record a node".

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

GradMode/Introduction
GradMode/is_enabled
GradMode/set_enabled
```
<!-- apiref:toctree:end -->
