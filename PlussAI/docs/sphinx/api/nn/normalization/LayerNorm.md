# `pluss::ai::nn::LayerNorm`

Normalizes each row to zero mean / unit variance over its last dimension,
then applies a learned per-feature scale and shift: `(x - mean) / sqrt(var +
eps) * weight + bias`. Unlike [`PReLU`](../activation/PReLU.md), which has
one kind of learnable state (a slope), `LayerNorm` has two (`weight` and
`bias`), both shaped `[normalized_size]` and both trained alongside the
rest of the model. Runs through a single fused reverse-mode node with an
analytic gradient rather than the underlying mean/sub/var/rsqrt/mul/mul/add
chain, so it is significantly cheaper to differentiate than composing those
operations by hand would be.

## Signature

```cpp
explicit nn::LayerNorm::LayerNorm(nn::LayerNormOptions options);
```

## Construction

`LayerNorm` has exactly one constructor, and it requires a
[`LayerNormOptions`](LayerNormOptions.md) -- there is no no-argument
overload, because `normalized_size` has no meaningful default (`weight`
and `bias` cannot be allocated without it). At minimum, set
`normalized_size` to the width of the axis being normalized:

```cpp
nn::LayerNormOptions opts{};
opts.normalized_size = 128;
nn::LayerNorm ln(opts);
```

## State

`weight()` (mutable and `const` overloads) -- `Series&` / `const Series&`
: The learned per-feature scale, shape `[normalized_size]`, initialized to
  all-ones. Registered as a parameter.

`bias()` (mutable and `const` overloads) -- `Series&` / `const Series&`
: The learned per-feature shift, shape `[normalized_size]`, initialized to
  all-zeros. Registered as a parameter.

`options()` (mutable and `const` overloads) returns the
[`LayerNormOptions`](LayerNormOptions.md) this instance was constructed
with. `options().cfg`'s `platform`/`dtype` fields stay in sync
automatically whenever `.to(...)` is called on the module -- they are not
just the value passed at construction time.

## Methods

`forward(input)` / `operator()(input)` -- `Series`
: Returns `input.layer_norm({normalized_size}, weight(), bias(), eps)`.
  Throws if `input`'s last axis does not match `normalized_size` (the
  `weight`/`bias` broadcast fails).

## Practical Examples

Three examples: the normalization property itself (each row lands at zero
mean / unit variance before the affine step), confirming the learned
`weight`/`bias` behave exactly like a scale-and-shift applied after plain
normalization, and confirming `LayerNorm` runs on the MPS backend with
`options().cfg` tracking the move.

### Example 1

With the default `weight=1`/`bias=0`, `forward`'s output *is* the plain
normalized row -- each row's mean lands at (numerically) `0` and its
(biased) variance at (numerically) `1`, up to `eps`'s small effect.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::LayerNormOptions opts{};
  opts.normalized_size = 3;
  nn::LayerNorm ln(opts);  // eps=1e-5 (default), weight=ones, bias=zeros

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 100.0, 200.0, 300.0}, {2, 3}, f64);
  const pluss::Series y = ln.forward(x);

  std::cout << "x = " << x << "\n";
  std::cout << "layer_norm(x) = " << y << "\n";
  std::cout << "per-row mean: " << y.mean(-1, false) << "\n";
  std::cout << "per-row var (biased): " << y.var(-1, /*unbiased=*/false, false) << "\n";

  return 0;
}
```

Output

```text
x = pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 2, 3],
  [100, 200, 300]]
layer_norm(x) = pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-1.2247357, 0, 1.2247357],
  [-1.2247449, 0, 1.2247449]]
per-row mean: pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0, 0]
per-row var (biased): pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.999985, 1]
```

Note both rows normalize to the *same* output despite wildly different raw
scales (`[1,2,3]` vs `[100,200,300]`) -- that scale-invariance is the whole
point of the operation. The first row's variance prints as `0.999985`
rather than an exact `1` because `eps` is added before the reciprocal
square root; at the second row's larger magnitude the effect rounds away
in the printed output.

### Example 2

`weight`/`bias` apply after normalization as an ordinary scale-and-shift --
setting `weight=2`, `bias=1` produces exactly the same result as normalizing
with the defaults and then computing `* 2 + 1` by hand.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Series;

  nn::LayerNormOptions opts{};
  opts.normalized_size = 3;
  nn::LayerNorm ln(opts);

  ln->weight().copy_(pluss::make_series({2.0, 2.0, 2.0}, {3}));
  ln->bias().copy_(pluss::make_series({1.0, 1.0, 1.0}, {3}));

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const Series x = pluss::make_series({1.0, 2.0, 3.0}, {1, 3}, f64);
  const Series y = ln.forward(x);

  // hand formula: normalize with weight=1/bias=0, then scale by 2 and shift by 1
  nn::LayerNormOptions plain_opts{};
  plain_opts.normalized_size = 3;
  nn::LayerNorm plain(plain_opts);
  const Series by_hand = plain.forward(x).mul(2.0).add(1.0);

  std::cout << "weight: " << ln->weight() << "\n";
  std::cout << "bias:   " << ln->bias() << "\n";
  std::cout << "layer_norm(x) with weight=2, bias=1: " << y << "\n";
  std::cout << "plain_normalize(x) * 2 + 1:          " << by_hand << "\n";
  std::cout << "same values: " << std::boolalpha << y.allclose(by_hand) << "\n";

  return 0;
}
```

Output

```text
weight: pluss::Series(shape=[3], strides=[1], dtype=float32, platform=cpu, numel=3, contiguous=true)
data=[2, 2, 2]
bias:   pluss::Series(shape=[3], strides=[1], dtype=float32, platform=cpu, numel=3, contiguous=true)
data=[1, 1, 1]
layer_norm(x) with weight=2, bias=1: pluss::Series(shape=[1,3], strides=[3,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[-1.4494714, 1, 3.4494714]]
plain_normalize(x) * 2 + 1:          pluss::Series(shape=[1,3], strides=[3,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[-1.4494714, 1, 3.4494714]]
same values: true
```

### Example 3

`LayerNorm` runs on the MPS backend the same way it does on CPU -- moving
the module with `.to(pMPS)` moves both `weight` and `bias`, and
`options().cfg.platform` updates to match automatically.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Series;
  using pluss::ScalarType;

  nn::LayerNormOptions opts{};
  opts.normalized_size = 4;
  nn::LayerNorm ln(opts);
  ln.to(pluss::pMPS);

  std::cout << "weight platform after .to(pMPS): " << static_cast<int>(ln->weight().platform()) << "\n";
  std::cout << "options().cfg.platform tracks it: " << static_cast<int>(ln->options().cfg.platform) << "\n";

  const Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {1, 4}).to(ScalarType::Float32).to(pluss::pMPS);
  try {
    const Series y = ln.forward(x);
    std::cout << "layer_norm on MPS: OK, platform=" << static_cast<int>(y.platform()) << "\n";
  } catch (const std::exception& e) {
    std::cout << "layer_norm on MPS: FAILED -- " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
weight platform after .to(pMPS): 2
options().cfg.platform tracks it: 2
layer_norm on MPS: OK, platform=2
```

## Changelog

- **0.1** — layer normalization over the last dimension, `normalized_size`/
  `eps`/`cfg` options.

## See Also

- [`LayerNormOptions`](LayerNormOptions.md) -- the options struct.
- [`PReLU`](../activation/PReLU.md) -- the other parameterized layer
  documented so far, for comparison of how learnable state is exposed.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

LayerNormOptions
```
<!-- apiref:toctree:end -->
