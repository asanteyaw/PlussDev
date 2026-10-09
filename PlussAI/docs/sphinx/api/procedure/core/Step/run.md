<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c6149ce4feec -->

# `pluss::ai::Step::run`

[Back to `pluss::ai::Step`](../Step.md)

## Signature

```cpp
Artifact run(const std::vector<Artifact>& inputs, const RunOptions& options) const;
```

## What It Does

Executes this one step directly: if a runner closure is present, calls
it with `inputs` and `options` and returns its result. Otherwise falls
back to one of two default behaviors -- a [`Sink`](../Introduction.md)-capability
step with no runner (every [`SinkSpec`](../SinkSpec.md)-backed step)
passes its one input through unchanged under the step's own name; any
other no-runner step with exactly one input does the same; a no-runner
step with zero or more-than-one inputs produces an empty, named
`Artifact` instead. [`Procedure::run`](../Procedure/run.md) calls this
once per step in topological order -- most callers never call
`Step::run` directly, since [`Procedure::run`](../Procedure/run.md)
already handles resolving each step's declared inputs from the artifact
pool first.

## Parameters

`const std::vector<Artifact>& inputs`
: The artifacts to run the step against, already resolved to the step's
  own declared input order.
`const RunOptions& options`
: The same per-call policy [`Procedure::run`](../Procedure/run.md)
  received.

## Returns

One [`Artifact`](../Artifact.md) -- the step's result.

## Errors

- Propagates whatever the runner closure itself throws.

## Practical Examples

### Example 1

A plain callable step's runner multiplies its input `Series`.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{1.0, 2.0}, {2}, cfg);

  const ai::Step s = ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& v) { return v.mul(10.0); });
  const ai::Artifact out = s.run({ai::procedure::input("x", x)}, ai::RunOptions{});
  std::cout << "elements=" << out.get<pluss::Series>().numel() << "\n";
  return 0;
}
```

Output

```text
elements=2
```

### Example 2

A sink step with no runner of its own passes its one input through,
publishing it under the sink step's own name.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{3.0, 4.0}, {2}, cfg);

  const ai::Step s = ai::procedure::step("store", ai::procedure::sink("procedure.doc.sink"));
  const ai::Artifact out = s.run({ai::procedure::input("in", x)}, ai::RunOptions{});
  std::cout << "name=" << out.name() << "\n";
  return 0;
}
```

Output

```text
name=store
```

### Example 3

A bare `step(name)` with no runner and no `Sink` capability still falls
back to the same one-input pass-through behavior.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{5.0}, {1}, cfg);

  const ai::Step s = ai::procedure::step("passthrough");
  const ai::Artifact out = s.run({ai::procedure::input("in", x)}, ai::RunOptions{});
  std::cout << "defined=" << !out.empty() << "\n";
  return 0;
}
```

Output

```text
defined=1
```

## See Also

- [`Procedure::run`](../Procedure/run.md) -- the usual caller, which
  resolves `inputs` for every step automatically.
- [`runnable`](runnable.md) -- whether a runner closure is present at
  all.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="reloadable.html">
    <span class="label">Previous</span>
    <span class="title">reloadable</span>
  </a>
  <a class="next-link" href="runnable.html">
    <span class="label">Next</span>
    <span class="title">runnable</span>
  </a>
</div>
<!-- apiref:nav:end -->
