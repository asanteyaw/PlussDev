# `pluss::ai::nn::Identity`

A pass-through module: returns its input unchanged. Its purpose is not the
computation (there is none) but slotting into a
[`Schedule`](../module/Schedule.md) wherever a stage is structurally
required but no transformation is wanted -- most commonly as the "no
activation" branch of code that conditionally chooses an activation, so the
caller never needs a special case for "don't add a stage at all."

## Signature

```cpp
nn::Identity::Identity();
```

## Construction

`Identity` takes no options -- default-construct it and use it directly, or
add it straight into a [`Schedule`](../module/Schedule.md) with
[`then`](../module/Schedule/then.md).

## State

None. `Identity` has no parameters and no configuration; every instance
behaves identically.

## Methods

`forward(input)` / `operator()(input)` -- `Series`
: Returns `input`, unchanged. Never throws.

## Practical Examples

Two examples: the pass-through property confirmed directly, and the
"conditional activation" pattern `Identity` exists for -- a helper that
builds a two-stage block with either a real activation or `Identity`
depending on a flag, with no branching required at the call site.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series x = pluss::make_series({1.0, -2.0, 3.5}, {3}, f64);

  nn::Identity identity;
  const pluss::Series y = identity.forward(x);

  std::cout << "x == identity.forward(x): " << std::boolalpha << x.allclose(y) << "\n";

  return 0;
}
```

Output

```text
x == identity.forward(x): true
```

### Example 2

`make_block` always adds an `"act"` stage -- `Identity` when `use_relu` is
`false` means the caller never has to special-case "no activation" as "no
stage."

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

namespace nn = pluss::ai::nn;

nn::Schedule make_block(nn::Fusion fc, bool use_relu) {
  nn::Schedule block;
  block.then("fc", fc);
  if (use_relu) {
    block.then("act", nn::ReLU());
  } else {
    block.then("act", nn::Identity());
  }
  return block;
}

int main() {
  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);

  nn::Fusion fc(2, 2, /*bias=*/false, f64);
  fc->weight().copy_(pluss::make_series({1.0, 0.0, -1.0, 0.0}, {2, 2}, f64));

  const nn::Schedule with_relu = make_block(fc, true);
  const nn::Schedule without = make_block(fc, false);

  const pluss::Series x = pluss::make_series({3.0, 1.0}, {1, 2}, f64);
  std::cout << "fc(x)                = " << fc.forward(x) << "\n";
  std::cout << "block with Identity() = " << without.forward(x) << " (identical to fc(x))\n";
  std::cout << "block with ReLU()     = " << with_relu.forward(x) << " (the negative entry is clipped)\n";

  return 0;
}
```

Output

```text
fc(x)                = pluss::Series(shape=[1,2], strides=[2,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[3, -3]]
block with Identity() = pluss::Series(shape=[1,2], strides=[2,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[3, -3]] (identical to fc(x))
block with ReLU()     = pluss::Series(shape=[1,2], strides=[2,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[3, 0]] (the negative entry is clipped)
```

## Changelog

- **0.1** — the pass-through module, no options.

## See Also

- [`Schedule`](../module/Schedule.md) -- the container `Identity` is almost
  always used inside.
- [`ReLU`](ReLU.md) -- the real activation `Identity` most often stands in
  for the absence of.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
