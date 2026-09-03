<!-- pluss-hand-authored -->

# Series::reverse

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::reverse() const -> void
Series::reverse(bool keep_graph,
                bool accumulate_sensitivity = false,
                bool track_higher_order = false) const -> void
Series::reverse(const Series& sensitivity_output) const -> void
Series::reverse(const Series& sensitivity_output,
                bool keep_graph,
                bool accumulate_sensitivity = false,
                bool track_higher_order = false) const -> void
```

## What It Does

`reverse()` launches reverse-mode propagation from the current `Series`.

If the current `Series` is the objective, `reverse()` walks backward through
the recorded computation graph and accumulates sensitivities onto tracked input
leaves.

This is the Pluss-native reverse-mode entrypoint. In day-to-day code, it is the
method you call after the forward computation is complete and you are ready to
populate sensitivities on the leaves that had tracking enabled.

## Read This First

`reverse()` propagates sensitivity backward from the current output node. The
stored result then lives on whichever `Series` you inspect.

If `y` was built from `x` and you call

```cpp
y.reverse();
```

then the two most common reads mean different things:

- `x.sensitivity()` means `dL/dx`, which for scalar `y` is the familiar
  `dy/dx`
- `y.sensitivity()` means `dL/dy`, which for scalar `y` as the objective is
  usually `1`

So the usual workflow is:

1. mark the leaf or leaves you want tracked
2. build the forward computation
3. call `reverse()` on the finished objective
4. read `sensitivity()` from the tracked leaves whose derivatives you want

That rule does not change for products, quotients, chains, trigonometric
expressions, or any other composition. `reverse()` always propagates from the
current output back to tracked leaves; `sensitivity()` always answers "with
respect to this specific `Series`."

Three nearby methods often get conflated when you first meet this part of the
API:

- [`tracks_slope_()`](tracks_slope_.md) is the opt-in step. It marks a `Series`
  as a leaf that should receive reverse-mode sensitivity later.
- `reverse()` is the propagation step. It applies the chain rule from the
  current output back through the recorded computation.
- [`sensitivity()`](sensitivity.md) is the read step. It returns the stored
  sensitivity that `reverse()` populated on a tracked leaf.

## Overload Map

There are two independent questions:

1. Is the current output scalar or non-scalar?
2. Do you need lifecycle toggles such as graph retention or sensitivity
   accumulation?

That yields four public shapes:

- `reverse()`
  Use this for a scalar objective when the default one-shot behavior is fine.
- `reverse(keep_graph, accumulate_sensitivity, track_higher_order)`
  Use this for a scalar objective when you need toggle control.
- `reverse(sensitivity_output)`
  Use this for a non-scalar objective, or whenever you want to choose the
  upstream sensitivity explicitly.
- `reverse(sensitivity_output, keep_graph, accumulate_sensitivity, track_higher_order)`
  Use this when the output is non-scalar and you also need lifecycle control.

## Parameters

**sensitivity_output** (`const Series&`)

The upstream sensitivity to seed at the current output.

This parameter matters whenever the current `Series` is not scalar.

- For scalar outputs, you usually omit it and let the method use the implicit
  scalar seed.
- For non-scalar outputs, you supply a same-shaped seed that says how much each
  output position contributes upstream.

That seed is mathematically load-bearing. It defines the weighted scalar
objective whose sensitivity will be propagated backward.

**keep_graph** (`bool`)

Controls whether the recorded source links used by this reverse pass are kept
after the pass finishes.

- `false`: one-shot behavior; release what can be released after the pass
- `true`: keep the graph links so the same objective can be reversed again

Use `true` when the same forward result must support multiple reverse passes.

**accumulate_sensitivity** (`bool`)

Controls what happens to already stored leaf sensitivities before this pass.

- `false`: clear existing stored sensitivities before the pass
- `true`: preserve them and add the new contributions on top

This toggle changes result interpretation materially. With accumulation off,
each reverse pass gives you a fresh answer. With accumulation on, repeated
passes add together.

**track_higher_order** (`bool`)

Controls whether the propagation itself should be recorded so higher-order
reverse work can be built on top of it.

- `false`: ordinary first-order reverse pass
- `true`: ask the system to record the propagation path for higher-order use

For ordinary first-order sensitivity extraction, leave this at `false`.

## Returns

`void`
    `reverse()` populates sensitivity state on tracked leaves rather than
    returning a numeric value.

## Gradient Model

`reverse()` always computes the sensitivity of a scalar objective with respect
to tracked leaves.

That scalar objective can arise in two different ways:

- directly, because the current `Series` already has one element
- indirectly, because you supplied `sensitivity_output` for a non-scalar result

For a scalar output `L`, the stored sensitivity on `x` is the elementwise
quantity `dL/dx`.

For a non-scalar output `y`, calling `y.reverse(sensitivity_output)` means
Pluss treats the effective scalar objective as

```text
L = sum(sensitivity_output * y)
```

and then propagates `dL/dx` backward.

This sum should not be read narrowly as "the only allowed arithmetic total."
It is one standard way of collapsing a non-scalar output into a scalar
objective for differentiation. In ordinary user code, any expression whose
final result is scalar can play that role: a sum, a weighted sum, a
difference, a product, a ratio, or a more elaborate composition.

This is why a non-scalar reverse pass cannot guess its seed. Until you provide
that seed, there is no single scalar objective yet.

## Examples

Example 1. A scalar objective can reverse directly without an explicit seed.

```cpp
auto x = pluss::make_series(1.25, pluss::cfg::tracks_slope());

