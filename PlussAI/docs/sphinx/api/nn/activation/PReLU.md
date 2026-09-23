# `pluss::ai::nn::PReLU`

The Parametric ReLU, applied elementwise: `x > 0 ? x : weight * x`. Unlike
every other activation in this family, `PReLU`'s slope on the negative side
is a **learnable parameter**, not a fixed constant -- it starts at `init`
(`0.25` by default) and is trained alongside the rest of the model like any
other module's weight. When `weight` holds more than one entry, it
broadcasts against the input the same way any other `Series` binary op
does, which is where `PReLU` has a real, narrower-than some other
frameworks' behavior worth knowing about -- see Example 2/3.

## Signature

```cpp
explicit nn::PReLU::PReLU(nn::PReLUOptions options);
explicit nn::PReLU::PReLU(std::int64_t num_parameters = 1, double init = 0.25, pluss::Config cfg = ::pluss::detail::nn::default_cfg_());

void nn::PReLU::reset_parameters();
```

## Construction

Three equivalent ways to construct: default-construct (`num_parameters=1,
init=0.25`), pass the three fields positionally, or pass a
[`PReLUOptions`](PReLUOptions.md) directly. All three route through
`reset_parameters()`, which fills `weight` with `init`.

## State

`weight()` (mutable and `const` overloads) -- `Series&` / `const Series&`
: The learnable slope, shape `[num_parameters]`. Registered as a parameter,
  so it shows up in `parameters()` / `named_parameters()` and moves with
  the module under `.to(...)`.

`options()` (mutable and `const` overloads) returns the
[`PReLUOptions`](PReLUOptions.md) this instance was constructed with.

## Methods

`forward(input)` / `operator()(input)` -- `Series`
: Returns `input.prelu(weight())`. Throws `pluss::Error` ("broadcast:
  incompatible shapes") if `weight`'s shape does not broadcast against
  `input` -- see the per-channel caveat below.

`reset_parameters()` -- `void`
: Refills `weight()` with `options().init`, in place. Called once during
  construction; call it again to reinitialize an already-trained instance.

`weight()` -- `Series&` (or `const Series&`)
: The learnable per-channel slope.

`options()` -- `PReLUOptions&` (or `const PReLUOptions&`)
: The options this instance holds.

## Practical Examples

Four examples: the default single shared slope, a per-channel slope that
broadcasts correctly because the channel axis is last, the same per-channel
slope failing against a conventional `[N, C, L]` layout where the channel
axis is *not* last, and confirming `PReLU` runs on the MPS backend with
values matching CPU exactly.

### Example 1

With the default `num_parameters=1`, `weight` is a single learned scalar
shared across every element -- `PReLU` also reports one trainable
parameter, unlike every stateless activation earlier in this family.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::PReLU prelu;  // num_parameters=1, init=0.25 (defaults)

  std::cout << "weight: " << prelu->weight() << "\n";
  std::cout << "parameters().size(): " << prelu.parameters().size()
            << " -- unlike the other activations, PReLU is trainable\n";

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series x = pluss::make_series({-4.0}, {1}, f64);
  std::cout << "prelu(-4) with default init=0.25: " << prelu.forward(x) << "\n";

  return 0;
}
```

Output

```text
weight: pluss::Series(shape=[1], strides=[1], dtype=float32, platform=cpu, numel=1, contiguous=true)
data=[0.25]
parameters().size(): 1 -- unlike the other activations, PReLU is trainable
prelu(-4) with default init=0.25: pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[-1]
```

### Example 2

With `num_parameters` set to the channel count and the channel axis placed
*last* (shape `[samples, channels]`), `weight`'s `[channels]` shape
broadcasts against the input's trailing axis correctly -- each column gets
its own slope.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Series;

  // channel dimension last: shape [2, 3] (2 samples, 3 channels)
  const Series x = pluss::make_series({-1.0, -2.0, -3.0, 1.0, 2.0, 3.0}, {2, 3});

  nn::PReLUOptions opts{};
  opts.num_parameters = 3;
  nn::PReLU prelu(opts);
  prelu->weight().copy_(pluss::make_series({0.1, 0.2, 0.3}, {3}));

  std::cout << "weight: " << prelu->weight() << "\n";
  std::cout << "prelu(x): " << prelu.forward(x) << "\n";

  return 0;
}
```

Output

