<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9c5fd1082879 -->

# `pluss::ai::nn::Module::named_buffers`

[Back to `pluss::ai::nn::Module`](../Module.md)

## Signature

```cpp
std::vector<Module::BufferRef> nn::Module::named_buffers();
std::vector<Module::ConstBufferRef> nn::Module::named_buffers() const;
```

## What It Does

The same recursive buffer walk as [`buffers`](buffers.md), with each
result's dotted registration name attached -- the buffer counterpart to
[`named_parameters`](named_parameters.md). As with `buffers()`, no type in
this reference currently registers a buffer, so this returns empty today;
it exists for the same forward-looking reason.

## Parameters

None.

## Returns

The same recursive buffer walk as [`buffers`](buffers.md), but each entry is
a `BufferRef{std::string name, Series* series}` (or `ConstBufferRef` for the
`const` overload) instead of a bare pointer. `name` is dotted with the
registering child's own registration name at each level -- a buffer named
`"count"` registered two `module_record` levels deep would read
`"child_name.grandchild_name.count"`.

## Errors

None.

## Practical Examples

Four examples: the same custom subclass as [`buffers`](buffers.md) read
through `named_buffers()` to show the attached name, the empty case, dotted
names across two registered children, and the `const` overload.

### Example 1

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

  void observe() { count_ = count_.add(1); }

private:
  pluss::Series count_;
};

int main() {
  RunningCountImpl tracker;
  tracker.observe();
  tracker.observe();
  tracker.observe();

  for (auto& named : tracker.named_buffers()) {
    std::cout << "'" << named.name << "' = " << *named.series << "\n";
  }

  return 0;
}
```

Output

```text
'count' = pluss::Series(shape=[1], strides=[1], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=[3]
```

At the top level (no enclosing `module_record`), the name is exactly the
registration name (`"count"`), with no prefix.

### Example 2

A built-in layer has no buffers to name.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  pluss::ai::nn::Fusion layer(3, 2);
  std::cout << "Fusion named_buffers().empty()=" << std::boolalpha << layer.named_buffers().empty() << "\n";
  return 0;
}
```

Output

```text
Fusion named_buffers().empty()=true
```

### Example 3

Two `RunningCountImpl` children registered under a third module -- each
buffer's name is prefixed with its own registering child's name.

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
  for (auto& named : pair.named_buffers()) {
    std::cout << "'" << named.name << "'\n";
  }
  return 0;
}
```

Output

```text
'a.count'
'b.count'
```

### Example 4

The `const` overload, returning `ConstBufferRef` entries.

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

  for (auto& named : const_tracker.named_buffers()) {
    std::cout << "'" << named.name << "' (ConstBufferRef)\n";
  }

  return 0;
}
```

Output

```text
'count' (ConstBufferRef)
```

## See Also

- [`buffers`](buffers.md) -- the same walk without names.
- [`named_parameters`](named_parameters.md) -- the parallel walk over
  gradient-tracked, learned state.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="modules.html">
    <span class="label">Previous</span>
    <span class="title">modules</span>
  </a>
  <a class="next-link" href="named_modules.html">
    <span class="label">Next</span>
    <span class="title">named_modules</span>
  </a>
</div>
<!-- apiref:nav:end -->