auto y = x * x * x - 2.0 * (x * x) + x;
y.reverse();

std::cout << "y=" << y.item<double>() << "\n";
std::cout << "dL_dx=" << x.sensitivity().item<double>() << "\n";
```

Here the scalar objective is

```text
L = x³ - 2x² + x
```

so

```text
dL/dx = 3x² - 4x + 1
```

Evaluating at `x = 1.25` gives

```text
3(1.25)^2 - 4(1.25) + 1 = 0.6875
```

### Output

```text
y=0.078125
dL_dx=0.6875
```

Example 2. A non-scalar result is commonly reduced first, then reversed as a
scalar objective.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
x.tracks_slope_(true);

auto y = x.mul(pluss::make_series({10.0, 20.0, 30.0}));
y.sum().reverse();

std::cout << "y=" << y << "\n";
std::cout << "dL_dx=" << x.sensitivity() << "\n";
```

The forward value `y` is vector-valued:

```text
y = [10x₁, 20x₂, 30x₃]
```

Here `y.sum()` is just the chosen scalar objective builder for the example. It
is common, but it is not the only possibility.

`y.sum()` turns this particular vector output into the scalar objective

```text
L = 10x₁ + 20x₂ + 30x₃
```

Differentiating with respect to `x` gives

```text
dL/dx = [10, 20, 30]
```

The actual values of `x` do not matter here because the objective is linear in
each entry.

### Output

```text
y=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[10, 40, 90]
dL_dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[10, 20, 30]
```

This is the most common pattern for vector-valued forward code: reduce to a
scalar objective, then call `reverse()` with no explicit seed.

Example 3. A non-scalar output can be reversed directly when you provide the
upstream sensitivity explicitly.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
x.tracks_slope_(true);

auto y = x.mul(x);
y.reverse(pluss::make_series({1.0, 10.0, 100.0}));

std::cout << "y=" << y << "\n";
std::cout << "dL_dx=" << x.sensitivity() << "\n";
```

This is the key seeded case.

The forward output is

```text
y = [x₁², x₂², x₃²]
```

Because the seed is `[1, 10, 100]`, the effective scalar objective is

```text
L = 1·y₁ + 10·y₂ + 100·y₃
  = 1·x₁² + 10·x₂² + 100·x₃²
```

Different output positions therefore contribute with different weights.

Now differentiate entrywise at `x = [1, 2, 3]`:

```text
dL/dx₁ = 1 · 2x₁ = 1 · 2(1) = 2
dL/dx₂ = 10 · 2x₂ = 10 · 2(2) = 40
dL/dx₃ = 100 · 2x₃ = 100 · 2(3) = 600
```

### Output

```text
y=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 4, 9]
dL_dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2, 40, 600]
```

The seed weights the positions differently, so the propagated sensitivity does
too.

Example 4. Reversing a non-scalar output without a seed fails explicitly.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
x.tracks_slope_(true);

auto y = x.mul(x);
try {
  y.reverse();
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

Without the seed there is no single scalar objective yet. A vector output has
many possible downstream uses, so `reverse()` cannot guess how those output
positions should be combined.

### Output

```text
series_tape::backpropagate: non-scalar output requires explicit seed
```

Example 5. With `keep_graph=true` and the default
`accumulate_sensitivity=false`, repeated reverse passes on the same objective
overwrite the stored sensitivity rather than adding to it.

```cpp
auto x = pluss::make_series(2.0);
x.tracks_slope_(true);

