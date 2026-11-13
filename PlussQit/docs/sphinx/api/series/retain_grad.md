<!-- pluss-hand-authored -->

# Series::retain_grad

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::retain_grad() const
Series Series::retain_grad(bool keep) const
```

## What It Does

`retain_grad()` tells the reverse-mode system to keep stored sensitivity on
this `Series` after a later reverse pass.

This matters most for intermediate results.

By default, non-root intermediate values may participate in propagation during
reverse without keeping their own stored public sensitivity afterward.
`retain_grad()` changes that policy for the chosen node so you can inspect the
intermediate's own sensitivity after reverse finishes.

## Overload Map

- `retain_grad()`
  Shorthand for enabling retention.
- `retain_grad(bool keep)`
  Explicitly set whether retention should be on or off.

## Parameters

**keep** (`bool`)

Whether to keep this series' sensitivity after reverse-mode runs.

- `true`: keep stored sensitivity on this value
- `false`: do not keep it

The zero-argument form means `keep = true`.

## Returns

`Series`
    The current `Series` value.

This lets the method sit naturally in value-oriented call chains even though
its real job is to mutate reverse-mode retention state.

## Mathematical Meaning

Suppose the forward computation has an intermediate `mid` and a final scalar
objective `L`.

During reverse-mode, Pluss may need the quantity

```text
dL/dmid
```

in order to continue propagating back to earlier values.

Without retention, that intermediate sensitivity can be used internally during
the reverse pass and then discarded afterward.

With `retain_grad()`, that same quantity is kept as a readable public result on
`mid`, so `mid.sensitivity()` becomes meaningful after the pass completes.

This means `retain_grad()` does not change the mathematics of the derivative.
It changes whether a non-leaf keeps its own derivative result for inspection.

## When It Matters

`retain_grad()` is mainly for inspection of non-leaf values:

- branch outputs
- intermediate transforms
- staging values in long computational chains
- any other interior node whose sensitivity you want to inspect after reverse

Without retention, leaf sensitivities are still computed normally. What changes
is whether the intermediate keeps its own stored sensitivity around for you to
read.

## Examples

Example 1. Retaining an intermediate keeps its sensitivity available after the
reverse pass.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0}, pluss::cfg::tracks_slope());

auto mid = x.mul(2.0);
mid.retain_grad();
mid.sum().reverse();

std::cout << "retains=" << (mid.retains_grad().item<bool>() ? "true" : "false") << "\n";
std::cout << "mid_has_dL_dmid=" << (mid.has_sensitivity() ? "true" : "false") << "\n";
std::cout << "mid_dL_dmid=" << mid.sensitivity() << "\n";
std::cout << "x_dL_dx=" << x.sensitivity() << "\n";
```

Here

```text
mid = 2x
L = sum(mid)
```

So the reverse pass sees

```text
dL/dmid = [1, 1, 1]
```

because `sum` contributes one unit of upstream sensitivity to each entry, and

```text
dL/dx = dL/dmid * dmid/dx = [1, 1, 1] * 2 = [2, 2, 2]
```

because each entry of `mid` is `2 * x`.

### Output

```text
retains=true
mid_has_dL_dmid=true
mid_dL_dmid=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 1, 1]
x_dL_dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2, 2, 2]
```

Example 2. Without retention, the intermediate still helps propagate, but it
does not keep its own stored sensitivity afterward.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
x.tracks_slope_(true);

auto mid = x.mul(2.0);
mid.sum().reverse();

std::cout << "mid_retains=" << (mid.retains_grad().item<bool>() ? "true" : "false") << "\n";
std::cout << "mid_has_dL_dmid=" << (mid.has_sensitivity() ? "true" : "false") << "\n";
std::cout << "x_has_dL_dx=" << (x.has_sensitivity() ? "true" : "false") << "\n";
```

The chain rule still reaches the leaf, so `x` keeps its stored sensitivity.
What is missing is only the intermediate's own retained public result.

### Output

```text
mid_retains=false
mid_has_dL_dmid=false
x_has_dL_dx=true
```

Example 3. The boolean overload can turn retention back off.

```cpp
auto x = pluss::make_series({1.0, 2.0});
x.tracks_slope_(true);

auto mid = x.mul(4.0);
mid.retain_grad(false);
mid.sum().reverse();

std::cout << "retains=" << (mid.retains_grad().item<bool>() ? "true" : "false") << "\n";
std::cout << "mid_has_dL_dmid=" << (mid.has_sensitivity() ? "true" : "false") << "\n";
```

### Output

```text
retains=false
mid_has_dL_dmid=false
```

Example 4. The state change itself is visible immediately through
`retains_grad()`, even before any reverse pass happens.

```cpp
auto x = pluss::make_series({1.0, 2.0});
x.tracks_slope_(true);
auto mid = x.mul(3.0);

std::cout << "before=" << (mid.retains_grad().item<bool>() ? "true" : "false") << "\n";
mid.retain_grad();
std::cout << "after=" << (mid.retains_grad().item<bool>() ? "true" : "false") << "\n";
```

### Output

```text
before=false
after=true
```

## Gotchas And Failure Modes

- `retain_grad()` does not run reverse-mode by itself. It only changes what is
  kept after a later reverse pass.
- This method does not change `dL/dx` on earlier tracked leaves. It only
  changes whether an intermediate keeps its own `dL/dmid`.
- The public return type is `Series`, but the real effect is retention-state
  mutation on the receiver.

## Summary

Use `retain_grad()` when an intermediate `Series` should keep its own stored
sensitivity after reverse so you can inspect it later.

## See Also

- [`Series::retains_grad`](retains_grad.md): inspect the keep-state flag.
- [`Series::sensitivity`](sensitivity.md): read the stored sensitivity after
  reverse.
- [`Series::reverse`](reverse.md): run the reverse pass that populates the
  stored values.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="resize_as.html">
    <span class="label">Previous</span>
    <span class="title">resize_as</span>
  </a>
  <a class="next-link" href="retains_grad.html">
    <span class="label">Next</span>
    <span class="title">retains_grad</span>
  </a>
</div>
