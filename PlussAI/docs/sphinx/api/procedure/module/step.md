# `pluss::ai::procedure::step`

The raw-module adapter overload: wraps any object satisfying the
`SeriesForwardModule` concept (a `.forward(const Series&) -> Series`
method, e.g. an `nn::Module` layer) as a `Map`+`Infer` `Step` with no
persistence of its own -- the least-guided of the four adapter
families.

## Signature

```cpp
template <typename Module>
pluss::ai::Step pluss::ai::procedure::step(std::string name, std::string input_name, std::string output_name, Module module);
```

`Module` must satisfy `requires(Module& m, const Series& x) { { m.forward(x) } -> std::convertible_to<Series>; }`.

## What It Does

Builds a `Map`+`Infer`-capability [`Step`](../core/Step.md) whose runner
calls `module.forward(x)` on a `Series` input and publishes the result.
`RunOptions::mode` is ignored -- `forward` has only one behavior.
Nothing is ever persisted: this overload attaches no
[`StepPersistSpec`](../core/StepPersistSpec.md), regardless of what
`Module` is.

## Parameters

`std::string name`, `std::string input_name`, `std::string output_name`
: The step's name and its one input/output port. Must both be
  non-empty.
`Module module`
: Any object satisfying `SeriesForwardModule`, moved into the step.

## Returns

A [`Step`](../core/Step.md) value.

## Errors

- Throws if `input_name` or `output_name` is empty.
- At run time, throws if the resolved input artifact is not a `Series`.

## Practical Examples

### Example 1

A real `nn` layer, wrapped directly.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/nn.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  namespace ai = pluss::ai;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<float>{-2.0F, 0.0F, 3.0F}, {3, 1}, cfg);
  pluss::nn::ReLU relu{};

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("relu", "x", "positive", relu),
  });
  const ai::ProcedureResult out = proc.run({ai::procedure::input("x", x)});
  std::cout << "elements=" << out.get<pluss::Series>("positive").numel() << "\n";
  return 0;
}
```

Output

```text
elements=3
```

### Example 2

A plain, caller-defined struct satisfies the same concept -- nothing
beyond a matching `forward` method is required.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

struct SquareModule {
  pluss::Series forward(const pluss::Series& x) { return x.mul(x); }
};

int main() {
  namespace ai = pluss::ai;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{2.0, 3.0, 4.0}, {3}, cfg);

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("square", "x", "x2", SquareModule{}),
  });
  const ai::ProcedureResult out = proc.run({ai::procedure::input("x", x)});
  std::cout << "elements=" << out.get<pluss::Series>("x2").numel() << "\n";
  return 0;
}
```

Output

```text
elements=3
```

### Example 3

The resulting step is never persistable, regardless of what it wraps.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

struct SquareModule {
  pluss::Series forward(const pluss::Series& x) { return x.mul(x); }
};

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("square", "x", "x2", SquareModule{});
  std::cout << "persistable=" << s.persistable() << "\n";
  return 0;
}
```

Output

```text
persistable=0
```

## See Also

- [`ice`](../ice/step.md) -- the guided, persistable path for full
  `ice::Model`s.
- [`registry`](../registry.md) -- adding persistence this overload
  cannot provide on its own.
