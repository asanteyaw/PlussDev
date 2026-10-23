<!-- pluss-hand-authored -->

# Series::CalcAccessor::options

[Back to Series::CalcAccessor API Reference](../../series_calc_accessor_api_reference.md)

## Signature

```cpp
const Series::CalcAccessor::CalcOptions&
Series::CalcAccessor::options() const noexcept
```

## What It Returns

`options()` returns a read-only reference to the policy currently bound to a
calculus accessor. The root member `x.calc` starts with default options;
`x.calc(custom)` returns a lightweight accessor containing a copy of `custom`.
Inspecting the copy is useful when a configured accessor is passed through
generic code and the eventual terminal operation needs to report or branch on
its policy.

`options()` does not validate a policy by itself. Validation belongs to the
terminal method because fields have operation-specific meaning. For example,
`order=0` is valid for `diff()` but invalid for `antidiff()`, and a missing
`lower` matters only to the no-bound `antidiff(fn)` overload.

## CalcOptions Fields

| Field | Used by | Meaning | Default |
| --- | --- | --- | --- |
| `order` | `diff`, `antidiff` | Derivative order, or number of repeated integrations | `1` |
| `elementwise` | `diff` | Return lane-wise diagonal derivatives with the receiver's shape | `false` |
| `strict_dependency` | `diff`, `grad`, `jac`, `hess` | Fail when the result is disconnected from the independent value | `false` |
| `lower` | `antidiff(fn)` | Lower bound used when no explicit bound is passed | unset |
| `initial` | `antidiff` | Lower-bound values `F^(k)(lower)`, one per integration order | empty, meaning all zero |
| `rule` | `antidiff` | Fixed/refined quadrature rule | `Rule::Simpson` |
| `intervals` | `antidiff` | Initial number of quadrature intervals | `256` |
| `adaptive` | `antidiff` | Refine by doubling intervals | `true` |
| `atol` | `antidiff` | Absolute convergence tolerance | `1e-10` |
| `rtol` | `antidiff` | Relative convergence tolerance | `1e-8` |
| `max_refinements` | `antidiff` | Maximum interval doublings | `8` |
| `require_convergence` | `antidiff` | Throw rather than return the last unconverged estimate | `true` |

`AntidiffOptions` is a type alias for `CalcOptions`; it does not define a
second policy object.

## Precedence

Bound options provide defaults. A terminal argument replaces the corresponding
field for that call:

- `configured.diff(fn, n)` uses `n`, not `configured.options().order`;
- `configured.antidiff(fn, a)` uses `a`, not
  `configured.options().lower`;
- `configured.antidiff(fn, a, other)` uses `other` for the entire call.

The original configured accessor is unchanged by all three forms. This makes a
configured accessor safe to reuse across calls with occasional overrides.

## Examples

Example 1. Bind and inspect an integration policy.

```cpp
pluss::Series::CalcAccessor::CalcOptions options;
options.lower = 0.0;
options.intervals = 32;
options.adaptive = false;

auto x = pluss::make_series({0.5, 1.0, 1.5, 2.0});
auto configured = x.calc(options);
const auto& policy = configured.options();

std::cout << "lower=" << *policy.lower << "\n";
std::cout << "intervals=" << policy.intervals << "\n";
std::cout << "adaptive=" << std::boolalpha << policy.adaptive << "\n";
```

### Output

```text
lower=0
intervals=32
adaptive=false
```

Example 2. The bound lower endpoint is used when omitted; an explicit endpoint
overrides it without changing the stored policy.

```cpp
pluss::Series::CalcAccessor::CalcOptions options;
options.lower = 0.0;
options.intervals = 32;
options.adaptive = false;

auto x = pluss::make_series({0.5, 1.0, 1.5, 2.0});
auto configured = x.calc(options);
auto square = [](const pluss::Series& t) { return t.square(); };

std::cout << "bound=" << configured.antidiff(square) << "\n";
std::cout << "override=" << configured.antidiff(square, 1.0) << "\n";
std::cout << "still_bound_to=" << *configured.options().lower << "\n";
```

### Output

```text
bound=pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.041666667, 0.33333333, 1.125, 2.6666667]
override=pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[-0.29166667, 0, 0.79166667, 2.3333333]
still_bound_to=0
```

Example 3. A bound derivative order is the default, while an explicit order
changes one call only.

```cpp
pluss::Series::CalcAccessor::CalcOptions options;
options.order = 2;

auto x = pluss::make_series(0.5);
auto configured = x.calc(options);
auto sine = [](const pluss::Series& z) { return z.sin(); };

std::cout << std::setprecision(10);
std::cout << "bound_second=" << configured.diff(sine).item<double>() << "\n";
std::cout << "explicit_first=" << configured.diff(sine, 1).item<double>() << "\n";
std::cout << "stored_order=" << configured.options().order << "\n";
```

### Output

```text
bound_second=-0.4794255386
explicit_first=0.8775825619
stored_order=2
```

Example 4. One policy can define a second antiderivative completely, including
its anchor and both lower-bound conditions.

```cpp
pluss::Series::CalcAccessor::CalcOptions options;
options.order = 2;
options.lower = 0.0;
options.initial = {2.0, 3.0};
options.adaptive = false;
options.intervals = 32;

auto upper = pluss::make_series({0.5, 1.0, 2.0});
auto configured = upper.calc(options);
auto result = configured.antidiff(
    [](const pluss::Series& t) { return t.mul(0.0).add(1.0); });

std::cout << result << "\n";
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[3.625, 5.5, 10]
```

## Ownership And Lifetime

The configured accessor owns its option copy but only borrows the receiver.
Mutating the original `CalcOptions` after `x.calc(options)` does not alter the
configured accessor. The reference returned by `options()` remains valid only
as long as that accessor object remains alive; do not retain a reference taken
from a temporary such as `x.calc(options).options()`.

## Failure Modes

`options()` itself is `noexcept`. Invalid values are diagnosed when a terminal
method consumes them. See [`diff`](diff.md) for derivative validation and
[`antidiff`](antidiff.md) for integration validation.

## See Also

- [`Series::calc`](../series/calc.md): accessor construction and call styles.
- [`Series::CalcAccessor::diff`](diff.md): derivative-specific fields.
- [`Series::CalcAccessor::antidiff`](antidiff.md): integration-specific fields.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="jac.html"><span class="label">Previous</span><span class="title">jac</span></a>
  <a class="next-link" href="../../series_api_reference.html"><span class="label">Next</span><span class="title">Series</span></a>
</div>
