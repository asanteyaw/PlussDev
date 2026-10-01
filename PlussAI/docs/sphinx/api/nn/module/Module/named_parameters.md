<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 332f49d2abc3 -->

# `pluss::ai::nn::Module::named_parameters`

[Back to `pluss::ai::nn::Module`](../Module.md)

## Signature

```cpp
std::vector<Module::ParameterRef> nn::Module::named_parameters();
std::vector<Module::ConstParameterRef> nn::Module::named_parameters() const;
```

## What It Does

The same recursive parameter walk as [`parameters`](parameters.md), but
each result carries its dotted registration name alongside the pointer --
what you want when identifying, logging, or selectively touching specific
weights (a per-layer learning rate, a checkpoint diff) rather than treating
every parameter identically. Names compose through nesting: a `Schedule`
prefixes each stage's own parameter names with that stage's registration
name and a dot, so a deeply nested weight's name reflects its full path
from the root.

## Parameters

None.

## Returns

The same recursive parameter walk as [`parameters`](parameters.md), but each
entry is a `ParameterRef{std::string name, Series* series}` (or
`ConstParameterRef`) instead of a bare pointer. Each concrete layer names its
own parameters at registration (`Fusion` registers `"weight"` and, when
`bias` is enabled, `"bias"`); a `Schedule` prefixes each stage's own names
with the stage's registration name and a dot.

## Errors

None.

## Practical Examples

Four examples: a two-stage `Schedule` showing which dotted names come out,
the empty case, two levels of nesting, and the `const` overload.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule net;
  net.append("in", nn::Fusion(3, 4));
  net.append("out", nn::Fusion(4, 2, /*bias=*/false));

  for (auto& named : net.named_parameters()) {
    std::cout << "'" << named.name << "' defined=" << std::boolalpha << named.series->defined() << "\n";
  }

  return 0;
}
```

Output

```text
'in.weight' defined=true
'in.bias' defined=true
'out.weight' defined=true
```

`out.bias` never appears — the `out` stage was constructed with
`bias=false`, so `Fusion` never registers a bias parameter for it at all
(not an undefined placeholder; the entry itself is absent).

### Example 2

A bare `Module`, nothing registered -- `named_parameters()` returns an empty
vector, same as the unnamed form.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  pluss::ai::nn::Module m;
  std::cout << "bare Module named_parameters().empty()=" << std::boolalpha << m.named_parameters().empty() << "\n";
  return 0;
}
```

Output

```text
bare Module named_parameters().empty()=true
```

### Example 3

Two levels of `Schedule` nesting -- the dotted name accumulates one segment
per level, in registration order.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule inner;
  inner.append("proj", nn::Fusion(4, 4));

  nn::Schedule outer;
  outer.append("block", inner);

  for (auto& named : outer.named_parameters()) {
    std::cout << "'" << named.name << "'\n";
  }

  return 0;
}
```

Output

```text
'block.proj.weight'
'block.proj.bias'
```

### Example 4

The `const` overload, returning `ConstParameterRef` entries instead of
`ParameterRef` -- reached through a `const Fusion&` rather than a mutable
one, the same way [`parameters`](parameters.md)'s own `const` overload works.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Fusion layer(3, 2);
  const nn::Fusion& const_layer = layer;

  std::vector<pluss::ai::nn::Module::ConstParameterRef> named = const_layer.named_parameters();
  for (auto& entry : named) {
    std::cout << "'" << entry.name << "' (ConstParameterRef)\n";
  }

  return 0;
}
```

Output

```text
'weight' (ConstParameterRef)
'bias' (ConstParameterRef)
```

## See Also

- [`parameters`](parameters.md) -- the same walk without names.
- [`named_buffers`](named_buffers.md) -- the parallel walk over
  non-gradient-tracked state.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="named_modules.html">
    <span class="label">Previous</span>
    <span class="title">named_modules</span>
  </a>
  <a class="next-link" href="parameters.html">
    <span class="label">Next</span>
    <span class="title">parameters</span>
  </a>
</div>
<!-- apiref:nav:end -->
