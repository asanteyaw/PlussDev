# `pluss::ai::nn::Schedule`

`Schedule` is a named, ordered composition of modules and callables run in
sequence -- the container every multi-layer network in this reference is
built from. Each stage is added under a name (explicit or auto-generated),
and each stage's declared output type must match the next stage's declared
input type -- checked at the moment a stage is added, not deferred to the
first `forward` call.

A stage does not have to be an `nn::Module`. Anything exposing
`forward(In) -> Out` or `operator()(In) -> Out` qualifies -- a plain lambda,
a free function object, or a `Module`/`ModuleHolder` type. Non-`Module`
stages contribute no parameters and are skipped by `module_at`, but
otherwise participate in the chain exactly like a layer does. Most chains
are `Series -> Series` throughout (the common case `append` assumes by
default), but `Schedule` is not limited to that: `append_as<In, Out>` lets a
stage declare any input/output type, including `std::tuple<...>` --
`RNN`/`GRU`/`LSTM`'s own `forward` returns a tuple, and a `Schedule` can
chain directly into and out of one.

## Signature

```cpp
nn::Schedule::Schedule();
template <class... Layers> explicit nn::Schedule::Schedule(Layers&&... layers);
```

The variadic constructor calls `reserve(sizeof...(layers))` once, then
`append(...)` on each argument in order -- equivalent to default-constructing
and calling `append` repeatedly, in one expression.

## Construction

```cpp
nn::Schedule empty;                                   // no stages
nn::Schedule net(nn::Fusion(3, 4), nn::Fusion(4, 2));  // two stages, auto-named "0" and "1"
```

Stages are then added or removed with [`append`](Schedule/append.md) /
[`append_as`](Schedule/append_as.md) / [`insert`](Schedule/insert.md) /
[`insert_as`](Schedule/insert_as.md) / [`extend`](Schedule/extend.md) /
[`erase`](Schedule/erase.md). `append` alone additionally takes a same-typed
list in one call -- `net.append({Fusion(...), Fusion(...)})` -- for a run
of identical layer types, so building a deep stack does not need one
`.append(...)` per layer. See [`append`](Schedule/append.md) for the full
set of overloads.

## State

[`size`](Schedule/size.md) / [`empty`](Schedule/empty.md) report the stage
count; [`contains`](Schedule/contains.md) / [`index_of`](Schedule/index_of.md) /
[`name_at`](Schedule/name_at.md) look a stage up by name or position;
[`input_type_at`](Schedule/input_type_at.md) /
[`output_type_at`](Schedule/output_type_at.md) /
[`stage_info`](Schedule/stage_info.md) /
[`stage_manifest`](Schedule/stage_manifest.md) introspect each stage's
declared types and whether it is a `Module`.

## Methods

| Method | Returns | Covered on |
| --- | --- | --- |
| `append(layer)` / `append(name, layer)` / `append(list)` | `Schedule&` | [`append`](Schedule/append.md) |
| `append_as<In, Out>(layer)` / `append_as<In, Out>(name, layer)` | `Schedule&` | [`append_as`](Schedule/append_as.md) |
| `insert(index, layer)` / `insert(index, name, layer)` | `Schedule&` | [`insert`](Schedule/insert.md) |
| `insert_as<In, Out>(index, layer)` / `insert_as<In, Out>(index, name, layer)` | `Schedule&` | [`insert_as`](Schedule/insert_as.md) |
| `extend(other)` / `extend(container)` | `Schedule&` | [`extend`](Schedule/extend.md) |
| `erase(index)` | `void` | [`erase`](Schedule/erase.md) |
| `forward(input)` / `operator()(input)` | `Series` | [`forward`](Schedule/forward.md) |
| `run<ReturnType>(inputs...)` | `ReturnType` | [`run`](Schedule/run.md) |
| `size()` | `size_t` | [`size`](Schedule/size.md) |
| `empty()` | `bool` | [`empty`](Schedule/empty.md) |
| `reserve(count)` | `void` | [`reserve`](Schedule/reserve.md) |
| `contains(name)` | `bool` | [`contains`](Schedule/contains.md) |
| `index_of(name)` | `size_t` | [`index_of`](Schedule/index_of.md) |
| `name_at(index)` | `const std::string&` | [`name_at`](Schedule/name_at.md) |
| `input_type_at(index)` | `const std::type_info&` | [`input_type_at`](Schedule/input_type_at.md) |
| `output_type_at(index)` | `const std::type_info&` | [`output_type_at`](Schedule/output_type_at.md) |
| `module_at(index)` | `Module*` (or `const Module*`) | [`module_at`](Schedule/module_at.md) |
| `stage_info(index)` / `at(index)` | `StageInfo` | [`stage_info`](Schedule/stage_info.md) |
| `stage_manifest()` | `vector<StageInfo>` | [`stage_manifest`](Schedule/stage_manifest.md) |
| `clear()` | `void` | [`clear`](Schedule/clear.md) |

`at(index)` is a one-line alias for `stage_info(index)`; it is covered on
that same page rather than getting its own. `train`/`eval`/`to`/
`parameters`/`named_parameters`/`buffers`/`named_buffers`/`modules`/
`named_modules`/`snapshot`/`restore` also apply to `Schedule` (recursing
into every `Module`-like stage) -- they are the base [`Module`](Module.md)
contract, documented there, not repeated here.

## Changelog

- **0.1** — the twenty-method schedule contract: heterogeneous-type stage
  composition, insertion/removal, execution, and introspection.

## See Also

- [`Module`](Module.md) -- the base contract `Schedule` (and every stage
  that is a `Module`) implements.
- [`fusion`](../fusion.md), [`cnn`](../cnn.md), [`rnn`](../rnn.md),
  [`attention`](../attention.md) -- the concrete layer types most commonly
  chained into a `Schedule`.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Schedule/Introduction
Schedule/append
Schedule/append_as
Schedule/clear
Schedule/contains
Schedule/empty
Schedule/erase
Schedule/extend
Schedule/forward
Schedule/index_of
Schedule/input_type_at
Schedule/insert
Schedule/insert_as
Schedule/module_at
Schedule/name_at
Schedule/output_type_at
Schedule/reserve
Schedule/run
Schedule/size
Schedule/stage_info
Schedule/stage_manifest
```
<!-- apiref:toctree:end -->