auto y = x * x;
y.reverse(true);
std::cout << "first_dL_dx=" << x.sensitivity().item<double>() << "\n";
y.reverse(true, false);
std::cout << "second_dL_dx=" << x.sensitivity().item<double>() << "\n";
```

Here `L = x²`, so each reverse pass computes `dL/dx = 2x = 4` at `x = 2`.
The only question is whether the second pass replaces the stored value or adds
to it.

### Output

```text
first_dL_dx=4
second_dL_dx=4
```

Example 6. With `accumulate_sensitivity=true`, the second pass adds on top of
the first one.

```cpp
auto x = pluss::make_series(2.0);
x.tracks_slope_(true);

auto y = x * x;
y.reverse(true);
std::cout << "first_dL_dx=" << x.sensitivity().item<double>() << "\n";
y.reverse(true, true);
std::cout << "second_dL_dx=" << x.sensitivity().item<double>() << "\n";
```

The derivative of one pass is still `4`. With accumulation enabled, the second
pass stores `4 + 4 = 8`.

### Output

```text
first_dL_dx=4
second_dL_dx=8
```

Examples 5 and 6 are the important toggle pair. The forward objective did not
change. Only the accumulation policy changed.

Example 7. If nothing was marked for tracking, `reverse()` does not populate a
stored sensitivity on that leaf.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
x.mul(2.0).sum().reverse();
std::cout << "has_dL_dx=" << (x.has_sensitivity() ? "true" : "false") << "\n";
```

`reverse()` still runs on the scalar result. What is missing is not the
backward pass itself, but a tracked leaf on which to store the propagated
sensitivity.

### Output

```text
has_dL_dx=false
```

Example 8. A scalar expression such as `x * x.cos()` does run the reverse
pass, but if the input leaf was never marked for tracking then there is
nowhere to store `dy/dx`, and reading `y.sensitivity()` does not recover it.

```cpp
auto x = pluss::make_series(1.7);
auto y = x * x.cos();

y.reverse();

std::cout << "x_has_dy_dx=" << (x.has_sensitivity() ? "true" : "false") << "\n";
std::cout << "y_has_stored_sensitivity=" << (y.has_sensitivity() ? "true" : "false") << "\n";
std::cout << "y_sensitivity=" << y.sensitivity().item<double>() << "\n";
```

This is the current public distinction on the verified surface:

- `reverse()` is the propagation step
- `tracks_slope_()` is the opt-in storage step for input leaves
- `x.sensitivity()` is where `dy/dx` is stored after you tracked `x`
- `y.sensitivity()` is about stored sensitivity on the output object itself,
  not a shortcut for "please give me the derivative with respect to the input"

### Output

```text
x_has_dy_dx=false
y_has_stored_sensitivity=false
y_sensitivity=0
```

Example 9. The corrected product-rule usage marks `x` for tracking before the
forward expression is built, then reads the derivative from `x.sensitivity()`.

```cpp
auto x = pluss::make_series(1.7);
x.tracks_slope_(true);

auto y = x * x.cos();
y.reverse();

std::cout << "y=" << y.item<double>() << "\n";
std::cout << "dy_dx=" << x.sensitivity().item<double>() << "\n";
std::cout << "dL_dy=" << y.sensitivity().item<double>() << "\n";
```

The forward function is

```text
y = x cos(x)
```

Use the product rule:

```text
d/dx [x cos(x)] = 1·cos(x) + x·(-sin(x))
                = cos(x) - x sin(x)
```

Evaluating at `x = 1.7` gives

```text
dy/dx = cos(1.7) - 1.7 sin(1.7) = -1.8146746720647213
```

The output object itself is the scalar objective, so its own stored root
sensitivity is `1`.

### Output

```text
y=-0.21903564030239187
dy_dx=-1.8146746720647213
dL_dy=1
```

Example 10. Quotient-rule objectives work the same way.

```cpp
auto x = pluss::make_series(1.7);
x.tracks_slope_(true);

auto y = x.div(x.square().add(1.0));
y.reverse();

std::cout << pluss::series::print(y) << "\n";
std::cout << "dy_dx=" << x.sensitivity().item<double>() << "\n";
```

The forward function is

```text
y = x / (1 + x²)
```

Differentiate with the quotient rule:

```text
dy/dx = ((1 + x²)·1 - x·2x) / (1 + x²)²
      = (1 - x²) / (1 + x²)²
```

At `x = 1.7`:

```text
dy/dx = (1 - 1.7²) / (1 + 1.7²)² = -0.12490004692012345
```

### Output

```text
Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=0.43701799485861187
dy_dx=-0.12490004692012345
```

