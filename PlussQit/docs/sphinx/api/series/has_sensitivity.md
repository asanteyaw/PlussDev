<!-- pluss-hand-authored -->

# Series::has_sensitivity

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::has_sensitivity() const -> bool
```

## What It Does

`has_sensitivity()` tells you whether this `Series` currently stores a
reverse-mode sensitivity value.

That wording is deliberate.

The method answers a storage-state question, not a mathematical question. It
does not ask whether the correct derivative would be zero or nonzero. It asks
whether a reverse pass has populated a stored sensitivity on this value and
that stored value has not since been cleared.

This makes `has_sensitivity()` the right companion to
[`sensitivity()`](sensitivity.md), especially because the current public
surface returns a zero-valued series from `sensitivity()` even before any
reverse pass has populated one.

## Parameters

None.

## Returns

`bool`
    `true` if a stored sensitivity currently exists on this `Series`,
    otherwise `false`.

## Relationship To Nearby Methods

- [`tracks_slope()`](tracks_slope.md) answers "is this series configured to
  receive reverse-mode sensitivity if propagation reaches it?"
- `has_sensitivity()` answers "does this series currently hold a stored
  reverse-mode result?"
- [`sensitivity()`](sensitivity.md) reads that stored result itself

Those are related, but they are different stages:

1. configure tracking
2. run reverse-mode
3. inspect whether a stored result exists
4. read the stored result

## Mathematical Meaning

Suppose a tracked leaf `x` participates in a scalar objective `L`.

After `reverse()` runs, the stored value on `x` is the sensitivity `dL/dx`.

`has_sensitivity()` does not tell you what `dL/dx` is. It only tells you
whether that quantity is currently stored on the object.

That distinction matters because a stored sensitivity can legitimately be all
zeros, and on the current public surface a not-yet-populated sensitivity also
reads back as zeros through `sensitivity()`.

## Examples

Example 1. A tracked `Series` starts without stored sensitivity, gains it after
reverse, and loses it again after clearing.

```cpp
auto life = pluss::make_series({2.0, 4.0, 8.0}, pluss::cfg::tracks_slope());

std::cout << "before=" << (life.has_sensitivity() ? "true" : "false") << "\n";
life.log().sum().reverse();
std::cout << "after_reverse=" << (life.has_sensitivity() ? "true" : "false") << "\n";
life.clear();
std::cout << "after_clear=" << (life.has_sensitivity() ? "true" : "false") << "\n";
```

The scalar objective is

```text
L = log(2) + log(4) + log(8)
```

so a real reverse pass does occur and produces a real stored sensitivity.
`has_sensitivity()` is the boolean check that this stored result exists.

### Output

```text
before=false
after_reverse=true
after_clear=false
```

Example 2. `has_sensitivity()` is only ever `true` after a reverse pass over a
tracked leaf. Reverse-mode over an objective with no tracked leaf does not fall
through to an empty result — it raises.

```cpp
auto silent_x = pluss::make_series({1.0, 2.0, 3.0});

try {
  silent_x.mul(2.0).sum().reverse();
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}

std::cout << "has_dL_dx=" << (silent_x.has_sensitivity() ? "true" : "false") << "\n";
```

The scalar objective `L = 2x₁ + 2x₂ + 2x₃` is mathematically differentiable,
but `silent_x` was never configured as a tracked leaf, so the graph is
disconnected and [`reverse()`](reverse.md) reports it. `has_sensitivity()` is
still `false` afterward — nothing was stored.

### Output

```text
series_tape::backpropagate: objective does not track slope and has no source link; nothing to differentiate (a constant, or built only from non-differentiable ops)
has_dL_dx=false
```

Example 3. `has_sensitivity()` is the way to distinguish "no stored
sensitivity yet" from "a stored sensitivity that happens to contain zeros."

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
x.tracks_slope_(true);

std::cout << "has_dL_dx=" << (x.has_sensitivity() ? "true" : "false") << "\n";
std::cout << x.sensitivity();
```

### Output

```text
has_dL_dx=false
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0, 0]
```

The zero values alone do not prove a reverse pass happened. The boolean state
from `has_sensitivity()` is what disambiguates that case.

Example 4. A retained intermediate can also report stored sensitivity after
reverse.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
x.tracks_slope_(true);

auto mid = x.mul(2.0);
mid.retain_grad();
mid.sum().reverse();

std::cout << "mid_has_dL_dmid=" << (mid.has_sensitivity() ? "true" : "false") << "\n";
std::cout << "x_has_dL_dx=" << (x.has_sensitivity() ? "true" : "false") << "\n";
```

Here `mid = 2x`, and the scalar objective is

```text
L = sum(mid)
```

so `dL/dmid = [1, 1, 1]` and `dL/dx = [2, 2, 2]`. Because `mid` was marked to
retain its own sensitivity, both booleans become `true`.

### Output

```text
mid_has_dL_dmid=true
x_has_dL_dx=true
```

## Gotchas And Failure Modes

- `has_sensitivity()` can be `false` even when `tracks_slope()` is `true`.
- A zero-valued `sensitivity()` result does not imply `has_sensitivity()` is
  `true`.
- Clearing sensitivity resets this method back to `false` without disabling
  tracking.
- On intermediates, this method is usually only interesting when retention was
  enabled first.

## Summary

Use `has_sensitivity()` as the authoritative test for whether a `Series`
currently stores reverse-mode sensitivity. It answers a storage-state question,
not a derivative-value question.

## See Also

- [`Series::sensitivity`](sensitivity.md): read the stored value itself.
- [`Series::tracks_slope`](tracks_slope.md): inspect tracking configuration.
- [`Series::clear`](clear.md): clear stored
  sensitivity.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="hardshrink.html">
    <span class="label">Previous</span>
    <span class="title">hardshrink</span>
  </a>
  <a class="next-link" href="heaviside.html">
    <span class="label">Next</span>
    <span class="title">heaviside</span>
  </a>
</div>
