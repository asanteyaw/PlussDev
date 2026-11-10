<!-- generated from HjmPaths.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `rates::HjmPaths`

Simulated forward curves, the short rate and the normals of an HJM simulation.

`forward` is `[steps + 1, paths, tenors]`: the curve of each path at each date, on the tenor grid. `rate` is `[steps + 1, paths]`: the short rate, the forward at zero time to maturity. `innovation` is `[steps, paths]`: the shared standard normals. With `paths_first` the first two axes of `rate` and `forward` swap.

## Fields

### `rate`

`pluss::Series`, default `undefined`.

`[steps + 1, paths]`: the short rate.

### `forward`

`pluss::Series`, default `undefined`.

`[steps + 1, paths, tenors]`: the forward curve.

### `innovation`

`pluss::Series`, default `undefined`.

`[steps, paths]`: the shared normals.

## Guidance

Returned by `hjm_paths`.

## Practical Examples

### Example 1

The shapes.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace rt = fin::rates;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const rt::HjmPaths sim = rt::hjm_paths(12, 5, 0.05, rt::HjmParams{});
  out.writeln("rate", sim.rate.sizes(), " forward", sim.forward.sizes(), " innovation", sim.innovation.sizes());
}
```

Output

```text
rate [13, 5]  forward [13, 5, 6]  innovation [12, 5]
```

### Example 2

A curve on one path at one date.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace rt = fin::rates;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const rt::HjmPaths sim = rt::hjm_paths(52, 3, 1.0 / 52.0, rt::HjmParams{});
  out.writeln("curve of path 0 after a year:", sim.forward.select(0, 52).select(0, 0).items<double>());
}
```

Output

```text
curve of path 0 after a year: [0.0237, 0.0241, 0.0247, 0.0253, 0.0258, 0.0264]
```

### Example 3

The short rate starts at `r0`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace rt = fin::rates;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const rt::HjmPaths sim = rt::hjm_paths(12, 5, 0.05, rt::HjmParams{});
  out.writeln(sim.rate.select(0, 0).items<double>());
}
```

Output

```text
[0.0300, 0.0300, 0.0300, 0.0300, 0.0300]
```

## See Also

- [`hjm_paths`](hjm_paths.md): produces it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="HjmParams.html">
    <span class="label">Previous</span>
    <span class="title">HjmParams</span>
  </a>
  <a class="next-link" href="HullWhiteParams.html">
    <span class="label">Next</span>
    <span class="title">HullWhiteParams</span>
  </a>
</div>
<!-- apiref:nav:end -->
