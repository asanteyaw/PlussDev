<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a8ef5cf62d20 -->

# `pluss::ai::nn::Module::parameters`

[Back to `pluss::ai::nn::Module`](../Module.md)

## Signature

```cpp
std::vector<Series*> nn::Module::parameters();
std::vector<const Series*> nn::Module::parameters() const;
```

## What It Does

Walks this module's registration tree and collects every `Series` any
layer registered as trainable state (via `parameter_record`), flattened
into one list with no name attached -- the input an
[`optim::Optimizer`](../../optim/Optimizer.md) needs when built from a raw
pointer list rather than directly from a `Module&`. The walk is recursive
and depth-first, so a `Schedule` of several layers returns every stage's
parameters in one call without the caller visiting each stage by hand.

## Parameters

None.

## Returns

A flat `vector` of pointers to every `Series` registered as a parameter
(via `parameter_record`) on this module or on any of its registered
children, recursively. This is what you pass to an
[`optim::Optimizer`](../../optim/Optimizer.md) constructor when you are not
constructing it directly from a `Module&`. The `const` overload returns
`const Series*` pointers, useful for read-only introspection where the
non-`const` overload would let a caller mutate weights by accident.

## Errors

None.

## Practical Examples

Three examples: a single layer's parameters through both overloads, the
empty case, and recursive collection across two levels of nesting.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Fusion layer(3, 2);  // in_features=3, out_features=2, bias=true (default)

  std::vector<pluss::Series*> params = layer.parameters();
  std::cout << "parameter count: " << params.size() << "\n";
  for (std::size_t i = 0; i < params.size(); ++i) {
    std::cout << "param[" << i << "] defined=" << std::boolalpha << params[i]->defined()
              << " numel=" << params[i]->numel() << "\n";
  }

  const nn::Fusion& const_layer = layer;
  std::vector<const pluss::Series*> const_params = const_layer.parameters();
  std::cout << "const overload count: " << const_params.size() << "\n";

  return 0;
}
```

Output

```text
parameter count: 2
param[0] defined=true numel=6
param[1] defined=true numel=2
const overload count: 2
```

`param[0]` is `weight` (shape `[2, 3]`, 6 elements); `param[1]` is `bias`
(shape `[2]`, 2 elements) -- registration order, matching the order
`FusionImpl`'s constructor calls `parameter_record`.

### Example 2

A bare `Module` -- no subclass, nothing registered -- has no parameters at
all. `parameters()` returns an empty vector rather than failing.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  pluss::ai::nn::Module m;  // no subclass, nothing registered
  std::cout << "bare Module parameters().empty()=" << std::boolalpha << m.parameters().empty() << "\n";
  return 0;
}
```

Output

```text
bare Module parameters().empty()=true
```

### Example 3

Two levels of `Schedule` nesting -- `parameters()` collects across both
without you walking the tree yourself.

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

  std::cout << "total parameters across two nesting levels: " << outer.parameters().size() << "\n";

  return 0;
}
```

Output

```text
total parameters across two nesting levels: 6
```

Six: `weight` + `bias` for each of the three `Fusion` layers (`in`,
`block.proj`, `out`) -- the nesting depth of `block` does not change how
many parameters it contributes, only their names (see
[`named_parameters`](named_parameters.md)).

## See Also

- [`named_parameters`](named_parameters.md) -- the same walk, with dotted
  names attached.
- [`buffers`](buffers.md) -- the parallel walk over non-gradient-tracked
  state.
- [`optim::Optimizer`](../../optim/Optimizer.md) -- the usual consumer of
  this method's result.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="named_parameters.html">
    <span class="label">Previous</span>
    <span class="title">named_parameters</span>
  </a>
  <a class="next-link" href="restore.html">
    <span class="label">Next</span>
    <span class="title">restore</span>
  </a>
</div>
<!-- apiref:nav:end -->
