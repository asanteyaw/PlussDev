<!-- pluss-hand-authored -->

# Series::calc

[Back to Series API Reference](../../series_api_reference.md)

## Signatures

```cpp
Series::CalcAccessor Series::calc
const Series::CalcAccessor& Series::calc() const noexcept
Series::CalcAccessor Series::calc(const Series::CalcAccessor::CalcOptions& options) const
```

## What It Does

`calc` is the entrypoint for calculus on a callable. The owning `Series` has a
different role in the two directions:

- for `diff()`, `grad()`, `jac()`, and `hess()`, it is the independent value at
  which the callable is differentiated;
- for `antidiff()`, it is the upper endpoint of an anchored integral.

Accessing `calc` does not evaluate the callback, record a persistent graph, or
copy the receiver. A terminal method selects the operation and invokes the
callback with a temporary differentiable or quadrature-node `Series`.

Open the dedicated [Series::CalcAccessor API Reference](../../series_calc_accessor_api_reference.md)
for the complete method family, tensor-shape conventions, and option table.

## Why This Is Separate

`Series` already has two similarly named sequence operations, but neither takes
a callable:

- [`Series::diff()`](diff.md) subtracts adjacent stored samples.
- [`Series::antidiff`](antidiff.md) integrates stored samples with trapezoids.

`calc` instead asks a function for values as needed. `x.calc.diff(fn)`
differentiates the expression built by `fn(x)`, while
`upper.calc.antidiff(fn, lower)` evaluates `fn` at quadrature nodes between two
endpoints. The separation prevents a sampled sequence from being mistaken for
the function that generated it.

## Calling Styles

The member and zero-argument call forms are equivalent:

```cpp
auto dot = x.calc.diff(fn);
auto call = x.calc().diff(fn);
```

Passing `CalcOptions` creates a lightweight accessor with those defaults bound:

```cpp
Series::CalcAccessor::CalcOptions options;
options.order = 2;

auto second = x.calc(options);
auto value = second.diff(fn);
```

The receiver and its numeric storage are not copied. The returned accessor
keeps the receiver binding plus its own option record, so separate configured
accessors may be used independently.

## Examples

Example 1. Dot and call syntax select the same first derivative.

```cpp
auto x = pluss::make_series(0.5);
auto fn = [](const pluss::Series& z) { return z.sin(); };

std::cout << x.calc.diff(fn) << "\n";
std::cout << x.calc().diff(fn) << "\n";
```

### Output

```text
data=0.87758256
data=0.87758256
```

Example 2. Bound options organize several calls around one calculus policy.

```cpp
pluss::Series::CalcAccessor::CalcOptions options;
options.order = 2;

auto x = pluss::make_series(0.5);
auto configured = x.calc(options);

std::cout << configured.diff(
    [](const pluss::Series& z) { return z.sin(); }) << "\n";
std::cout << "order=" << configured.options().order << "\n";
```

### Output

```text
data=-0.47942554
order=2
```

Example 3. The same entrypoint makes the receiver an integration endpoint.

```cpp
auto upper = pluss::make_series({0.5, 1.0, 2.0});
auto area = upper.calc.antidiff(
    [](const pluss::Series& t) { return t.square(); }, 0.0);

std::cout << area << "\n";
```

### Output

```text
data=[0.041666667, 0.33333333, 2.6666667]
```

Each endpoint is independent: the result is
`[integral_0^0.5 t^2 dt, integral_0^1 t^2 dt, integral_0^2 t^2 dt]` and keeps
the receiver's shape.

## Lifetime And Mutation

The accessor is cheap, but it remains bound to its receiver. Do not retain an
accessor beyond the lifetime of the `Series` that created it. Terminal methods
return new `Series` values and do not mutate the receiver.

## See Also

- [Series::CalcAccessor API Reference](../../series_calc_accessor_api_reference.md)
- [`Series::diff`](diff.md): adjacent-sample differences, not callable
  differentiation.
- [`Series::antidiff`](antidiff.md): sampled-data trapezoidal integration.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="broadcast_to.html"><span class="label">Previous</span><span class="title">broadcast_to</span></a>
  <a class="next-link" href="cartprod.html"><span class="label">Next</span><span class="title">cartprod</span></a>
</div>
