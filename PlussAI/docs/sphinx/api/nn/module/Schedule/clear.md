<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c12f8c9b75a8 -->

# `pluss::ai::nn::Schedule::clear`

[Back to `pluss::ai::nn::Schedule`](../Schedule.md)

`ScheduleImpl` defines its own `clear()`, which **hides** the inherited
[`Module::clear`](../Module.md) entirely -- ordinary C++ name hiding,
not an override. Calling `.clear()` on a `Schedule` value (forwarded through
`ModuleHolder`, reachable via `.`, same as `->`) always reaches this
destructive version, never `Module::clear()`'s sensitivity-only behavior.
Read this page even if you already know what `Module::clear()` does
elsewhere in this reference -- they are unrelated operations that happen to
share a name.

## Signature

```cpp
void nn::Schedule::clear();
```

## What It Does

Removes every stage from the chain, rebuilds the (now-empty)
module-registration tree so `parameters()`/`buffers()`/`modules()` reflect
the removal immediately, and resets the auto-naming counter
[`append`](append.md) uses back to `0`. After `clear()`, the `Schedule` is
indistinguishable from a freshly default-constructed one -- reach for it to
reuse a `Schedule` value for a different chain rather than constructing a
new one.

## Parameters

None.

## Returns

`void`. Removes every stage, rebuilds the (now-empty) module-registration
tree, and resets the auto-naming counter back to `0`.

## Errors

None.

## Practical Examples

Three examples: a basic clear, a direct side-by-side against
`Module::clear()` on a plain layer, and confirming the auto-name counter
resets too.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule net;
  net.append("in", nn::Fusion(3, 4));
  net.append("out", nn::Fusion(4, 2));
  std::cout << "before: size()=" << net->size() << "\n";

  net.clear();
  std::cout << "after clear(): size()=" << net->size() << " empty()=" << std::boolalpha << net->empty() << "\n";

  return 0;
}
```

Output

```text
before: size()=2
after clear(): size()=0 empty()=true
```

### Example 2

`Fusion` (a plain `Module`) calling `clear()` only clears recorded gradient
sensitivity -- its parameters are untouched. `Schedule` calling `clear()`
removes every stage. Same method name, unrelated behavior.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  // Module::clear() (documented on the base Module page) only clears each
  // parameter's recorded gradient sensitivity -- it never removes anything.
  nn::Fusion layer(3, 2);
  layer.clear();
  std::cout << "Fusion (a plain Module): clear() leaves parameters().size()=" << layer.parameters().size() << "\n";

  // ScheduleImpl defines its OWN clear() with the same name, which hides
  // the inherited Module::clear() entirely (ordinary C++ name hiding --
  // there is no `using Module::clear;` bringing the base version back).
  // Schedule::clear() is destructive: it erases every stage.
  nn::Schedule net;
  net.append("in", nn::Fusion(3, 4));
  net.append("out", nn::Fusion(4, 2));
  net.clear();
  std::cout << "Schedule: clear() leaves size()=" << net->size()
            << " (every stage is gone, not just their gradient state)\n";

  return 0;
}
```

Output

```text
Fusion (a plain Module): clear() leaves parameters().size()=2
Schedule: clear() leaves size()=0 (every stage is gone, not just their gradient state)
```

### Example 3

`clear()` also resets the auto-naming counter -- the next unnamed stage
after a `clear()` starts back at `"0"`, not wherever the counter had
reached.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule net;
  net.append(nn::Fusion(3, 4));   // auto-named "0"
  net.append(nn::Fusion(4, 2));   // auto-named "1"

  net.clear();
  net.append(nn::Fusion(5, 5));   // clear() resets the auto-name counter too

  std::cout << "after clear() + one more append(): stage 0 name='" << net->name_at(0) << "'\n";

  return 0;
}
```

Output

```text
after clear() + one more append(): stage 0 name='0'
```

## See Also

- [`Module::clear`](../Module.md) -- the unrelated base-class method
  this one hides.
- [`erase`](erase.md) -- removes one stage rather than every stage.
- [`empty`](empty.md) -- `true` immediately after `clear()`.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="append_as.html">
    <span class="label">Previous</span>
    <span class="title">append_as</span>
  </a>
  <a class="next-link" href="contains.html">
    <span class="label">Next</span>
    <span class="title">contains</span>
  </a>
</div>
<!-- apiref:nav:end -->
