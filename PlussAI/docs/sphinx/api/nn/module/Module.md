# `pluss::ai::nn::Module`

`Module` is the base class every layer, container, and composed network in
`nn` derives from. It owns parameter/buffer/child-module registration,
`train`/`eval` mode (propagated recursively to every registered child),
`to(platform)`/`to(dtype)` moves (likewise recursive), flat and named
introspection over parameters/buffers/child modules, and `snapshot`/`restore`
for checkpointing.

You rarely name `Module` directly. Every concrete layer (`Fusion`, `Conv1d`,
`RNN`, `MultiheadAttention`, `Schedule`, ...) is a `ModuleHolder`-wrapped
value type around a real `Module` subclass (`FusionImpl`, `Conv1dImpl`, ...)
— see [`Introduction`](Introduction.md) for that indirection. `ModuleHolder`
forwards every method on this page, so calling `layer.parameters()` or
`layer.train(false)` on a wrapped layer value reaches `Module`'s own
implementation without you ever touching the `Impl` type.

## Signature

```cpp
explicit nn::Module::Module(bool training = true) noexcept;
virtual nn::Module::~Module() = default;
```

`Module` is not abstract — it has no pure virtual methods, so it is directly
constructible. A bare `Module` has no `forward`/`operator()` of its own
(those are added by each concrete subclass), so a directly-constructed
`Module` is useful mainly as an empty container for manually registering
parameters — the normal way to get one is through a concrete layer type.

## Construction

```cpp
nn::Module m;             // training_ starts true
nn::Module m2(false);     // constructed directly into eval mode
```

Every concrete layer constructs a `Module` base (implicitly, via its own
constructor) and then calls the protected `parameter_record`/`buffer_record`/
`module_record` methods (see [`Introduction`](Introduction.md)) to register
its own state. Those three registration methods are `protected` — not part
of this page's public `## Methods` table — because only a `Module` subclass's
own constructor calls them, never external code.

## State

`is_training()` reports the current mode as a plain `bool` — there is no
separate `Snapshot`-style read for it; it is also embedded in
`Module::Snapshot::training` so a checkpoint captures mode along with values.
`parameter_refs()`/`buffer_refs()`/`module_refs()` (not tabulated below, used
mainly internally) expose this module's *own* registered records — as
`ParameterRef{name, Series*}` etc. — without recursing into children, unlike
`named_parameters()`/`named_buffers()`/`named_modules()`, which do.

## Methods

| Method | Returns | Covered on |
| --- | --- | --- |
| `train(enabled = true)` | `void` | [`train`](Module/train.md) |
| `eval()` | `void` | [`eval`](Module/eval.md) |
| `to(platform)` / `to(dtype)` / `to(platform, dtype)` | `Module&` | [`to`](Module/to.md) |
| `parameters()` | `vector<Series*>` (or `const Series*`) | [`parameters`](Module/parameters.md) |
| `named_parameters()` | `vector<ParameterRef>` (or `ConstParameterRef`) | [`named_parameters`](Module/named_parameters.md) |
| `buffers()` | `vector<Series*>` (or `const Series*`) | [`buffers`](Module/buffers.md) |
| `named_buffers()` | `vector<BufferRef>` (or `ConstBufferRef`) | [`named_buffers`](Module/named_buffers.md) |
| `modules()` | `vector<Module*>` (or `const Module*`) | [`modules`](Module/modules.md) |
| `named_modules()` | `vector<ModuleRef>` (or `ConstModuleRef`) | [`named_modules`](Module/named_modules.md) |
| `snapshot()` | `Module::Snapshot` | [`snapshot`](Module/snapshot.md) |
| `restore(snap, strict = true)` | `Module::RestoreReport` | [`restore`](Module/restore.md) |

Two more methods exist on every `Module` but are not covered on their own
page since they are trivial, one-line compositions of the above:
`is_training()` (returns the `bool` `train`/`eval` last set) and `clear()`
(clears every registered parameter's recorded sensitivity — the gradient
tape's per-parameter accumulator — without touching values; used between
optimizer steps in the rare case you are not already using
`optim::Optimizer::step`, which calls the equivalent internally).

## Changelog

- **0.1** — the eleven-method module contract: registration, mode, device/
  dtype moves, flat and named introspection, checkpointing.

## See Also

- [`Schedule`](Schedule.md) — the container that composes `Module`s.
- [`Fusion`](../fusion/Fusion.md), [`Conv1d`](../cnn/Conv1d.md),
  [`RNN`](../rnn/RNN.md) — representative concrete subclasses.
- [`optim::Optimizer`](../optim/Optimizer.md) — constructed from a `Module&`
  or from `Module::parameters()` directly.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Module/Introduction
Module/buffers
Module/eval
Module/modules
Module/named_buffers
Module/named_modules
Module/named_parameters
Module/parameters
Module/restore
Module/snapshot
Module/to
Module/train
```
<!-- apiref:toctree:end -->
