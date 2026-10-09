# `pluss::ai::procedure::bind`

Registers a callable under a key so a persisted `step(..., fn, key)` can
be reloaded by that key alone later -- the `callable` family's own sugar
over the lower-level [`registry`](../registry.md) bind forms.

## Signature

```cpp
template <typename Fn>
void pluss::ai::procedure::bind(std::string key, Fn fn);
```

`Fn` must satisfy the same constraint `step`'s callable overloads
require: callable with a `const pluss::Series&` or a
`const pluss::table::DataFrame&`.

## What It Does

Registers a [`registry::CustomContract`](../registry/CustomContract.md)
under `key` whose load hook reconstructs a
[`callable`](step.md)-style step from `fn` -- internally, this `bind`
builds exactly the `CustomContract` the lower-level
[`registry::bind`](../registry/bind.md) forms build by hand, saving a
caller from writing the load hook themselves for the common
single-callable case.

## Parameters

`std::string key`
: The registry key. Must be non-empty and not already bound.
`Fn fn`
: The callable to register.

## Returns

Nothing (`void`).

## Errors

- Throws if `key` is empty.
- Throws if `key` is already bound (by this `bind`, or by any
  [`registry`](../registry.md) bind form).

## Practical Examples

### Example 1

Binding a key, then building a persistable step with it.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>
#include <string>

int main() {
  namespace ai = pluss::ai;

  ai::procedure::bind("doubler_v1", [](const pluss::Series& v) { return v.mul(2.0); });
  const ai::Step s = ai::procedure::step("double", "x", "doubled",
      [](const pluss::Series& v) { return v.mul(2.0); }, std::string("doubler_v1"));

  std::cout << "custom=" << s.custom() << "\n";
  return 0;
}
```

Output

```text
custom=1
```

### Example 2

The bound step's `component()` is the key it was bound under.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>
#include <string>

int main() {
  namespace ai = pluss::ai;

  ai::procedure::bind("tripler_v1", [](const pluss::Series& v) { return v.mul(3.0); });
  const ai::Step s = ai::procedure::step("triple", "x", "tripled",
      [](const pluss::Series& v) { return v.mul(3.0); }, std::string("tripler_v1"));

  std::cout << "component=" << s.component() << "\n";
  return 0;
}
```

Output

```text
component=tripler_v1
```

### Example 3

Binding the same key twice throws.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  ai::procedure::bind("unique_key_demo", [](const pluss::Series& v) { return v.mul(4.0); });
  bool threw = false;
  try {
    ai::procedure::bind("unique_key_demo", [](const pluss::Series& v) { return v.mul(5.0); });
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

- [`step`](step.md) -- the keyed overload this `bind` feeds.
- [`registry`](../registry.md) -- the lower-level mechanism this is
  sugar over.
