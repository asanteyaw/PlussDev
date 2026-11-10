<!-- generated from pack_vector.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::pack_vector`

Builds a parameter pack of named vector blocks.

## Signature

```cpp
optim::ParameterPack<pluss::Series> optim::pack_vector(std::initializer_list<std::pair<std::string, pluss::Series>> blocks);
```

## What It Does

Like [`pack_scalars`](pack_scalars.md) but each block keeps its length: a model with a weight vector `w` of 3 entries and a bias `b` of 1 is `pack_vector({{"w", w}, {"b", b}})`. Each value is flattened to a vector. Linear rows address the concatenation of all blocks in order, so the columns of a row are `w₀, w₁, w₂, b`.

## Parameters

`blocks`
: Name and value of each block, in order.

## Returns

A `ParameterPack<Series>`.

## Practical Examples

### Example 1

A vector block and a scalar-length block.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const Pack pack = optim::pack_vector({{"w", pluss::series::make({1.0, 2.0, 3.0})}, {"b", pluss::series::make({0.5})}});
  out.writeln("w =", pack.at("w").items<double>(), ", b =", pack.at("b").items<double>());
}
```

Output

```text
w = [1.0000, 2.0000, 3.0000] , b = [0.5000]
```

### Example 2

A matrix is flattened. A 2×2 value becomes a block of four entries.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const Pack pack = optim::pack_vector({{"m", pluss::series::make({{1.0, 2.0}, {3.0, 4.0}})}});
  out.writeln("sizes =", pack.at("m").sizes(), ", values =", pack.at("m").items<double>());
}
```

Output

```text
sizes = [4] , values = [1.0000, 2.0000, 3.0000, 4.0000]
```

### Example 3

Fitting a line: weights and an intercept as two blocks. Least squares through `(0,1), (1,3), (2,5)`: slope 2, intercept 1.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series xs = pluss::series::make({0.0, 1.0, 2.0});
  const pluss::Series ys = pluss::series::make({1.0, 3.0, 5.0});
  const Pack start = optim::pack_vector({{"w", pluss::series::make({0.0})}, {"b", pluss::series::make({0.0})}});
  const optim::OptimaState result = optim::optima([&](const Pack &p) { return (p.at("w") * xs + p.at("b") - ys).square().sum(); }, start);
  out.writeln("w =", result.params.at("w").items<double>(), " b =", result.params.at("b").items<double>());
}
```

Output

```text
w = [2.0000]  b = [1.0000]
```

## See Also

- [`pack_scalars`](pack_scalars.md): scalar blocks
- [`pack_series`](pack_series.md): a single block

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pack_series.html">
    <span class="label">Previous</span>
    <span class="title">pack_series</span>
  </a>
  <a class="next-link" href="presets_internal_protocol.html">
    <span class="label">Next</span>
    <span class="title">internal_protocol</span>
  </a>
</div>
<!-- apiref:nav:end -->
