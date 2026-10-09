# `pluss::ai::procedure::step`

Wraps a plain function or lambda over `Series` or `DataFrame` as a
`Map`-capability `Step` -- the engine-agnostic adapter, with an optional
registry-key overload that makes the step persistable.

## Signature

```cpp
template <typename Fn>
pluss::ai::Step pluss::ai::procedure::step(std::string name, std::string input_name, std::string output_name, Fn fn);

template <typename Fn>
pluss::ai::Step pluss::ai::procedure::step(std::string name, std::string input_name, std::string output_name, Fn fn, std::string key);
```

`Fn` must be callable with a `const pluss::Series&` or a
`const pluss::table::DataFrame&` (or both); it need not return any
particular type -- see the Introduction's "Two Return Shapes" section.

## What It Does

Builds a `Map`-capability [`Step`](../core/Step.md) that calls `fn` with
whichever of its one input's possible types `fn` accepts, and publishes
the result under `output_name`. The first overload builds a step with no
persistence (`StepOptions::component == "callable"`). The second
overload additionally requires `key` to already be
[`bind`](bind.md)-registered, and builds a step whose
[`custom()`](../core/Step/custom.md) is `true` and whose
[`component()`](../core/Step/component.md) is `key` -- reloadable later
by that key alone via [`procedure::load`](../persist/load.md).

## Parameters

`std::string name`
: The step's name.
`std::string input_name`
: The one input port name.
`std::string output_name`
: The one output port name -- also where an unwrapped return value is
  published under.
`Fn fn`
: The callable. Must be non-empty for both overloads to construct
  correctly (thrown-checked only at the generic layer `step` delegates
  to).
`std::string key`
: A registry key already bound via [`bind`](bind.md).

## Returns

A [`Step`](../core/Step.md) value.

## Errors

- Throws if `input_name` or `output_name` is empty.
- Throws (on the keyed overload) if `key` has not been
  [`bind`](bind.md)-registered.
- At run time, throws if the resolved input artifact's type matches
  neither `Series` nor `DataFrame` (whichever `Fn` actually accepts).

## Practical Examples

### Example 1

A `Series` callable -- no persistence.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{1.0, 2.0}, {2}, cfg);

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& s) { return s.mul(10.0); }),
  });

  const ai::ProcedureResult out = proc.run({ai::procedure::input("x", x)});
  std::cout << "elements=" << out.get<pluss::Series>("scaled").numel() << "\n";
  return 0;
}
```

Output

```text
elements=2
```

### Example 2

A `DataFrame` callable -- the same overload, dispatched on the
parameter type `Fn` accepts.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/mic.h>
#include <pluss/pluss.h>

#include <cstdint>
#include <iostream>

int main() {
  namespace ai = pluss::ai;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame df({{"x", Slate(pluss::make_series(std::vector<double>{10.0, 20.0}, {2}, cfg))}});

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("columns", "raw", "count", [](const DataFrame& frame) {
        return static_cast<std::int64_t>(frame.nfeatures());
      }),
  });

  const ai::ProcedureResult out = proc.run({ai::procedure::input("raw", df)});
  std::cout << "count=" << out.get<std::int64_t>("count") << "\n";
  return 0;
}
```

Output

```text
count=1
```

### Example 3

The keyed overload makes the step persistable, once the key is bound.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>
#include <string>

int main() {
  namespace ai = pluss::ai;

  ai::procedure::bind("scale_fn", [](const pluss::Series& v) { return v.mul(10.0); });
  const ai::Step s = ai::procedure::step("scale", "x", "scaled",
      [](const pluss::Series& v) { return v.mul(10.0); }, std::string("scale_fn"));

  std::cout << "persistable=" << s.persistable() << "\n";
  return 0;
}
```

Output

```text
persistable=1
```

## See Also

- [`bind`](bind.md) -- registering the key the second overload needs.
- [`build`](../build.md) -- the generic `step` overloads underneath
  this one.