Example 11. Chain-rule compositions are also recorded correctly.

```cpp
auto x = pluss::make_series(1.2);
x.tracks_slope_(true);

auto y = x.square().cos();
y.reverse();

std::cout << "y=" << y.item<double>() << "\n";
std::cout << "dy_dx=" << x.sensitivity().item<double>() << "\n";
```

Here the outer function is `cos(u)` and the inner function is `u = x²`, so

```text
dy/dx = (-sin(u)) · du/dx
      = -sin(x²) · 2x
```

At `x = 1.2`:

```text
dy/dx = -sin(1.2²) · 2(1.2) = -2.3795000356600475
```

### Output

```text
y=0.13042370873814554
dy_dx=-2.3795000356600475
```

Example 12. Scalar objectives do not need to be sums. Any expression whose
final result is scalar can be the objective you reverse.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
x.tracks_slope_(true);

auto objective = 2.0 * x.pos(0) + 4.0 * x.pos(1) + x.pos(2).sqrt();
objective.reverse();

std::cout << "L=" << objective.item<double>() << "\n";
pluss::series::print(std::cout, x.sensitivity()) << "\n";
```

The scalar objective is

```text
L = 2x₁ + 4x₂ + √x₃
```

Differentiate entrywise:

```text
dL/dx₁ = 2
dL/dx₂ = 4
dL/dx₃ = 1 / (2√x₃)
```

At `x₃ = 3`:

```text
dL/dx₃ = 1 / (2√3) = 0.28867513459481287
```

### Output

```text
L=11.732050807568877
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2, 4, 0.28867513459481287]
```

Example 13. Mixed transcendental objectives can combine trigonometric and
logarithmic terms, and the stored sensitivity remains the entrywise partial
derivative of that scalar objective.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
x.tracks_slope_(true);

auto objective = x.pos(0).sin() - 1.5 * x.pos(1).cos() + x.pos(2).log();
objective.reverse();

std::cout << "L=" << objective.item<double>() << "\n";
std::cout << "dL_dx=" << x.sensitivity() << "\n";
```

The scalar objective is

```text
L = sin(x₁) - 1.5 cos(x₂) + log(x₃)
```

Differentiate each entry:

```text
dL/dx₁ = cos(x₁)
dL/dx₂ = 1.5 sin(x₂)
dL/dx₃ = 1 / x₃
```

At `x = [1, 2, 3]`:

```text
dL/dx = [cos(1), 1.5 sin(2), 1/3]
      = [0.5403023058681398, 1.3639461402385225, 0.3333333333333333]
```

### Output

```text
L=2.56430352829672
dL_dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.5403023058681398, 1.3639461402385225, 0.3333333333333333]
```

## Gotchas And Failure Modes

- `reverse()` computes sensitivities for tracked leaves. If you never enabled
  tracking on a leaf, there is nothing to store there afterward.
- The exact snippet `x = make_series(1.7); y = x * x.cos(); y.reverse();`
  does not store `dy/dx` on `x` unless you first opt `x` into tracking.
- Reading `y.sensitivity()` is not the same thing as asking for `dy/dx`. Read
  the tracked input leaf you care about, usually `x.sensitivity()`.
- Scalar outputs can use the implicit root seed `1`. Non-scalar outputs must
  receive an explicit seed.
- `sensitivity_output` changes the mathematics of the result. It is not merely
  an overload-disambiguation token.
- `keep_graph=true` keeps the captured reverse path available for another pass,
  but it does not by itself request accumulation.
- `accumulate_sensitivity=true` adds the new result on top of what was already
  stored. Leave it `false` when you want a fresh answer per pass.
- `track_higher_order=true` is for higher-order workflows, not ordinary first
  sensitivities.

## Summary

Use `reverse()` when the forward computation is complete and you want
reverse-mode sensitivities propagated onto tracked leaves. First decide which
leaves should track, then decide whether your output is already scalar or needs
an explicit seed, and finally read the result from `sensitivity()` on the
tracked leaves.

## See Also

- [`Series::tracks_slope_`](tracks_slope_.md): mark a leaf for tracking.
- [`Series::sensitivity`](sensitivity.md): read the stored result.
- [`Series::has_sensitivity`](has_sensitivity.md): distinguish stored state
  from zero-valued readback.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="reshape_as.html">
    <span class="label">Previous</span>
    <span class="title">reshape_as</span>
  </a>
  <a class="next-link" href="round.html">
    <span class="label">Next</span>
    <span class="title">round</span>
  </a>
</div>
