<!-- pluss-hand-authored -->

# Series::retains_grad

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::retains_grad() const
```

## What It Does

`retains_grad()` reports whether this `Series` is currently marked to keep its
stored sensitivity after reverse-mode runs.

One public-surface quirk matters immediately: this method returns a scalar
boolean `Series`, not a plain `bool`. In ordinary C++ use, most callers read it
with `.item<bool>()`.

## Parameters

None.

## Returns

`Series`
    A scalar boolean `Series` indicating whether retention is enabled on this
    value.

## Relationship To Nearby Methods

- [`retain_grad()`](retain_grad.md) changes this state
- `retains_grad()` reads this state
- [`has_sensitivity()`](has_sensitivity.md) asks whether a stored sensitivity
  currently exists

Those are different questions:

- one is configuration
- one is current stored data presence

## Mathematical Meaning

Suppose `mid` is an intermediate value in a computation with scalar objective
`L`.

If retention is enabled, then after reverse-mode finishes, `mid` is allowed to
keep the stored quantity

```text
dL/dmid
```

If retention is disabled, that same quantity may still exist transiently during
propagation, but it is not kept as a public stored result afterward.

So `retains_grad()` is not asking whether the derivative exists mathematically.
It is asking whether the object is configured to keep that derivative around
for later inspection.

## Examples

Example 1. Intermediates do not start in retain mode by default.

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

Example 2. Turning retention off again is reflected immediately.

```cpp
auto x = pluss::make_series({1.0, 2.0});
x.tracks_slope_(true);
auto mid = x.mul(4.0);
mid.retain_grad(false);

std::cout << "retains=" << (mid.retains_grad().item<bool>() ? "true" : "false") << "\n";
```

### Output

```text
retains=false
```

Example 3. Retention state and stored-sensitivity presence are related but not
identical.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
x.tracks_slope_(true);

auto mid = x.mul(2.0);
mid.retain_grad();

std::cout << "retains=" << (mid.retains_grad().item<bool>() ? "true" : "false") << "\n";
std::cout << "mid_has_dL_dmid=" << (mid.has_sensitivity() ? "true" : "false") << "\n";
```

At this point retention is enabled, but no reverse pass has run yet, so the
intermediate still has no stored sensitivity.

### Output

```text
retains=true
mid_has_dL_dmid=false
```

Example 4. After reverse runs, a retained intermediate can both report
retention and hold stored sensitivity.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
x.tracks_slope_(true);

auto mid = x.mul(2.0);
mid.retain_grad();
mid.sum().reverse();

std::cout << "retains=" << (mid.retains_grad().item<bool>() ? "true" : "false") << "\n";
std::cout << "mid_has_dL_dmid=" << (mid.has_sensitivity() ? "true" : "false") << "\n";
```

For

```text
mid = 2x
L = sum(mid)
```

the reverse pass computes `dL/dmid = [1, 1, 1]`. Because retention is enabled,
that intermediate sensitivity is kept.

### Output

```text
retains=true
mid_has_dL_dmid=true
```

## Gotchas And Failure Modes

- `retains_grad()` returns a scalar `Series`, not a plain `bool`.
- `retains_grad() == true` does not by itself mean reverse-mode has already
  populated stored sensitivity. It only means the keep-state is enabled.
- Use [`has_sensitivity()`](has_sensitivity.md) when you need to know whether a
  stored sensitivity currently exists.

## Summary

Use `retains_grad()` to inspect whether a `Series` is configured to keep its
stored sensitivity after reverse, and read it as a boolean scalar with
`.item<bool>()` in ordinary C++ code.

## See Also

- [`Series::retain_grad`](retain_grad.md): enable or disable the keep-state.
- [`Series::has_sensitivity`](has_sensitivity.md): check whether stored
  sensitivity currently exists.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="retain_grad.html">
    <span class="label">Previous</span>
    <span class="title">retain_grad</span>
  </a>
  <a class="next-link" href="reverse.html">
    <span class="label">Next</span>
    <span class="title">reverse</span>
  </a>
</div>
