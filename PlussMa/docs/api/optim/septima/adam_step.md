<!-- generated from adam_step.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::adam_step`

Applies one Adam update and returns the new parameters, leaving the inputs unchanged.

## Signature

```cpp
std::vector<pluss::Series> optim::adam_step(std::vector<pluss::Series> params, const std::vector<pluss::Series> &sensitivities, optim::AdamState &state, const optim::AdamOptions &options = {});
```

## What It Does

The same update as [`adam_step_inplace`](adam_step_inplace.md), but `params` is taken by value and the stepped copy is returned. The `state` is still updated in place.

## Parameters

`params`
: Parameter blocks (by value).

`sensitivities`
: Gradients.

`state`
: The running state; updated.

`options`
: Adam settings.

## Returns

The stepped parameters.

## Practical Examples

### Example 1

One step.

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
  optim::AdamState state;
  const std::vector<Series> next = optim::adam_step({pluss::series::make({1.0, -2.0})}, {pluss::series::make({2.0, -4.0})}, state, {.learning_rate = 0.1});
  out.writeln("next =", next[0].items<double>(), ", step =", state.step);
}
```

Output

```text
next = [0.9000, -1.9000] , step = 1
```

### Example 2

A loop that keeps the old and new parameters.

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
  std::vector<Series> params{pluss::series::make({5.0})};
  optim::AdamState state;
  for (int i = 0; i < 3; ++i) {
    const std::vector<Series> next = optim::adam_step(params, {2.0 * params[0]}, state, {.learning_rate = 0.5});
    out.writeln("from", params[0].items<double>(), "to", next[0].items<double>());
    params = next;
  }
}
```

Output

```text
from [5.0000] to [4.5000]
from [4.5000] to [4.0021]
from [4.0021] to [3.5079]
```

### Example 3

Maximising. With `maximize = true` the step climbs the gradient.

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
  optim::AdamState state;
  const std::vector<Series> next = optim::adam_step({pluss::series::make({1.0})}, {pluss::series::make({1.0})}, state, {.learning_rate = 0.1, .maximize = true});
  out.writeln("next =", next[0].items<double>());
}
```

Output

```text
next = [1.1000]
```

## See Also

- [`adam_step_inplace`](adam_step_inplace.md): in place
- [`AdamOptions`](AdamOptions.md): the settings

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="adam_state_like.html">
    <span class="label">Previous</span>
    <span class="title">adam_state_like</span>
  </a>
  <a class="next-link" href="adam_step_inplace.html">
    <span class="label">Next</span>
    <span class="title">adam_step_inplace</span>
  </a>
</div>
<!-- apiref:nav:end -->
