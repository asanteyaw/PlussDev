# `pluss::ai::procedure` (registry)

`registry` owns the mechanism behind
[`StepPersistSpec::custom`](core/StepPersistSpec.md): a module-level map
from key to reload recipe, populated by [`bind`](registry/bind.md)/
[`bind_ice`](registry/bind_ice.md) and consulted by
[`step`](registry/step.md)'s `CustomPersist` overload and by
[`procedure::load`](persist/load.md).

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`CustomContract`](registry/CustomContract.md) | the full registry record: key, schema versions, a `StepLoader`, an optional `CompatibilityGate` |
| [`CustomPersist`](registry/CustomPersist.md) | the lighter request `step` itself consumes, keyed into an already-bound contract |
| [`bind`](registry/bind.md) | registers a `CustomContract` directly, or from a bare `StepLoader` |
| [`bind_ice`](registry/bind_ice.md) | registers an `ice::Model` factory by key, for `ice`'s owning `step` overload |
| [`step`](registry/step.md) | the `CustomPersist`-backed `Step` builder |

## How To Call It

Bind a callable under a registry key, then build a step that persists under that key. The key is stored with the step, so a saved procedure can be reloaded by name.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/procedure.h>
#include <pluss/mic.h>

#include <iostream>
#include <vector>

int main() {
  namespace ai = pluss::ai;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3}, cfg);

  auto scaler = [](const pluss::Series& v) { return v.mul(10.0); };
  ai::procedure::bind("my_scaler_v1", scaler);

  const ai::Step s = ai::procedure::step("scale", "x", "scaled", scaler, std::string("my_scaler_v1"));
  const ai::Procedure proc = ai::procedure::make({s});

  const ai::ProcedureResult out = proc.run({ai::procedure::input("x", x)});
  std::cout << "ok = " << (out.ok() ? "true" : "false") << "\n";
  std::cout << "scaled sum = " << out.get<pluss::Series>("scaled").sum().item<double>() << "\n";
  return 0;
}
```

Output

```text
ok = true
scaled sum = 60
```

This is `callable`'s own sugar (see [`callable::bind`](callable/bind.md))
built directly on `registry::bind`; reach for `registry`'s own
[`CustomContract`](registry/CustomContract.md)/[`step`](registry/step.md)
only when a caller-defined component needs a reload recipe more involved
than one callable.

## Registration Lifecycle

The registry is process-local. Binding a key does not modify a saved bundle;
it equips the current process to interpret that key when constructing a keyed
step or loading a bundle. A service that restores custom procedures should
bind every supported contract during deterministic startup, before accepting
bundle input. Binding the same key twice is rejected rather than silently
replacing executable behavior.

Keys are durable API identifiers. Include a component and contract version in
the key when incompatible behavior needs to coexist. Schema integers then
describe compatible evolution within that identity: the persistence schema
versions the procedure-side record, and the component schema versions the
wrapped component's data.

## Load-Time Validation

Loading a custom step is not merely a key lookup. The registered contract may
run a compatibility gate before its loader, and the reconstructed step is
checked against the manifest for component label, persistence key, both schema
versions, and reloadable, frozen, and custom flags. A loader cannot quietly
return a different step contract under a familiar key.

The registry is not a code-serialization mechanism. Bundles contain component
state and identifiers, not arbitrary executable closures. Deploying a bundle
therefore requires deploying the binary that binds its known contracts.

## Concurrency And Ownership

Registered callables, loaders, factories, and gates are stored by value in
module-level maps. Registration is intended as startup configuration, not a
concurrent hot-path operation. Once registration is complete, step creation
and loading read those recipes to build independently owned step state.

## Related Pages

- [`callable`](callable.md) -- the common case this mechanism serves.
- [`persist`](persist.md) -- where a bound contract is consulted, during
  `load`.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

registry/Introduction
registry/CustomContract
registry/CustomPersist
registry/bind
registry/bind_ice
registry/step
```
<!-- apiref:toctree:end -->
