# `pluss::ai::procedure::bind_ice`

Registers a zero-argument factory that builds a fresh `ice::Model`
subclass instance by key, so a persisted `ice` step's `restore_key` can
reconstruct the right architecture before loading its weights.

## Signature

```cpp
template <typename Factory>
void pluss::ai::procedure::bind_ice(std::string key, Factory factory);
```

`Factory` must be callable with no arguments, returning (or convertible
to) something derived from `ice::Model`.

## What It Does

Registers `factory` under `key` in a registry separate from
[`bind`](bind.md)'s own -- [`ice`](../ice.md)'s owning `step` overload's
`restore_key` is looked up here, not in the `CustomContract` registry,
since reconstructing an `ice::Model` subclass only ever needs "build a
fresh instance of the right architecture," with weight-loading handled
uniformly afterward by the model's own `load`.

## Parameters

`std::string key`
: The registry key. Must be non-empty and not already bound.
`Factory factory`
: A zero-argument callable returning an `ice::Model` subclass.

## Returns

Nothing (`void`).

## Errors

- Throws if `key` is empty.
- Throws if `key` is already bound.

## Practical Examples

### Example 1

Binding a factory, then building a persistable owning `ice` step with
it.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/nn.h>
#include <pluss/pluss.h>

#include <iostream>
#include <string>

int main() {
  namespace ai = pluss::ai;
  namespace ice = pluss::ai::ice;

  ai::procedure::bind_ice("doc_outline_v1", []() { return ice::Outline(pluss::nn::Fusion(1, 1)); });
  const ai::Step s = ai::procedure::step("learn", "train", "done",
      ice::Outline(pluss::nn::Fusion(1, 1)), std::string("doc_outline_v1"));

  std::cout << "persistable=" << s.persistable() << "\n";
  return 0;
}
```

Output

```text
persistable=1
```

### Example 2

Binding the same key twice throws.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/nn.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;
  namespace ice = pluss::ai::ice;

  ai::procedure::bind_ice("unique_ice_key", []() { return ice::Outline(pluss::nn::Fusion(1, 1)); });
  bool threw = false;
  try {
    ai::procedure::bind_ice("unique_ice_key", []() { return ice::Outline(pluss::nn::Fusion(1, 1)); });
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

### Example 3

An unbound key leaves the owning `step` overload usable, but
unpersistable when `restore_key` itself is left empty.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/nn.h>
#include <pluss/pluss.h>

#include <iostream>
#include <string>

int main() {
  namespace ai = pluss::ai;
  namespace ice = pluss::ai::ice;

  const ai::Step s = ai::procedure::step("learn", "train", "done",
      ice::Outline(pluss::nn::Fusion(1, 1)), std::string(""));

  std::cout << "persistable=" << s.persistable() << "\n";
  return 0;
}
```

Output

```text
persistable=0
```

## See Also

- [`ice`](../ice/step.md) -- the owning overload this feeds.
- [`bind`](bind.md) -- the separate registry for `CustomPersist` steps.
