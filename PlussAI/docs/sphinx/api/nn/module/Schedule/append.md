<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: bdbff023f343 -->

# `pluss::ai::nn::Schedule::append`

[Back to `pluss::ai::nn::Schedule`](../Schedule.md)

## Signature

```cpp
template <typename Layer>
nn::Schedule& nn::Schedule::append(Layer&& layer);

template <typename Layer>
nn::Schedule& nn::Schedule::append(std::string name, Layer&& layer);

template <typename Layer>
nn::Schedule& nn::Schedule::append(pluss::RangeView<Layer> layers);

template <typename Layer>
nn::Schedule& nn::Schedule::append(std::initializer_list<Layer> layers);
```

All four overloads assume `Series -> Series` -- use [`append_as`](append_as.md) when
a stage's real input/output types are something else (a `std::tuple<...>`,
for instance).

## What It Does

Appends one or more stages to the end of this `Schedule`'s chain, after
checking each stage's declared input type against whatever the chain
currently ends with. The first two overloads add one stage per call and
are the primary way a `Schedule` is built up: a sequence of `.append(...)`
calls, each returning `*this` so they chain, reading top to bottom in the
order layers actually run. Any object exposing `forward`/`operator()` with
the right shape qualifies as a stage, not just an `nn::Module` -- see
Example 2.

The remaining two overloads add several **same-typed**, auto-named stages
in one call -- a plain ordered list -- so a run of identical layer types
does not need one `.append(...)` per layer:

- `pluss::RangeView<Layer>` accepts a non-owning view over an existing
  container (`std::vector<Layer>`, `std::array<Layer, N>`, a span) of
  same-typed stages.
- `std::initializer_list<Layer>` is the disambiguating overload for a bare
  `{...}` literal -- `pluss::RangeView<Layer>` alone cannot deduce `Layer`
  from a braced-init-list argument, since deducing a class template
  parameter that way is not something list-initialization supports; a
  `std::initializer_list<T>` parameter is the one form the language does
  let a template argument be deduced through. See Example 4.

Both are strictly same-typed per call: a list mixing different layer types
(a `Fusion` alongside an `RNN`, say) has to go through the single-stage
overloads instead, chained (`.append(a).append(b)`) or via the variadic
[`Schedule` constructor](../Schedule.md#signature) -- neither of those two
places restricts the stages to one type, since each argument is its own
compile-time template parameter there.

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
  every `append`/`append_as`/`insert`/`insert_as` call, regardless of name or
  position -- see Example 3 on [`insert`](insert.md) for what that means once
  stages are added out of order.

`pluss::RangeView<Layer> layers` / `std::initializer_list<Layer> layers`
: A same-typed run of stages to append in order, each auto-named exactly as
  the single-`Layer` overload would name it. Equivalent to calling the
  single-stage overload once per element.

## Returns

`Schedule&` -- a reference to `*this`, so calls chain:
`net.append(a).append(b).append(c)`.

## Errors

- `name` already used by another stage in this `Schedule` -- throws (for
  the list overloads, on whichever element's name collides first; every
  element before it has already been added).
- `layer`'s declared output type does not match the type the next existing
  stage (if any) declares as its input, or the previous stage's declared
  output does not match `Series` when appending to a non-empty chain whose
  last stage's output is not `Series` -- throws. See the category
  [Introduction](../Introduction.md)'s "Type-Checked, Not Shape-Checked"
  note for exactly what this check does and does not catch.

## Practical Examples

Four examples: building a chain with auto-named stages and running it, a
stage that is not an `nn::Module` at all, the duplicate-name error, and the
list-style multi-append.

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
  net.append(nn::Fusion(3, 4)).append(nn::Fusion(4, 2));  // auto-named "0", "1"

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
  net.append("scale", double_it).append("head", nn::Fusion(3, 1));

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
  net.append("a", nn::Fusion(3, 4));

  try {
    net.append("a", nn::Fusion(4, 2));  // "a" is already taken
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

### Example 4

Three `Fusion` layers, added in one call from a bare brace literal --
equivalent to three separate `.append(...)` calls, each auto-named the
same way.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule net;
  net.append({nn::Fusion(3, 4), nn::Fusion(4, 4), nn::Fusion(4, 2)});  // auto-named "0", "1", "2"

  std::cout << "size()=" << net->size() << "\n";
  for (std::size_t i = 0; i < net->size(); ++i) {
    std::cout << "  stage " << i << " name='" << net->name_at(i) << "'\n";
  }

  return 0;
}
```

Output

```text
size()=3
  stage 0 name='0'
  stage 1 name='1'
  stage 2 name='2'
```

## See Also

- [`append_as`](append_as.md) -- the same operation for a stage whose real type
  is not `Series -> Series`.
- [`insert`](insert.md) -- adds a stage at a specific position instead of
  the end.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="append_as.html">
    <span class="label">Next</span>
    <span class="title">append_as</span>
  </a>
</div>
<!-- apiref:nav:end -->
