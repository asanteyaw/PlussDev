<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 386cf0271e62 -->

# `pluss::ai::nn::Module::buffers`

[Back to `pluss::ai::nn::Module`](../Module.md)

## Signature

```cpp
std::vector<Series*> nn::Module::buffers();
std::vector<const Series*> nn::Module::buffers() const;
```

## Parameters

None.

## Returns

A flat `vector` of pointers to every `Series` registered as a buffer on this
module or on any of its registered children (recursively) -- the same
recursive walk `parameters()` does, but over `buffer_record` registrations
instead of `parameter_record` ones. The `const` overload returns
`const Series*` pointers. Order matches depth-first registration order, not
sorted by name -- see [`named_buffers`](named_buffers.md) for the
name-attached form.

## Errors

None.

## Practical Examples

Four examples: a buffer registered by a custom `Module` subclass (since none
of `nn`'s built-in layers currently register one), the empty case on a
built-in layer, recursive collection across two registered children, and the
`const` overload.

### Example 1

`buffer_record` (protected, called from a subclass constructor) is the
mechanism for state that travels with a module but is not learned -- running
statistics, a position table, anything a forward pass reads but an optimizer
never updates. None of the built-in layers in this reference use it today
(`Fusion`'s `weight`/`bias` are learned *parameters*, registered via
`parameter_record`, not buffers) -- so this example defines a minimal custom
subclass to demonstrate the mechanism directly, then confirms a built-in
layer's `buffers()` is empty for contrast.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

namespace nn = pluss::ai::nn;

// A minimal custom Module subclass that registers a buffer -- running
// statistics, a position table, anything that travels with the module but
// is not learned. None of nn's built-in layers currently register any
// buffer (see the second example), so this is the mechanism demonstrated
// directly.
class RunningCountImpl final : public nn::Module {
public:
  RunningCountImpl() : nn::Module(true) {
    count_ = pluss::Series::zeros({1}, pluss::ScalarType::Int64, pluss::pCPU);
    this->buffer_record("count", count_);
  }

  void observe() { count_ = count_.add(1); }

private:
  pluss::Series count_;
};

int main() {
  RunningCountImpl tracker;
  tracker.observe();
  tracker.observe();
  tracker.observe();

  std::cout << "buffer count: " << tracker.buffers().size() << "\n";
  for (auto& named : tracker.named_buffers()) {
    std::cout << "'" << named.name << "' = " << *named.series << "\n";
  }

  return 0;
}
```

Output

```text
buffer count: 1
'count' = pluss::Series(shape=[1], strides=[1], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=[3]
```

### Example 2

For contrast: `Fusion`, a built-in layer, registers `weight`/`bias` as
learned *parameters* (see [`parameters`](parameters.md)), never as buffers.
`buffers()` on any layer in the built-in catalog is empty today.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  pluss::ai::nn::Fusion layer(3, 2);
  std::cout << "Fusion buffers().empty()=" << std::boolalpha << layer.buffers().empty()
            << " -- weight/bias are parameters, not buffers\n";
  return 0;
}
```

Output

```text
Fusion buffers().empty()=true -- weight/bias are parameters, not buffers
```

### Example 3

Two `RunningCountImpl` instances (from Example 1) registered as children of
a third custom module -- `buffers()` collects across both, the same
recursive walk `parameters()` does.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

namespace nn = pluss::ai::nn;

class RunningCountImpl final : public nn::Module {
public:
  RunningCountImpl() : nn::Module(true) {
    count_ = pluss::Series::zeros({1}, pluss::ScalarType::Int64, pluss::pCPU);
    this->buffer_record("count", count_);
  }

private:
  pluss::Series count_;
};
using RunningCount = nn::ModuleHolder<RunningCountImpl>;

class PairImpl final : public nn::Module {
public:
  PairImpl() : nn::Module(true) {
    this->module_record("a", a_);
    this->module_record("b", b_);
  }

private:
  RunningCount a_;
  RunningCount b_;
};

int main() {
  PairImpl pair;
  std::cout << "buffers collected across two registered children: " << pair.buffers().size() << "\n";
  return 0;
}
```

Output

```text
buffers collected across two registered children: 2
```

### Example 4

The `const` overload, returning `const Series*` pointers -- reached through
a `const RunningCountImpl&` rather than a mutable one.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

namespace nn = pluss::ai::nn;

class RunningCountImpl final : public nn::Module {
public:
  RunningCountImpl() : nn::Module(true) {
    count_ = pluss::Series::zeros({1}, pluss::ScalarType::Int64, pluss::pCPU);
    this->buffer_record("count", count_);
  }

private:
  pluss::Series count_;
};

int main() {
  RunningCountImpl tracker;
  const RunningCountImpl& const_tracker = tracker;

  std::vector<const pluss::Series*> bufs = const_tracker.buffers();
  std::cout << "const overload: buffers().size()=" << bufs.size() << "\n";

  return 0;
}
```

Output

```text
const overload: buffers().size()=1
```

## See Also

- [`named_buffers`](named_buffers.md) -- the same walk, with dotted names
  attached.
- [`parameters`](parameters.md) -- the parallel walk over gradient-tracked,
  learned state.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="eval.html">
    <span class="label">Next</span>
    <span class="title">eval</span>
  </a>
</div>
<!-- apiref:nav:end -->
