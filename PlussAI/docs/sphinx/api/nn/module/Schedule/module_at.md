<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: baabbfe041e0 -->

# `pluss::ai::nn::Schedule::module_at`

[Back to `pluss::ai::nn::Schedule`](../Schedule.md)

Only reachable via `->` (`net->module_at(index)`).

## Signature

```cpp
Module* nn::Schedule::Impl::module_at(std::size_t index);
const Module* nn::Schedule::Impl::module_at(std::size_t index) const;
```

## What It Does

Reaches one specific stage's underlying `Module`, by position, when that
stage's own type-specific accessors are needed rather than the whole
`Schedule`'s recursive view -- for example calling
[`train(false)`](../Module/train.md) on a single stage while leaving the
rest of the chain in training mode. Only works for stages that are
actually `nn::Module`s; a plain lambda or other bare callable stage throws.

## Parameters

`std::size_t index`
: A position in `[0, size())`.

## Returns

`Module*` (or `const Module*`) -- the stage's underlying `Module`, reached
the same way `module_record` does internally. Use this when you need the
one specific stage's own `Module` methods
([`train`](../Module/train.md)/[`parameters`](../Module/parameters.md)/etc.)
rather than the whole `Schedule`'s recursive view.

## Errors

- `index >= size()` -- throws.
- The stage at `index` is not an `nn::Module` (a plain lambda or other
  bare callable) -- throws. See the category
  [Introduction](../Introduction.md)'s "Stages Do Not Have To Be Modules"
  note.

## Practical Examples

Three examples: fetching a layer's `Module*` and using it directly, the
`const` overload, and the error a non-`Module` stage produces.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule net;
  net.append("head", nn::Fusion(3, 2));

  nn::Module* m = net->module_at(0);
  std::cout << "module_at(0)->is_training()=" << std::boolalpha << m->is_training() << "\n";
  std::cout << "module_at(0)->parameters().size()=" << m->parameters().size() << "\n";

  return 0;
}
```

Output

```text
module_at(0)->is_training()=true
module_at(0)->parameters().size()=2
```

### Example 2

The `const` overload, reached through a `const Schedule&`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule net;
  net.append("head", nn::Fusion(3, 2));

  const nn::Schedule& const_net = net;
  const nn::Module* m = const_net->module_at(0);
  std::cout << "const overload: module_at(0)->parameters().size()=" << m->parameters().size() << "\n";

  return 0;
}
```

Output

```text
const overload: module_at(0)->parameters().size()=2
```

### Example 3

A lambda stage has no `Module` to fetch -- `module_at` throws rather than
returning `nullptr`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  auto double_it = [](const pluss::Series& x) { return x.mul(2.0); };

  nn::Schedule net;
  net.append("scale", double_it);  // not an nn::Module -- no module to fetch

  try {
    net->module_at(0);
    std::cout << "(did not throw)\n";
  } catch (const std::exception& e) {
    std::cout << "caught: " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
caught: pluss::nn::Schedule::module_at: stage 'scale' is not an nn::Module
```

## See Also

- [`stage_info`](stage_info.md) -- reports whether a stage is a `Module`
  (`module_stage`) without throwing.
- [`Module`](../Module.md) -- the type `module_at` returns a pointer to.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="insert_as.html">
    <span class="label">Previous</span>
    <span class="title">insert_as</span>
  </a>
  <a class="next-link" href="name_at.html">
    <span class="label">Next</span>
    <span class="title">name_at</span>
  </a>
</div>
<!-- apiref:nav:end -->
