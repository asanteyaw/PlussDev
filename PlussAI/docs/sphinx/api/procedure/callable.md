# `pluss::ai::procedure` (callable)

`callable` owns the simplest, engine-agnostic way to put real logic into
a step: a plain function or lambda over `Series` or `DataFrame`,
wrapped by [`step`](callable/step.md) into a `Map`-capability
[`Step`](core/Step.md), plus [`bind`](callable/bind.md), its own sugar
for making such a step reloadable.

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`step`](callable/step.md) | wraps `Fn` as a `Step`, with or without a registry key for persistence |
| [`bind`](callable/bind.md) | registers `Fn` under a key, for the keyed `step` overload |

## How To Call It

Build a procedure around one plain callable step, then run it with a named input. The program prints the run status and the sum of the scaled values, which is 10 times 1 + 2 + 3 = 60.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/procedure.h>

#include <iostream>
#include <vector>

int main() {
  namespace ai = pluss::ai;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3}, cfg);

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& s) { return s.mul(10.0); }),
  });

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

A callable step never references `lymph::`/`ice::`/`nlp::` types and its
[`RunOptions::mode`](core/RunOptions.md) is simply ignored -- it does the
same thing in `Infer`, `Learn`, and `Assess` modes alike. Use it for any
single-input, single-output transformation that does not need to be a
full model: a scaling factor, a column count, a feature derived with
ordinary `Series`/`DataFrame` arithmetic.

## Dispatch And Return Values

The callable's parameter type determines which artifact alternative the step
accepts. A function accepting `const Series&` receives only a `Series`; one
accepting `const DataFrame&` receives only a `DataFrame`. There is no implicit
conversion between the two. A generic callable that is valid for both is tried
as a Series callable first when the input actually holds a Series, then as a
DataFrame callable for a DataFrame input.

Returning a supported value such as `Series`, `std::int64_t`, or `bool` wraps
that value under the declared output name. Returning `Artifact` gives the
callable control over the output name: an unnamed artifact acquires the
declared name, while an explicitly named artifact keeps its own name. Prefer a
plain value unless dynamic naming is truly part of the contract, because fixed
names are easier for links and readers to reason about.

## Ownership And Persistence

The callable is moved into the step's runner closure. Captured state therefore
lives as long as the step and is shared according to the callable's own capture
types. The unkeyed overload cannot be saved because an arbitrary C++ closure
has no portable reconstruction recipe. The keyed overload separates code from
state: `bind(key, fn)` registers the code recipe in the process, while the
saved bundle records the key and graph metadata. The same key must be bound
before a later process calls `load()`.

Registration is process-global and duplicate keys are rejected. Use stable,
versioned keys for deployed bundles; changing the behavior behind an existing
key changes what old bundles execute even if their manifest remains valid.

## Failure Boundary

Input/output port names and registry availability are checked while building
the step. Artifact type compatibility is checked at run time. Exceptions from
the callable propagate through the normal procedure failure policy: they throw
under `fail_fast=true` and become a failed result plus warning under
`fail_fast=false`.

## Related Pages

- [`build`](build.md) -- the generic `step` overloads this family
  layers on top of.
- [`registry`](registry.md) -- the lower-level mechanism
  [`bind`](callable/bind.md) is sugar over.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

callable/Introduction
callable/step
callable/bind
```
<!-- apiref:toctree:end -->
