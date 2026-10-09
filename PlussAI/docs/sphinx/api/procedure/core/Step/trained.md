<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 67568bee960f -->
# `pluss::ai::Step::trained`

[Back to `pluss::ai::Step`](../Step.md)

## Signature

```cpp
bool trained() const noexcept;
```

## What It Does

Returns `options().trained` -- whether the component this step wraps
already has learned state. Built-in model adapters
([`lymph`](../../lymph.md)/[`ice`](../../ice.md)/[`nlp`](../../nlp.md))
set this from the wrapped model/workflow's own `trained()` at the moment
the step is constructed; it does not update afterward even if
[`run`](run.md) later calls the component's `learn` in
[`ProcedureMode::Learn`](../Introduction.md).

## Parameters

None.

## Returns

`true` or `false`.

## Errors

None.

## Practical Examples

### Example 1

A plain callable step is never "trained" -- the concept does not apply
to it.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& x) { return x.mul(10.0); });
  std::cout << "trained=" << s.trained() << "\n";
  return 0;
}
```

Output

```text
trained=0
```

### Example 2

`StepOptions::trained` can be set directly for a hand-built step.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  ai::StepOptions options{};
  options.trained = true;
  const ai::Step s = ai::procedure::step("marked_trained", options);

  std::cout << "trained=" << s.trained() << "\n";
  return 0;
}
```

Output

```text
trained=1
```

### Example 3

A sink step is not a model, so it is never trained.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("store", ai::procedure::sink("procedure.doc.sink"));
  std::cout << "trained=" << s.trained() << "\n";
  return 0;
}
```

Output

```text
trained=0
```

## See Also

- [`lymph`](../../lymph.md) / [`ice`](../../ice.md) / [`nlp`](../../nlp.md)
  -- the adapter families that set this from a real model's own state.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="save_component.html">
    <span class="label">Previous</span>
    <span class="title">save_component</span>
  </a>
</div>
<!-- apiref:nav:end -->
