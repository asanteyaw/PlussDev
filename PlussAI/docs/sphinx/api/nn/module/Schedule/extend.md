<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2ae7b3d2b096 -->

# `pluss::ai::nn::Schedule::extend`

[Back to `pluss::ai::nn::Schedule`](../Schedule.md)

## Signature

```cpp
nn::Schedule& nn::Schedule::extend(const nn::Schedule& other);

template <typename Container>
ScheduleImpl& /* only on the underlying Impl, reached via -> */ extend(const Container& layers);
```

## What It Does

Appends multiple stages in one call. The two overloads are not symmetric in
how they are reached: `Schedule` itself only re-declares
`extend(const Schedule&)` (returning `Schedule&`, for chaining) --
the template `extend(const Container&)` overload exists only on the
underlying implementation type and needs `->` (`net->extend(container)`),
not `.` (`net.extend(container)`), the same `.`/`->` boundary the
[`module` Introduction](../Introduction.md) explains for every layer type's
own accessors. Reach for `extend` instead of repeated `append` calls when
stages already live in a container or another `Schedule`, and when the
looser (never-throws) name-collision handling described below is
acceptable.

## Parameters

`const Schedule& other`
: Every stage in `other` is deep-copied (`StageBase::clone()`) and appended
  to `*this`, in order. `other` is left unmodified. A name already used in
  `*this` is silently replaced with a fresh auto-generated name (the same
  counter [`append`](append.md) uses) rather than rejected -- `extend` never
  throws over a name collision the way `append`/`insert` do.

`const Container& layers`
: Any range of stage-like values (a `std::vector<Fusion>`, for instance).
  Equivalent to calling [`append(layer)`](append.md) once per element, in
  order -- every element is auto-named, and the usual per-element type
  checks against the growing chain still apply.

## Returns

`Schedule&` for the `Schedule`-argument overload; `ScheduleImpl&` (via `->`)
for the container overload.

## Errors

- Container overload: any element's declared type fails the usual
  [`append`](append.md) chain check -- throws, having already appended every
  prior element.
- `Schedule`-argument overload: a stage type that does not fit the growing
  chain -- throws (`clone()` itself requires each stage's layer type to be
  copy-constructible, which every built-in layer and ordinary lambda is).

## Practical Examples

Three examples: cloning one `Schedule`'s stages onto the end of another, the
name-collision auto-rename, and building from a plain container.

### Example 1

`block`'s two stages are cloned onto the end of `net`; `block` itself is
untouched afterward.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule block;
  block.append("conv", nn::Fusion(4, 4));
  block.append("act", nn::ReLU());

  nn::Schedule net;
  net.append("in", nn::Fusion(3, 4));
  net.extend(block);  // clones block's stages onto the end of net

  for (std::size_t i = 0; i < net->size(); ++i) {
    std::cout << "stage " << i << ": '" << net->name_at(i) << "'\n";
  }
  std::cout << "block untouched, still size()=" << block->size() << "\n";

  return 0;
}
```

Output

```text
stage 0: 'in'
stage 1: 'conv'
stage 2: 'act'
block untouched, still size()=2
```

### Example 2

When `other` has a stage name `*this` already uses, `extend` renames it
instead of throwing -- the fresh name comes from `*this`'s own auto-name
counter.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule block;
  block.append("shared_name", nn::Fusion(4, 4));

  nn::Schedule net;
  net.append("shared_name", nn::Fusion(3, 4));  // net already has this name
  net.extend(block);  // collision -- extend auto-renames instead of failing

  std::cout << "stage 0: '" << net->name_at(0) << "'\n";
  std::cout << "stage 1: '" << net->name_at(1) << "' (renamed to avoid colliding with stage 0)\n";

  return 0;
}
```

Output

```text
stage 0: 'shared_name'
stage 1: '0' (renamed to avoid colliding with stage 0)
```

### Example 3

Building from a plain `std::vector` of layers -- note the `->`, since the
container-taking overload is reached through the underlying implementation,
not through `Schedule` itself.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>
#include <vector>

int main() {
  namespace nn = pluss::ai::nn;

  std::vector<nn::Fusion> layers;
  layers.emplace_back(4, 4);
  layers.emplace_back(4, 4);

  nn::Schedule net;
  net.append("in", nn::Fusion(3, 4));
  net->extend(layers);  // template overload -- only on ScheduleImpl, calls append(layer) per element

  for (std::size_t i = 0; i < net->size(); ++i) {
    std::cout << "stage " << i << ": '" << net->name_at(i) << "'\n";
  }

  return 0;
}
```

Output

```text
stage 0: 'in'
stage 1: '0'
stage 2: '1'
```

## See Also

- [`append`](append.md) -- what `extend` calls once per element, for the
  container overload.
- [`clear`](clear.md) -- the opposite of accumulating stages: removes all
  of them.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="erase.html">
    <span class="label">Previous</span>
    <span class="title">erase</span>
  </a>
  <a class="next-link" href="forward.html">
    <span class="label">Next</span>
    <span class="title">forward</span>
  </a>
</div>
<!-- apiref:nav:end -->
