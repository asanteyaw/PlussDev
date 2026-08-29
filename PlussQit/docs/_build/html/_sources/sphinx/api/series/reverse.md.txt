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

That seed is not a cosmetic option. It changes the reverse computation.

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

## Examples

Example 1. A scalar objective can reverse directly without an explicit seed.

```cpp
auto x = pluss::make_series(1.25);
x.tracks_slope_(true);

auto y = x * x * x - 2.0 * (x * x) + x;
y.reverse();

std::cout << "y=" << y.item<double>() << "\n";
std::cout << "dx=" << x.sensitivity().item<double>() << "\n";
```

### Output

```text
y=0.078125
dx=0.6875
```

Example 2. A non-scalar result is commonly reduced first, then reversed as a
scalar objective.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
x.tracks_slope_(true);

auto y = x.mul(pluss::make_series({10.0, 20.0, 30.0}));
y.sum().reverse();

std::cout << "y=" << y << "\n";
std::cout << "dx=" << x.sensitivity() << "\n";
```

### Output

```text
y=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[10, 40, 90]
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
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
std::cout << "dx=" << x.sensitivity() << "\n";
```

### Output

```text
y=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 4, 9]
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
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
std::cout << "first=" << x.sensitivity().item<double>() << "\n";
y.reverse(true, false);
std::cout << "second=" << x.sensitivity().item<double>() << "\n";
```

### Output

```text
first=4
second=4
```

Example 6. With `accumulate_sensitivity=true`, the second pass adds on top of
the first one.

```cpp
auto x = pluss::make_series(2.0);
x.tracks_slope_(true);

auto y = x * x;
y.reverse(true);
std::cout << "first=" << x.sensitivity().item<double>() << "\n";
y.reverse(true, true);
std::cout << "second=" << x.sensitivity().item<double>() << "\n";
```

### Output

```text
first=4
second=8
```

Examples 5 and 6 are the important toggle pair. The forward objective did not
change. Only the accumulation policy changed.

Example 7. If nothing was marked for tracking, `reverse()` does not populate a
stored sensitivity on that leaf.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
x.mul(2.0).sum().reverse();
std::cout << "has_dx=" << (x.has_sensitivity() ? "true" : "false") << "\n";
```

### Output

```text
has_dx=false
```

## Gotchas And Failure Modes

- `reverse()` acts on the current result, not automatically on the original
  inputs.
- Non-scalar outputs need either an explicit seed or a prior scalar reduction.
- `keep_graph` and `accumulate_sensitivity` solve different problems:
  graph reuse versus sensitivity reuse.
- Stored sensitivity lives on the tracked leaves. Use
  [`has_sensitivity`](has_sensitivity.md) and [`sensitivity`](sensitivity.md)
  to inspect it afterward.

## Summary

Use `reverse()` to propagate sensitivity from the current objective back to
tracked leaves.

- use the short form for scalar objectives
- seed non-scalar objectives explicitly, or reduce them first
- set `keep_graph=true` only when the same objective must be reversed again
- set `accumulate_sensitivity=true` only when you intentionally want repeated
  contributions to add together

## See Also

- [`Series::tracks_slope_`](tracks_slope_.md): enable tracking on leaves before
  the forward pass.
- [`Series::sensitivity`](sensitivity.md): read the stored result afterward.
- [`Series::clear_sensitivity`](clear_sensitivity.md): reset stored
  sensitivity between passes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="reshape.html">
    <span class="label">Previous</span>
    <span class="title">reshape</span>
  </a>
  <a class="next-link" href="select.html">
    <span class="label">Next</span>
    <span class="title">select</span>
  </a>
</div>
