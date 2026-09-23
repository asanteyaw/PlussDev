<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a86c0d17fbef -->

# `pluss::ai::nn::Schedule::then`

[Back to `pluss::ai::nn::Schedule`](../Schedule.md)

## Signature

```cpp
template <typename Layer>
nn::Schedule& nn::Schedule::then(Layer&& layer);

template <typename Layer>
nn::Schedule& nn::Schedule::then(std::string name, Layer&& layer);
```

Both overloads assume `Series -> Series` -- use [`then_as`](then_as.md) when
a stage's real input/output types are something else (a `std::tuple<...>`,
for instance).

## Parameters

`Layer&& layer`
: Anything exposing `forward(const Series&) -> Series` or
  `operator()(const Series&) -> Series` -- an `nn::Module`/`ModuleHolder`
  layer, or a plain callable with no learnable state at all.

`std::string name`
: The stage's name, used by [`contains`](contains.md)/[`index_of`](index_of.md)/
  [`name_at`](name_at.md) and as the dotted prefix in
  [`named_parameters`](../Module/named_parameters.md). Must be non-empty and
  unique among this `Schedule`'s current stages. When omitted, the name is
  generated as `std::to_string` of an internal counter that increments on
  every `then`/`then_as`/`insert`/`insert_as` call, regardless of name or
  position -- see Example 3 on [`insert`](insert.md) for what that means once
  stages are added out of order.

## Returns

`Schedule&` -- a reference to `*this`, so calls chain:
`net.then(a).then(b).then(c)`.

## Errors

- `name` already used by another stage in this `Schedule` -- throws.
- `layer`'s declared output type does not match the type the next existing
  stage (if any) declares as its input, or the previous stage's declared
  output does not match `Series` when appending to a non-empty chain whose
  last stage's output is not `Series` -- throws. See the category
  [Introduction](../Introduction.md)'s "Type-Checked, Not Shape-Checked"
  note for exactly what this check does and does not catch.

## Practical Examples

Three examples: building a chain with auto-named stages and running it, a
stage that is not an `nn::Module` at all, and the duplicate-name error.

### Example 1

Two `Fusion` layers, chained with the auto-naming overload, then run
end to end.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule net;
  net.then(nn::Fusion(3, 4)).then(nn::Fusion(4, 2));  // auto-named "0", "1"

  std::cout << "size()=" << net->size() << "\n";
  for (std::size_t i = 0; i < net->size(); ++i) {
    std::cout << "  stage " << i << " name='" << net->name_at(i) << "'\n";
  }

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0}, {1, 3}, f64);
  net.to(pluss::ScalarType::Float64);
  const pluss::Series y = net.forward(x);
  std::cout << "output shape=" << y.sizes()[0] << "x" << y.sizes()[1] << "\n";

  return 0;
}
```

Output

```text
size()=2
  stage 0 name='0'
  stage 1 name='1'
output shape=1x2
```

### Example 2

A stage does not have to be an `nn::Module` -- a plain lambda with no
learnable state at all qualifies just as well, as long as it exposes
`forward`/`operator()` with the right shape.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  auto double_it = [](const pluss::Series& x) { return x.mul(2.0); };

  nn::Schedule net;
  net.then("scale", double_it).then("head", nn::Fusion(3, 1));

  std::cout << "stage 0 name='" << net->name_at(0) << "'\n";
  std::cout << "stage 1 name='" << net->name_at(1) << "'\n";
  std::cout << "parameters().size()=" << net.parameters().size()
            << " (only 'head' contributes -- the lambda has none)\n";

  return 0;
}
```

Output

```text
stage 0 name='scale'
stage 1 name='head'
parameters().size()=2 (only 'head' contributes -- the lambda has none)
```

### Example 3

Adding a stage under a name that is already taken throws, and the rejected
stage is never added.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule net;
  net.then("a", nn::Fusion(3, 4));

  try {
    net.then("a", nn::Fusion(4, 2));  // "a" is already taken
    std::cout << "(did not throw)\n";
  } catch (const std::exception& e) {
    std::cout << "caught: " << e.what() << "\n";
  }

  std::cout << "size() is still " << net->size() << " -- the rejected stage was never added\n";

  return 0;
}
```

Output

```text
caught: pluss::nn::Schedule duplicate stage name 'a'
size() is still 1 -- the rejected stage was never added
```

## See Also

- [`then_as`](then_as.md) -- the same operation for a stage whose real type
  is not `Series -> Series`.
- [`insert`](insert.md) -- adds a stage at a specific position instead of
  the end.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="stage_manifest.html">
    <span class="label">Previous</span>
    <span class="title">stage_manifest</span>
  </a>
  <a class="next-link" href="then_as.html">
    <span class="label">Next</span>
    <span class="title">then_as</span>
  </a>
</div>
<!-- apiref:nav:end -->
