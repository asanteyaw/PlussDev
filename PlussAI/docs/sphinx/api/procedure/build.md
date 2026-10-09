# `pluss::ai::procedure` (build)

`build` owns the free functions every [`core`](core.md) type is actually
constructed through in practice: the four generic
[`step`](build/step.md) overloads, [`sink`](build/sink.md),
[`link`](build/link.md), [`input`](build/input.md), and
[`make`](build/make.md). Every engine-specific adapter family
([`callable`](callable.md)/[`lymph`](lymph.md)/[`ice`](ice.md)/
[`nlp`](nlp.md)/[`module`](module.md)/[`registry`](registry.md)) adds its
own overload of `step` on top of these; `build` is the layer that works
regardless of which engine, or none at all, a step wraps.

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`step`](build/step.md) | the four generic `Step` builders: bare, with a runner, with a runner plus `StepPersistSpec`, and from a `SinkSpec` |
| [`sink`](build/sink.md) | builds a `SinkSpec` from a target name |
| [`link`](build/link.md) | builds a `Link` between two step names, with optional named ports |
| [`input`](build/input.md) | wraps any value as a named `Artifact`, for seeding `run()` |
| [`make`](build/make.md) | builds a `Procedure` from a step list and link list |

## How To Call It

Build the graph once, then run it with a named input. The program prints the run status and every artifact the run produced, including the seeded input and the sink output.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/procedure.h>

#include <cstdint>
#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("admit", ai::StepOptions{.capabilities = {ai::StepCapability::Admit}, .outputs = {"raw"}}),
      ai::procedure::step("store", ai::procedure::sink("risk_forecast.out")),
  }, {
      ai::procedure::link("admit", "store", "raw", "input"),
  });

  const ai::ProcedureResult out = proc.run({ai::procedure::input("x", std::int64_t{1})});
  std::cout << "ok = " << (out.ok() ? "true" : "false") << "\n";
  std::cout << "artifacts = " << out.artifacts().size() << "\n";
  for (const ai::Artifact& artifact : out.artifacts()) {
    std::cout << "  " << artifact.name() << "\n";
  }
  return 0;
}
```

Output

```text
ok = true
artifacts = 3
  x
  raw
  store
```

Every `procedure` program ends in a call to [`make`](build/make.md); every
step in its list comes from some overload of [`step`](build/step.md), and
every link from [`link`](build/link.md). `build` is the only family whose
functions never reference an engine type at all -- a graph built entirely
from `build`'s own functions and plain callables (see
[`callable`](callable.md)) needs no `lymph`/`ice`/`nlp` include at all.

## Construction Pipeline

Build a graph in four layers:

1. Name the external values with `input()` at run time.
2. Build each node with the `step()` overload matching its behavior.
3. Connect dependencies with `link()`, naming ports when either endpoint is
   ambiguous.
4. Call `make()` once to freeze and validate the topology.

`input()` does not modify the graph and `make()` does not capture run data.
That separation lets one procedure be invoked repeatedly with different
artifacts and modes.

## Generic Step Forms

| Form | Use |
| --- | --- |
| `step(name, options)` | Placeholder, pass-through, or sink-like fallback behavior with no custom runner |
| `step(name, options, runner)` | Fully custom runner over a vector of artifacts |
| `step(name, options, persist, runner)` | Custom runner with an explicit low-level persistence contract |
| `step(name, sink(target))` | Terminal publication marker that passes through its input |

The engine adapter overloads are preferable when available because they fill
capabilities, ports, component identity, persistence metadata, and runner
dispatch consistently. Use the generic forms for graph behavior that genuinely
does not belong to an existing adapter, especially joins consuming several
artifacts.

## Validation Timing

Builders reject local errors immediately: empty names, empty sink targets,
missing runners, and malformed persistence requests. `make()` then validates
whole-graph properties such as duplicate names, link endpoints and ports,
cycles, capabilities, and worker count. Artifact availability and type remain
run-time concerns because they depend on the inputs and mode of a particular
invocation.

## Value And Allocation Behavior

Builder arguments are accepted by value and moved into the resulting graph.
Initializer-list forms are convenient for small static graphs; vector forms
avoid rebuilding lists when a program assembles steps dynamically. Moving a
`Series` into an input artifact preserves its handle semantics and platform;
the build layer does not copy elements, cast dtypes, or transfer devices.

## Related Pages

- [`core`](core.md) -- the types these functions build.
- [`callable`](callable.md) -- the simplest engine-agnostic `step`
  overload, layered directly on top of `build`'s own generic forms.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

build/Introduction
build/step
build/sink
build/link
build/input
build/make
```
<!-- apiref:toctree:end -->
