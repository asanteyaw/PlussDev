# `pluss::ai::nn::Fusion`

The fully connected (dense/linear) layer: a learned affine map
`y = x @ weight^T + bias`. `weight` has shape `[out_features, in_features]`;
`bias`, when enabled, has shape `[out_features]`. This is the workhorse
layer every [`Schedule`](../module/Schedule.md) example in this reference
has already used -- the layer every other layer type in this reference is
typically chained with.

`forward` accepts input of any rank `>= 1`: the last axis must equal
`in_features`, and every leading axis (batch, sequence, or otherwise) is
preserved untouched in the output, with only the trailing axis becoming
`out_features`.

## Signature

```cpp
explicit nn::Fusion::Fusion(nn::FusionOptions options);
nn::Fusion::Fusion(std::int64_t in_features, std::int64_t out_features,
                    bool bias = true, pluss::Config cfg = {}, std::uint64_t seed = 0);

void nn::Fusion::reset_parameters();
```

## Construction

Two equivalent forms: pass a [`FusionOptions`](FusionOptions.md) directly,
or spell out the same fields as positional arguments -- the form every
example elsewhere in this reference uses, since it is the more concise of
the two for the common case:

```cpp
nn::Fusion a(nn::FusionOptions{3, 4, true, {}, 0});  // FusionOptions form
nn::Fusion b(3, 4);                                  // positional form, bias=true, cfg={}, seed=0 (defaults)
```

Neither form has a no-argument overload -- `in_features`/`out_features`
have no meaningful default, since `weight` cannot be allocated without
them.

## State

`weight()` (mutable and `const` overloads) -- `Series&` / `const Series&`
: The learned weight matrix, shape `[out_features, in_features]`.
  Registered as a parameter.

`bias()` (mutable and `const` overloads) -- `Series&` / `const Series&`
: The learned bias vector, shape `[out_features]`, when
  `options().bias == true`. Registered as a parameter in that case; an
  **undefined** `Series` (not a zero vector) when `options().bias ==
  false` -- see Example 2.

`options()` (mutable and `const` overloads) returns the
[`FusionOptions`](FusionOptions.md) this instance was constructed with.

## Methods

`forward(input)` / `operator()(input)` -- `Series`
: Returns `input @ weight().t() + bias()` (the addition skipped entirely
  when `bias` is disabled), with `input`'s leading axes preserved and its
  trailing axis mapped from `in_features` to `out_features`. Throws if
  `input`'s last axis does not equal `in_features`.

`reset_parameters()` -- `void`
: Re-fills `weight()` (and `bias()`, when enabled) in place with a fresh
  uniform draw in `[-1/sqrt(in_features), 1/sqrt(in_features)]`, using
  `options().seed` for `weight` and `options().seed + 1` for `bias`.
  Called once during construction; call it again to reinitialize an
  already-trained layer. Never reassigns `weight()`/`bias()` themselves, so
  registration and gradient tracking are undisturbed across any number of
  calls.

## Practical Examples

Five examples: a hand-verified affine map on a 2-D batch, the `bias=false`
form and the positional constructor, a higher-rank input showing every
leading axis preserved, confirming the initialization bound and
`reset_parameters()`'s reproducibility, and confirming `Fusion` runs on
the MPS backend.

### Example 1

A `weight`/`bias` set by hand (rather than randomly initialized), so the
output can be verified against the exact formula.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Fusion layer(3, 2);  // in_features=3, out_features=2, bias=true (defaults)

  layer->weight().copy_(pluss::make_series({1.0, 0.0, 0.0, 0.0, 1.0, 0.0}, {2, 3}));
  layer->bias().copy_(pluss::make_series({0.5, -0.5}, {2}));

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series x = pluss::make_series({10.0, 20.0, 30.0}, {1, 3}, f64);
  const pluss::Series y = layer.forward(x);

  std::cout << "weight: " << layer->weight() << "\n";
  std::cout << "bias:   " << layer->bias() << "\n";
  std::cout << "y = x @ weight^T + bias: " << y << "\n";

  return 0;
}
```

Output

```text
weight: pluss::Series(shape=[2,3], strides=[3,1], dtype=float32, platform=cpu, numel=6, contiguous=true)
data=[[1, 0, 0],
  [0, 1, 0]]
