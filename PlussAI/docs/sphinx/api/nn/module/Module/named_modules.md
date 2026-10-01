<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 689f3386ae67 -->

# `pluss::ai::nn::Module::named_modules`

[Back to `pluss::ai::nn::Module`](../Module.md)

## Signature

```cpp
std::vector<Module::ModuleRef> nn::Module::named_modules();
std::vector<Module::ConstModuleRef> nn::Module::named_modules() const;
```

## What It Does

The same recursive module walk as [`modules`](modules.md), with each
result's dotted registration name attached -- the module counterpart to
[`named_parameters`](named_parameters.md). Unlike
[`Schedule`](../Schedule.md)'s own stage lookups
([`contains`](../Schedule/contains.md),
[`index_of`](../Schedule/index_of.md),
[`module_at`](../Schedule/module_at.md)), which only see that `Schedule`'s
own immediate stages, `named_modules` walks the *entire* registration tree
by dotted name -- useful for finding a specific layer nested several levels
deep without walking children by hand.

## Parameters

None.

## Returns

The same recursive walk as [`modules`](modules.md), but each entry is a
`ModuleRef{std::string name, Module* module}` (or `ConstModuleRef`) instead
of a bare pointer. The first entry is always **this module itself**, with an
**empty name** (`""`) -- there is no registration name for the root of the
walk, since nothing registered it. Every other entry's name is dotted through
however many `module_record` levels separate it from the root.

## Errors

None.

## Practical Examples

Three examples: the nested `Schedule` from [`modules`](modules.md) read
through `named_modules()`, a leaf layer's boundary case, and the `const`
overload.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule inner;
  inner.append("proj", nn::Fusion(4, 4));

  nn::Schedule outer;
  outer.append("in", nn::Fusion(3, 4));
  outer.append("block", inner);
  outer.append("out", nn::Fusion(4, 2));

  for (auto& named : outer.named_modules()) {
    std::cout << "'" << (named.name.empty() ? "<self>" : named.name) << "'\n";
  }

  return 0;
}
```

Output

```text
'<self>'
'in'
'block'
'block.proj'
'out'
```

(The program prints `<self>` for readability; the real value in `named.name`
for that first entry is the empty string `""`.) `block.proj` shows the dotted
nesting: `proj` was registered inside `inner`, which was itself registered
under the name `"block"` inside `outer`.

### Example 2

A leaf layer's single entry -- itself, named `""`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  pluss::ai::nn::Fusion leaf(3, 2);
  auto named = leaf.named_modules();
  std::cout << "leaf named_modules().size()=" << named.size() << ", name='" << named.front().name << "'\n";
  return 0;
}
```

Output

```text
leaf named_modules().size()=1, name=''
```

### Example 3

The `const` overload, returning `ConstModuleRef` entries.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Fusion leaf(3, 2);
  const nn::Fusion& const_leaf = leaf;
  auto named = const_leaf.named_modules();
  std::cout << "const overload: named_modules().size()=" << named.size() << "\n";
  return 0;
}
```

Output

```text
const overload: named_modules().size()=1
```

## See Also

- [`modules`](modules.md) -- the same walk without names.
- [`named_parameters`](named_parameters.md) /
  [`named_buffers`](named_buffers.md) -- the parallel walks over leaf
  `Series`, dotted the same way.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="named_buffers.html">
    <span class="label">Previous</span>
    <span class="title">named_buffers</span>
  </a>
  <a class="next-link" href="named_parameters.html">
    <span class="label">Next</span>
    <span class="title">named_parameters</span>
  </a>
</div>
<!-- apiref:nav:end -->
