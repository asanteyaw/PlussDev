<!-- generated from adam_step_inplace.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::adam_step_inplace`

Applies one Adam update to a list of parameters, in place, given their gradients.

## Signature

```cpp
void optim::adam_step_inplace(std::vector<pluss::Series> &params, const std::vector<pluss::Series> &sensitivities, optim::AdamState &state, const optim::AdamOptions &options = {});
```

## What It Does

Use it to run Adam in your own loop, for example inside a training procedure where you compute gradients yourself. `sensitivities` holds one gradient per parameter block (same shape); `state` is updated and its step count incremented; `params` are replaced by the stepped values. The update is `p ← p − learning_rate · m̂ / (√v̂ + eps)` with bias-corrected moments. Weight decay, AMSGrad and maximisation follow [`AdamOptions`](AdamOptions.md). Complex parameters are stepped with independent real and imaginary channels. Parameters must have a floating or complex dtype; mismatched sizes are an error.

## Parameters

`params`
: Parameter blocks; replaced by the stepped values.

`sensitivities`
: Gradients, one per block.

`state`
: The running state; updated.

`options`
: Adam settings.

## Returns

Nothing; `params` and `state` are modified.

## Errors

- `optim::adam_step_inplace: params/sensitivities size mismatch` when the lists differ in length.

## Practical Examples

### Example 1

Three steps on `f = Σ x²`, whose gradient is `2x`. Each step moves every coordinate by about the learning rate.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <vector>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  std::vector<Series> params{pluss::series::make({1.0, -2.0})};
  optim::AdamState state = optim::adam_state_like(params);
  for (int i = 0; i < 3; ++i) {
    optim::adam_step_inplace(params, {2.0 * params[0]}, state, {.learning_rate = 0.1});
    out.writeln("step", state.step, ": x =", params[0].items<double>());
  }
}
```

Output

```text
step 1 : x = [0.9000, -1.9000]
step 2 : x = [0.8004, -1.8002]
step 3 : x = [0.7016, -1.7006]
```

### Example 2

Your own training loop: run until the gradient is small.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <vector>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  std::vector<Series> params{pluss::series::make({0.0, 0.0})};
  const Series target = pluss::series::make({3.0, -1.0});
  optim::AdamState state;
  optim::AdamOptions options;
  options.learning_rate = 0.2;
  int steps = 0;
  while (steps < 2000 && (2.0 * (params[0] - target)).abs().max().item<double>() > 1e-3) {
    optim::adam_step_inplace(params, {2.0 * (params[0] - target)}, state, options);
    ++steps;
  }
  out.writeln("steps =", steps, ", x =", params[0].items<double>());
}
```

Output

```text
steps = 142 , x = [3.000, -1.000]
```

### Example 3

Several blocks and weight decay. The state is created automatically when empty.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <vector>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  std::vector<Series> params{pluss::series::make({1.0}), pluss::series::make({2.0})};
  optim::AdamState state;
  optim::AdamOptions options;
  options.learning_rate = 0.1;
  options.weight_decay = 0.1;
  optim::adam_step_inplace(params, {params[0], params[1]}, state, options);
  out.writeln("a =", params[0].items<double>(), " b =", params[1].items<double>(), ", blocks in state =", state.m.size());
}
```

Output

```text
a = [0.9000]  b = [1.9000] , blocks in state = 2
```

### Example 4

A size mismatch is an error.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>
#include <vector>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto first_line = [](const std::exception &e) {
    const std::string text(e.what());
    return text.substr(0, text.find('\n'));
  };
  std::vector<Series> params{pluss::series::make({1.0, 2.0})};
  optim::AdamState state;
  try {
    optim::adam_step_inplace(params, {}, state);
  } catch (const std::exception &e) {
    out.writeln("error =", first_line(e));
  }
}
```

Output

```text
error = optim::adam_step_inplace: params/sensitivities size mismatch
```

## See Also

- [`adam_step`](adam_step.md): returns new parameters instead
- [`AdamState`](AdamState.md): the state
- [`AdamOptions`](AdamOptions.md): the settings

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="adam_step.html">
    <span class="label">Previous</span>
    <span class="title">adam_step</span>
  </a>
  <a class="next-link" href="l_sr1_trust_region.html">
    <span class="label">Next</span>
    <span class="title">l_sr1_trust_region</span>
  </a>
</div>
<!-- apiref:nav:end -->
