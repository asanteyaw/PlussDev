<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1a06451bc9f1 -->

# `pluss::ai::nn::Schedule::stage_info`

[Back to `pluss::ai::nn::Schedule`](../Schedule.md)

Only reachable via `->` (`net->stage_info(index)`). `at(index)` is a
one-line alias for this same method.

## Signature

```cpp
Schedule::Impl::StageInfo nn::Schedule::Impl::stage_info(std::size_t index) const;
Schedule::Impl::StageInfo nn::Schedule::Impl::at(std::size_t index) const;  // alias
```

`StageInfo` is a plain data struct with four fields (`name`, `input_type`,
`output_type`, `module_stage`) -- taught field by field under `## Returns`
below, not pasted here.

## What It Does

Bundles one stage's name, declared input/output types, and whether it is
an `nn::Module`, into a single struct -- everything
[`name_at`](name_at.md)/[`input_type_at`](input_type_at.md)/[`output_type_at`](output_type_at.md)
report individually, gathered in one call. Reach for `stage_info` when
inspecting or logging a stage's full description at once rather than
making three separate lookups for the same position.

## Parameters

`std::size_t index`
: A position in `[0, size())`.

## Returns

A `StageInfo` with four fields:

- `name` -- the stage's name, exactly [`name_at(index)`](name_at.md).
- `input_type`, `output_type` -- the stage's declared input/output types,
  already converted to `std::string` via `std::type_info::name()` -- the
  same raw, mangled spelling
  [`input_type_at`](input_type_at.md)/[`output_type_at`](output_type_at.md)
  give you as a `std::type_info&`, just pre-stringified for convenience
  (printing, logging) rather than for `==` comparison.
- `module_stage` -- `true` when the stage is an `nn::Module` (reachable
  through [`module_at`](module_at.md)); `false` for a plain callable with no
  learnable state -- see the category [Introduction](../Introduction.md).

## Errors

- `index >= size()` -- throws.

## Practical Examples

Three examples: reading every field for one stage, the two type strings for
an ordinary `Series -> Series` layer, and `module_stage` distinguishing a
real layer from a plain lambda.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule net;
  net.append("head", nn::Fusion(3, 2));

  const auto info = net->stage_info(0);
  std::cout << "name='" << info.name << "'\n";
  std::cout << "input_type='" << info.input_type << "'\n";
  std::cout << "output_type='" << info.output_type << "'\n";
  std::cout << "module_stage=" << std::boolalpha << info.module_stage << "\n";

  return 0;
}
```

Output

```text
name='head'
input_type='N5pluss5codem6SeriesE'
output_type='N5pluss5codem6SeriesE'
module_stage=true
```

(`N5pluss5codem6SeriesE` is the raw mangled name of `pluss::codem::Series`,
the internal type `pluss::Series` aliases -- implementation-defined, not
meant to be pattern-matched, only displayed or logged.)

### Example 2

Both stages in an ordinary chain report the same `input_type`/`output_type`
string, since both declare plain `Series`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule net;
  net.append("in", nn::Fusion(3, 4));
  net.append("out", nn::Fusion(4, 2));

  const auto manifest = net->stage_manifest();
  for (const auto& info : manifest) {
    std::cout << "'" << info.name << "' module_stage=" << std::boolalpha << info.module_stage << "\n";
  }

  return 0;
}
```

Output

```text
'in' module_stage=true
'out' module_stage=true
```

### Example 3

`module_stage` is the non-throwing way to tell a real layer from a plain
callable stage, before calling `module_at` on it.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  auto double_it = [](const pluss::Series& x) { return x.mul(2.0); };

  nn::Schedule net;
  net.append("scale", double_it);    // not an nn::Module
  net.append("head", nn::Fusion(3, 2));

  for (const auto& info : net->stage_manifest()) {
    std::cout << "'" << info.name << "' module_stage=" << std::boolalpha << info.module_stage << "\n";
  }

  return 0;
}
```

Output

```text
'scale' module_stage=false
'head' module_stage=true
```

## See Also

- [`stage_manifest`](stage_manifest.md) -- the same fields for every stage
  at once.
- [`module_at`](module_at.md) -- throws for a stage where `module_stage`
  would be `false`.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="size.html">
    <span class="label">Previous</span>
    <span class="title">size</span>
  </a>
  <a class="next-link" href="stage_manifest.html">
    <span class="label">Next</span>
    <span class="title">stage_manifest</span>
  </a>
</div>
<!-- apiref:nav:end -->
