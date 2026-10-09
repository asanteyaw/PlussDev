<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 82eabda5e7ba -->
# `pluss::ai::Step::name`

[Back to `pluss::ai::Step`](../Step.md)

## Signature

```cpp
const std::string& name() const noexcept;
```

## What It Does

Returns the step's name, exactly as passed to
[`procedure::step`](../../build/step.md). Names are what
[`Link`](../Link.md)/[`Procedure::get`](../Procedure/get.md)/
[`Procedure::has`](../Procedure/has.md) address steps by, and
[`check`](../Procedure/check.md) rejects a graph with two steps sharing
one.

## Parameters

None.

## Returns

A `const std::string&`.

## Errors

None.

## Practical Examples

### Example 1

A plain callable step's name.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& x) { return x.mul(10.0); });
  std::cout << "name=" << s.name() << "\n";
  return 0;
}
```

Output

```text
name=scale
```

### Example 2

A sink step's name is the step's own name, independent of its target.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("store", ai::procedure::sink("procedure.doc.sink"));
  std::cout << "name=" << s.name() << "\n";
  return 0;
}
```

Output

```text
name=store
```

### Example 3

Any name string works, including one chosen for readability rather than
brevity.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("custom");
  std::cout << "name=" << s.name() << "\n";
  return 0;
}
```

Output

```text
name=custom
```

## See Also

- [`Procedure::get`](../Procedure/get.md) / [`has`](../Procedure/has.md)
  -- looking a step up by this name.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="inputs.html">
    <span class="label">Previous</span>
    <span class="title">inputs</span>
  </a>
  <a class="next-link" href="options.html">
    <span class="label">Next</span>
    <span class="title">options</span>
  </a>
</div>
<!-- apiref:nav:end -->
