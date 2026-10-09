# `pluss::ai::procedure::bind`

The two low-level registry binders: `bind(CustomContract)` registers a
full contract directly, and `bind(key, StepLoader)` is the shorthand
that wraps a bare loader into a minimal `CustomContract`.

## Signature

```cpp
void pluss::ai::procedure::bind(CustomContract contract);
void pluss::ai::procedure::bind(std::string key, StepLoader loader);
```

## What It Does

Registers a [`CustomContract`](CustomContract.md) so that a step whose
[`persist_key`](../core/Step/persist_key.md) matches
`contract.key`/`key` can be reconstructed by
[`procedure::load`](../persist/load.md), or built directly via
[`step`](step.md)'s `CustomPersist` overload. The second overload is
exactly `bind(CustomContract{.key = key, .load = loader})` -- a
shorthand for the common case of registering just a loader with no
custom schema versions or compatibility gate.

## Parameters

`CustomContract contract`
: The full record to register. `contract.key` must be non-empty and
  `contract.load` must be callable.
`std::string key`
: The key to register under. Must be non-empty.
`StepLoader loader`
: The loader to wrap into a minimal contract. Must be callable.

## Returns

Nothing (`void`).

## Errors

- Throws if the key is empty.
- Throws if the loader/contract's `load` is not callable.
- Throws if the key is already bound (by either overload, or by
  [`callable::bind`](../callable/bind.md)).

## Practical Examples

### Example 1

Registering a full `CustomContract` by hand.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace ai = pluss::ai;

  ai::procedure::CustomContract contract{};
  contract.key = "my_scaler_v1";
  contract.load = [](std::string name, ai::StepOptions options, std::string_view,
                      const std::vector<std::pair<std::string, std::string>>&) -> ai::Step {
    return ai::procedure::step(std::move(name), options,
        [](const std::vector<ai::Artifact>& inputs, const ai::RunOptions&) -> ai::Artifact {
          return ai::Artifact("out", ai::Artifact::Value(inputs.front().get<pluss::Series>().mul(10.0)));
        });
  };
  ai::procedure::bind(contract);

  std::cout << "bound=1\n";
  return 0;
}
```

Output

```text
bound=1
```

### Example 2

The shorthand overload registers the same kind of contract from a bare
loader.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace ai = pluss::ai;

  ai::procedure::bind("my_loader_v1", [](std::string name, ai::StepOptions options, std::string_view,
                                         const std::vector<std::pair<std::string, std::string>>&) -> ai::Step {
    return ai::procedure::step(std::move(name), options);
  });

  std::cout << "bound=1\n";
  return 0;
}
```

Output

```text
bound=1
```

### Example 3

Binding the same key a second time throws.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace ai = pluss::ai;

  ai::procedure::bind("unique_registry_key", [](std::string name, ai::StepOptions options, std::string_view,
                                                 const std::vector<std::pair<std::string, std::string>>&) -> ai::Step {
    return ai::procedure::step(std::move(name), options);
  });
  bool threw = false;
  try {
    ai::procedure::bind("unique_registry_key", [](std::string name, ai::StepOptions options, std::string_view,
                                                   const std::vector<std::pair<std::string, std::string>>&) -> ai::Step {
      return ai::procedure::step(std::move(name), options);
    });
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

- [`CustomContract`](CustomContract.md) -- the record the first overload
  registers directly.
- [`step`](step.md) -- the `CustomPersist` overload that depends on a
  bound key.
- [`callable::bind`](../callable/bind.md) -- the single-callable sugar
  built on this.
