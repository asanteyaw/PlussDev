<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 32c1ca87cc67 -->
# `pluss::ai::Step::runnable`

[Back to `pluss::ai::Step`](../Step.md)

## Signature

```cpp
bool runnable() const noexcept;
```

## What It Does

Reports whether the step holds a runner closure at all. A step built
without one (the bare `step(name)`/`step(name, options)` overloads, and
the [`SinkSpec`](../SinkSpec.md)-backed `step(name, SinkSpec)` overload)
still [`run`](run.md)s successfully -- [`run`](run.md)'s own fallback
logic handles the no-runner case -- but `runnable()` distinguishes "has
real behavior of its own" from "falls back to the default pass-through."

## Parameters

None.

## Returns

`true` if a runner closure is present, `false` otherwise.

## Errors

None.

## Practical Examples

### Example 1

A plain callable step always has a runner.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& x) { return x.mul(10.0); });
  std::cout << "runnable=" << s.runnable() << "\n";
  return 0;
}
```

Output

```text
runnable=1
```

### Example 2

A sink step has no runner closure of its own -- its behavior (pass the
one input through under the step's own name) comes entirely from
[`run`](run.md)'s `Sink`-capability fallback.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("store", ai::procedure::sink("procedure.doc.sink"));
  std::cout << "runnable=" << s.runnable() << "\n";
  return 0;
}
```

Output

```text
runnable=0
```

### Example 3

Every built-in engine adapter -- here [`lymph`](../../lymph.md)'s
`Policy` wrapper -- supplies a real runner, so it is runnable.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;
  namespace lymph = pluss::ai::lymph;

  const ai::Step s = ai::procedure::step("policy", "raw", "clean", lymph::workflow::Policy{.drop_columns = {"x"}});
  std::cout << "runnable=" << s.runnable() << "\n";
  return 0;
}
```

Output

```text
runnable=1
```

## See Also

- [`run`](run.md) -- what happens with and without a runner.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="run.html">
    <span class="label">Previous</span>
    <span class="title">run</span>
  </a>
  <a class="next-link" href="save_component.html">
    <span class="label">Next</span>
    <span class="title">save_component</span>
  </a>
</div>
<!-- apiref:nav:end -->
