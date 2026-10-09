# `pluss::ai::procedure::step`

The registry-backed `Step` builder: `step(name, options, CustomPersist,
runner)` looks up `CustomPersist::key` in the contract registry (bound
via [`bind`](bind.md)) and fails fast if nothing is bound for it.

## Signature

```cpp
pluss::ai::Step pluss::ai::procedure::step(std::string name, StepOptions options, CustomPersist persist, Step::Runner runner);
```

## What It Does

Looks `persist.key` up in the registry [`bind`](bind.md) populates,
throwing immediately if nothing is bound for it. On success, builds a
[`Step`](../core/Step.md) whose [`custom()`](../core/Step/custom.md) is
`true`, whose [`component()`](../core/Step/component.md) is
`persist.key`, and whose [`StepPersistSpec`](../core/StepPersistSpec.md)
is built from the bound `CustomContract`'s own schema versions plus
`persist`'s own `reloadable`/`frozen`/`fields`/`save`. This is the
lower-level call [`callable`](../callable.md)'s own keyed `step`
overload builds on top of.

## Parameters

`std::string name`, `StepOptions options`
: The step's name and declared contract.
`CustomPersist persist`
: The registry key (already bound) plus the step's own persistence
  flags.
`Step::Runner runner`
: The runner closure.

## Returns

A [`Step`](../core/Step.md) value.

## Errors

- Throws if `persist.key` has no bound `CustomContract`.
- Throws if `name` is empty, or `runner` is not callable (the same
  checks every generic `step` overload makes).

## Practical Examples

### Example 1

A key bound via [`bind`](bind.md), then used directly through this
overload.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  ai::procedure::bind("registry_square_v1", [](const pluss::Series& v) { return v.mul(v); });

  ai::StepOptions options{};
  options.capabilities = {ai::StepCapability::Map};
  options.inputs = {"x"};
  options.outputs = {"y"};
  ai::procedure::CustomPersist persist{};
  persist.key = "registry_square_v1";

  const ai::Step s = ai::procedure::step("square", options, persist,
      [](const std::vector<ai::Artifact>& inputs, const ai::RunOptions&) -> ai::Artifact {
        const pluss::Series& v = inputs.front().get<pluss::Series>();
        return ai::Artifact("y", ai::Artifact::Value(v.mul(v)));
      });

  std::cout << "persistable=" << s.persistable() << "\n";
  return 0;
}
```

Output

```text
persistable=1
```

### Example 2

The resulting step reports `custom() == true`.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  ai::procedure::bind("registry_custom_flag_v1", [](const pluss::Series& v) { return v.mul(2.0); });

  ai::procedure::CustomPersist persist{};
  persist.key = "registry_custom_flag_v1";

  const ai::Step s = ai::procedure::step("double", ai::StepOptions{}, persist,
      [](const std::vector<ai::Artifact>&, const ai::RunOptions&) -> ai::Artifact { return ai::Artifact("out"); });

  std::cout << "custom=" << s.custom() << "\n";
  return 0;
}
```

Output

```text
custom=1
```

### Example 3

An unbound key throws immediately.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  ai::procedure::CustomPersist persist{};
  persist.key = "never_bound_key";

  bool threw = false;
  try {
    ai::procedure::step("broken", ai::StepOptions{}, persist,
        [](const std::vector<ai::Artifact>&, const ai::RunOptions&) -> ai::Artifact { return ai::Artifact("out"); });
  } catch (const std::exception&) {
    threw = true;
  }
  std::cout << "threw=" << threw << "\n";
  return 0;
}
```

Output

```text
threw=1
```

## See Also

- [`bind`](bind.md) -- registering the key this overload requires.
- [`callable`](../callable/step.md) -- the higher-level sugar built on
  this.