bias:   pluss::Series(shape=[2], strides=[1], dtype=float32, platform=cpu, numel=2, contiguous=true)
data=[0.5, -0.5]
y = x @ weight^T + bias: pluss::Series(shape=[1,2], strides=[2,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[10.5, 19.5]]
```

### Example 2

`bias=false` registers no bias parameter at all -- `bias()` is an
undefined `Series`, not a zero-filled one, and `forward` computes the pure
linear map. Both constructor forms (`FusionOptions`, positional) produce
identical results.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  // positional-args constructor, bias disabled
  nn::Fusion no_bias(3, 2, /*bias=*/false);
  std::cout << "no_bias.parameters().size()=" << no_bias.parameters().size()
            << " -- only weight registers, bias() is undefined\n";
  std::cout << "no_bias->bias().defined()=" << std::boolalpha << no_bias->bias().defined() << "\n";

  // FusionOptions constructor, equivalent to the positional form above
  nn::FusionOptions opts{};
  opts.in_features = 3;
  opts.out_features = 2;
  opts.bias = false;
  nn::Fusion no_bias2(opts);
  std::cout << "no_bias2.parameters().size()=" << no_bias2.parameters().size() << "\n";

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series x = pluss::make_series({1.0, 1.0, 1.0}, {1, 3}, f64);
  no_bias->weight().copy_(pluss::make_series({1.0, 1.0, 1.0, 2.0, 2.0, 2.0}, {2, 3}));
  const pluss::Series y = no_bias.forward(x);
  std::cout << "no_bias forward(x): " << y << " -- pure x @ weight^T, no shift\n";

  return 0;
}
```

Output

```text
no_bias.parameters().size()=1 -- only weight registers, bias() is undefined
no_bias->bias().defined()=false
no_bias2.parameters().size()=1
no_bias forward(x): pluss::Series(shape=[1,2], strides=[2,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[3, 6]] -- pure x @ weight^T, no shift
```

### Example 3

A `[batch, seq_len, in_features]` input -- every leading axis is
preserved; only the trailing axis changes from `in_features` to
`out_features`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Fusion layer(4, 6);

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  layer.to(pluss::ScalarType::Float64);

  // batch=2, seq_len=3, in_features=4
  const pluss::Series x = pluss::Series::full({2, 3, 4}, 1.0, pluss::ScalarType::Float64);
  const pluss::Series y = layer.forward(x);

  std::cout << "input shape:  [" << x.sizes()[0] << ", " << x.sizes()[1] << ", " << x.sizes()[2] << "]\n";
  std::cout << "output shape: [" << y.sizes()[0] << ", " << y.sizes()[1] << ", " << y.sizes()[2]
            << "] -- every leading dim preserved, only the last axis changes to out_features\n";

  return 0;
}
```

Output

```text
input shape:  [2, 3, 4]
output shape: [2, 3, 6] -- every leading dim preserved, only the last axis changes to out_features
```

### Example 4

The initial (and every `reset_parameters()`-reset) weight is drawn
uniformly from `[-1/sqrt(in_features), 1/sqrt(in_features)]`, and the same
`seed` always reproduces the exact same weight and bias.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <cmath>
#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Fusion layer(16, 8, true, pluss::Config{}, /*seed=*/5);

  const double bound = 1.0 / std::sqrt(16.0);
  const pluss::Series w = layer->weight();
  const pluss::Series wflat = w.reshape({w.numel()});
  std::cout << "bound = 1/sqrt(in_features) = " << bound << "\n";
  std::cout << "weight min >= -bound: " << std::boolalpha << (wflat.amin(0, false).item<double>() >= -bound) << "\n";
  std::cout << "weight max <= bound: " << (wflat.amax(0, false).item<double>() <= bound) << "\n";

  const pluss::Series weight_before = w.clone();
  const pluss::Series bias_before = layer->bias().clone();
  layer->reset_parameters();  // same seed -- reproduces the exact same weight/bias
  std::cout << "reset_parameters() with the same seed reproduces the same weight: "
            << weight_before.allclose(layer->weight()) << "\n";
  std::cout << "reset_parameters() with the same seed reproduces the same bias: "
            << bias_before.allclose(layer->bias()) << "\n";

  return 0;
}
```

Output

```text
bound = 1/sqrt(in_features) = 0.25
weight min >= -bound: true
weight max <= bound: true
reset_parameters() with the same seed reproduces the same weight: true
reset_parameters() with the same seed reproduces the same bias: true
```

### Example 5

`Fusion` runs on the MPS backend the same way it does on CPU -- moving the
module with `.to(pMPS)` moves both `weight` and `bias`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Series;

  nn::Fusion layer(3, 2);
  layer.to(pluss::pMPS);

  std::cout << "weight platform after .to(pMPS): " << static_cast<int>(layer->weight().platform()) << "\n";

  const Series x = pluss::make_series({1.0, 2.0, 3.0}, {1, 3}).to(pluss::ScalarType::Float32).to(pluss::pMPS);
  try {
    const Series y = layer.forward(x);
    std::cout << "fusion on MPS: OK, platform=" << static_cast<int>(y.platform()) << "\n";
  } catch (const std::exception& e) {
    std::cout << "fusion on MPS: FAILED -- " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
weight platform after .to(pMPS): 2
fusion on MPS: OK, platform=2
```

## Changelog

- **0.1** — the fully connected layer, `in_features`/`out_features`/`bias`/`cfg`/`seed`
  options.

## See Also

- [`FusionOptions`](FusionOptions.md) -- the options struct.
- [`Embedding`](../embedding/Embedding.md) -- another parameterized layer
  with the same uniform-bound initialization strategy.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

FusionOptions
```
<!-- apiref:toctree:end -->
