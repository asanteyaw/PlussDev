<!-- pluss-hand-authored -->

# Series::is_leaf

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
bool Series::is_leaf() const
```

## What It Does

Returns `true` when the `Series` is a **leaf** of the reverse-mode graph —
that is, when it has no recorded operation that produced it — and `false`
when it is an interior result computed from other tracked tensors.

Concretely:

- A `Series` you created directly (`make_series`, `zeros`, a parameter you
  built) is a leaf, whether or not it tracks slope.
- The result of any op on a slope-tracking input (`x.square()`,
  `a.matmul(b)`, …) is **not** a leaf — it carries a source link back to its
  parents.
- [`detach()`](detach.md) severs that link, so a detached view of an
  interior result is a leaf again.

Leaf status is about **graph provenance**, nothing else. It is independent of
rank, dtype, platform, whether the tensor currently holds a
[`sensitivity`](sensitivity.md), and — importantly — independent of
[`tracks_slope()`](tracks_slope.md): a leaf can track slope (a trainable
parameter) or not (a plain constant), and an interior result tracks slope
while never being a leaf.

Why it matters: after [`reverse()`](reverse.md), sensitivity is retained
automatically only on **leaves** — they are the natural optimization targets
(weights, inputs you are differentiating with respect to). To keep an
interior result's sensitivity you must ask for it explicitly with
[`retain_grad()`](retain_grad.md).

## Parameters

None.

## Returns

`bool`
    `true` iff the `Series` has no source link (it was created directly, or
    detached), `false` if it was produced by a recorded op.

## Gradient

Not applicable — `is_leaf()` returns a plain `bool`. (It *describes* the
graph; it is not part of it.)

## Examples

Example 1. A plain, untracked `Series` is a leaf — leaf-ness does not
require slope tracking.

```cpp
auto x = pluss::make_series({1.0, 2.0});
std::cout << std::boolalpha
          << "untracked: is_leaf=" << x.is_leaf()
          << " tracks_slope=" << x.tracks_slope() << "\n";
```

### Output

```text
untracked: is_leaf=true tracks_slope=false
```

Example 2. A tracked user tensor is a leaf; a value computed from it is not
— even though both track slope.

```cpp
auto leaf = pluss::make_series({1.0, 2.0});
leaf.tracks_slope_(true);
auto result = leaf.square();
std::cout << std::boolalpha
          << "leaf:   is_leaf=" << leaf.is_leaf()   << " tracks_slope=" << leaf.tracks_slope()   << "\n"
          << "result: is_leaf=" << result.is_leaf() << " tracks_slope=" << result.tracks_slope() << "\n";
```

### Output

```text
leaf:   is_leaf=true tracks_slope=true
result: is_leaf=false tracks_slope=true
```

This is the distinction to keep straight: `is_leaf()` and `tracks_slope()`
are orthogonal. `result` tracks slope (its gradient will flow) but it is
*not* a leaf (it has parents).

Example 3. [`detach()`](detach.md) makes an interior result a leaf again by
cutting its link to the graph.

```cpp
auto leaf = pluss::make_series({1.0, 2.0});
leaf.tracks_slope_(true);
auto result = leaf.square();
auto d = result.detach();
std::cout << std::boolalpha
          << "result.is_leaf()          = " << result.is_leaf() << "\n"
          << "result.detach().is_leaf() = " << d.is_leaf() << "\n";
```

### Output

```text
result.is_leaf()          = false
result.detach().is_leaf() = true
```

`d` shares `result`'s values but has no history — a useful way to feed a
computed quantity into a fresh graph as a constant.

Example 4. Only leaves keep sensitivity automatically after
[`reverse()`](reverse.md).

```cpp
auto w = pluss::make_series({2.0, 3.0});
w.tracks_slope_(true);
auto y = w.mul(w);            // interior result, not a leaf
y.sum().reverse();
std::cout << std::boolalpha
          << "w.is_leaf()=" << w.is_leaf()
          << " has_sensitivity=" << w.has_sensitivity() << "\n"
          << "y.is_leaf()=" << y.is_leaf()
          << " has_sensitivity=" << y.has_sensitivity() << "\n";
```

### Output

```text
w.is_leaf()=true has_sensitivity=true
y.is_leaf()=false has_sensitivity=false
```

`w` (a leaf) has its sensitivity; `y` (interior) does not — call
`y.retain_grad()` before `reverse()` if you need it.

## Gotchas And Failure Modes

- `is_leaf()` is **not** `tracks_slope()`. A leaf can be untracked (a
  constant); an interior result can track slope. They answer different
  questions.
- After `reverse()`, sensitivity is retained only on leaves. Use
  [`retain_grad()`](retain_grad.md) for an interior result you need the
  gradient of.
- [`detach()`](detach.md) produces a leaf; the original interior result is
  unchanged and stays non-leaf.
- Suspending recording ([`no_grad`](no_grad.md)) changes whether *new* ops
  acquire history — it does not retroactively turn an existing interior
  result into a leaf.

## Summary

`is_leaf()` reports a `Series`' position in the reverse-mode graph:

- `true` for a directly created tensor (tracked or not) and for a
  [`detach()`](detach.md)ed view
- `false` for the result of any op on a slope-tracking input
- orthogonal to [`tracks_slope()`](tracks_slope.md)
- leaves keep their sensitivity automatically after [`reverse()`](reverse.md);
  interior results need [`retain_grad()`](retain_grad.md)

## See Also

- [`Series::tracks_slope`](tracks_slope.md): whether gradient flows through
  this tensor — the orthogonal question.
- [`Series::retain_grad`](retain_grad.md): keep an interior result's
  sensitivity past `reverse()`.
- [`Series::detach`](detach.md): produce a history-free (leaf) view.
- [`Series::reverse`](reverse.md): the reverse pass whose retained
  sensitivities land on leaves.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_gpu.html">
    <span class="label">Previous</span>
    <span class="title">is_gpu</span>
  </a>
  <a class="next-link" href="is_mps.html">
    <span class="label">Next</span>
    <span class="title">is_mps</span>
  </a>
</div>
