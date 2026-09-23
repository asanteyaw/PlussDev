<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 329455e8f2da -->

# `pluss::ai::nn::Module::modules`

[Back to `pluss::ai::nn::Module`](../Module.md)

## Signature

```cpp
std::vector<Module*> nn::Module::modules();
std::vector<const Module*> nn::Module::modules() const;
```

## Parameters

None.

## Returns

A flat `vector<Module*>` (or `const Module*`) listing **this module itself,
first**, followed by every module registered as a child, recursively
(depth-first). This is the one recursive accessor on this page that includes
`this` in its own result -- `parameters()`/`buffers()` collect only the leaf
`Series`, never a `Module*`. The `const` overload returns `const Module*`.

## Errors

None.

## Practical Examples

Three examples: a nested `Schedule` showing the recursive walk, a leaf
layer's boundary case (self only), and the `const` overload.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule inner;
  inner.then("proj", nn::Fusion(4, 4));

  nn::Schedule outer;
  outer.then("in", nn::Fusion(3, 4));
  outer.then("block", inner);
  outer.then("out", nn::Fusion(4, 2));

  std::cout << "modules().size() = " << outer.modules().size() << " (self + every nested child)\n";

  return 0;
}
```

Output

```text
modules().size() = 5 (self + every nested child)
```

Five: `outer` itself, `in`, `block` (the nested `Schedule`), `block.proj`
(reached through it), and `out`.

### Example 2

A leaf layer with no children -- `modules()` still returns one entry: itself.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  pluss::ai::nn::Fusion leaf(3, 2);
  std::cout << "leaf modules().size()=" << leaf.modules().size() << " (self only, no children)\n";
  return 0;
}
```

Output

```text
leaf modules().size()=1 (self only, no children)
```

### Example 3

The `const` overload, reached through a `const Fusion&`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Fusion leaf(3, 2);
  const nn::Fusion& const_leaf = leaf;
  std::vector<const nn::Module*> const_mods = const_leaf.modules();
  std::cout << "const overload: modules().size()=" << const_mods.size() << "\n";
  return 0;
}
```

Output

```text
const overload: modules().size()=1
```

## See Also

- [`named_modules`](named_modules.md) -- the same walk, with dotted names
  attached; used in the example above's sibling page.
- [`parameters`](parameters.md) / [`buffers`](buffers.md) -- the parallel
  walks over leaf `Series`, not `Module*`.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="eval.html">
    <span class="label">Previous</span>
    <span class="title">eval</span>
  </a>
  <a class="next-link" href="named_buffers.html">
    <span class="label">Next</span>
    <span class="title">named_buffers</span>
  </a>
</div>
<!-- apiref:nav:end -->