```text
weight: pluss::Series(shape=[3], strides=[1], dtype=float32, platform=cpu, numel=3, contiguous=true)
data=[0.1, 0.2, 0.30000001]
prelu(x): pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-0.1, -0.40000001, -0.90000004],
  [1, 2, 3]]
```

(`0.30000001` is ordinary Float32 rounding of the literal `0.3` -- `weight`
was constructed at the default Float32, not Float64.)

### Example 3

The same per-channel `weight` against a conventional `[N, C, L]` layout
(channel axis in the *middle*, not last) throws. `PReLU`'s `forward` does
plain trailing-axis broadcasting between `weight` and `input`, the same
rule every other `Series` binary op follows -- it does not special-case the
channel axis the way some other frameworks' NCHW-aware PReLU does. Keep the
channel axis last, or reshape/transpose before calling `forward`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Series;

  // 1 sample, 3 channels, 2 spatial positions -- shape [1, 3, 2]
  const Series x = pluss::make_series(
      {-1.0, 1.0, -2.0, 2.0, -3.0, 3.0}, {1, 3, 2});

  nn::PReLUOptions opts{};
  opts.num_parameters = 3;
  nn::PReLU prelu(opts);
  prelu->weight().copy_(pluss::make_series({0.1, 0.2, 0.3}, {3}));

  std::cout << "weight: " << prelu->weight() << "\n";
  std::cout << "prelu(x): " << prelu.forward(x) << "\n";

  return 0;
}
```

Output

```text
libc++abi: terminating due to uncaught exception of type pluss::Error: broadcast: incompatible shapes
weight: pluss::Series(shape=[3], strides=[1], dtype=float32, platform=cpu, numel=3, contiguous=true)
data=[0.1, 0.2, 0.30000001]
prelu(x):
```

(the abort message lands first because it goes to unbuffered `stderr`,
while the `weight` line -- already written before the crash -- sits in
`stdout`'s buffer and only flushes afterward; `prelu(x): ` is the literal
string constant from the second `std::cout` statement, printed before its
next operand, `prelu.forward(x)`, throws mid-evaluation. The process
aborts on the uncaught exception; in real code this would be caught with a
`try`/`catch`, as the MPS-check examples throughout this family do.)

### Example 4

`PReLU` runs on the MPS backend and produces the same values as CPU, to
within floating-point tolerance -- move both the input and the module (so
`weight` moves too) to `pMPS`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Series;
  using pluss::ScalarType;

  const Series x_cpu = pluss::make_series({-2.0, -0.5, 0.0, 0.5, 2.0}, {5}).to(ScalarType::Float32);
  const Series x_mps = x_cpu.to(pluss::pMPS);

  nn::PReLU p_cpu;
  nn::PReLU p_mps;
  p_mps.to(pluss::pMPS);

  const Series y_cpu = p_cpu.forward(x_cpu);
  const Series y_mps = p_mps.forward(x_mps).to(pluss::pCPU);

  std::cout << "prelu CPU: " << y_cpu << "\n";
  std::cout << "prelu MPS (moved back to CPU to print): " << y_mps << "\n";
  std::cout << "match: " << std::boolalpha << y_cpu.allclose(y_mps) << "\n";

  return 0;
}
```

Output

```text
prelu CPU: pluss::Series(shape=[5], strides=[1], dtype=float32, platform=cpu, numel=5, contiguous=true)
data=[-0.5, -0.125, 0, 0.5, 2]
prelu MPS (moved back to CPU to print): pluss::Series(shape=[5], strides=[1], dtype=float32, platform=cpu, numel=5, contiguous=true)
data=[-0.5, -0.125, 0, 0.5, 2]
match: true
```

This exercises a real fix made alongside this documentation: `PReLU`'s
underlying kernel used to unconditionally promote to Float64 before
computing, which crashes on MPS (Float64 is entirely unsupported on that
backend). It now uses the same `compat_real_dtype_for_` dtype selection
every other composite activation already relies on, so `PReLU` works on
MPS like the rest of this family.

## Changelog

- **0.1** — the Parametric ReLU, `num_parameters`/`init`/`cfg` options.

## See Also

- [`PReLUOptions`](PReLUOptions.md) -- the options struct.
- [`LeakyReLU`](LeakyReLU.md) -- the same piecewise-linear shape with a
  fixed, non-learnable negative slope instead.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

PReLUOptions
```
<!-- apiref:toctree:end -->
