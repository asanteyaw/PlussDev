<!-- generated from EdgeMode.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `interp::EdgeMode`

What `interp::linear` does for a query outside the coordinate grid.

`Clamp` (the default) returns the value at the nearest end of the grid: the interpolant is constant beyond the samples. `Linear` extends the first and last segments along their straight lines, which extrapolates a trend. `Error` throws `std::out_of_range` if any query lies outside the grid, for callers who want an out-of-range query to be a mistake rather than a silent answer.

## Fields

### `Clamp`

`enumerator`, default `0`.

Beyond the grid, hold the end value.

### `Linear`

`enumerator`, default `1`.

Beyond the grid, extend the end segments.

### `Error`

`enumerator`, default `2`.

Beyond the grid, throw `std::out_of_range`.

## Guidance

Set as the last argument of `interp::linear`.

## Practical Examples

### Example 1

The three modes on a query below and above the grid. The samples `0, 10, 40` at `0, 1, 2`: clamping holds the ends, linear extension follows the slopes 10 and 30.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace interp = pluss::subsp::math::interp;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const Series x = pluss::make_series({0.0, 1.0, 2.0});
  const Series y = pluss::make_series({0.0, 10.0, 40.0});
  const Series q = pluss::make_series({-1.0, 3.0});
  out.writeln("Clamp  :", interp::linear(x, y, q, interp::EdgeMode::Clamp).items<double>());
  out.writeln("Linear :", interp::linear(x, y, q, interp::EdgeMode::Linear).items<double>());
}
```

Output

```text
Clamp  : [0.0000, 40.0000]
Linear : [-10.0000, 70.0000]
```

### Example 2

Inside the grid all three modes agree.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace interp = pluss::subsp::math::interp;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const Series x = pluss::make_series({0.0, 1.0, 2.0});
  const Series y = pluss::make_series({0.0, 10.0, 40.0});
  const Series q = pluss::make_series({0.5, 1.5});
  for (const interp::EdgeMode mode : {interp::EdgeMode::Clamp, interp::EdgeMode::Linear, interp::EdgeMode::Error})
    out.writeln(static_cast<int>(mode), ":", interp::linear(x, y, q, mode).items<double>());
}
```

Output

```text
0 : [5.0000, 25.0000]
1 : [5.0000, 25.0000]
2 : [5.0000, 25.0000]
```

### Example 3

`Error` as a guard. A query outside the grid is reported rather than answered.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

using namespace pluss::tils;

namespace interp = pluss::subsp::math::interp;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const Series x = pluss::make_series({0.0, 1.0, 2.0});
  const Series y = pluss::make_series({0.0, 10.0, 40.0});
  try {
    (void)interp::linear(x, y, pluss::make_series({2.5}), interp::EdgeMode::Error);
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
math::interp::linear: query lies outside the coordinate domain
```

## See Also

- [`linear`](linear.md): uses it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="linear.html">
    <span class="label">Next</span>
    <span class="title">linear</span>
  </a>
</div>
<!-- apiref:nav:end -->
