<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 912efca41ceb -->

# Thread::as_series

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::as_series() const -> const pluss::Series&
```

## What It Does

Exposes the underlying `pluss::Series` payload directly, by reference — the numeric escape
hatch for when you need the full `Series` surface (native reductions, elementwise
arithmetic, kernels that live below the table layer) rather than the curated subset
`Thread` re-exposes as its own methods. Requires the thread to be `Series`-kind; calling it
on a `Note`/`Timeline`/`TimeIndex` thread throws.

```{important} Returns a reference, not a copy — and does not rewrap into Thread
`as_series()` hands back `const pluss::Series&`, not a `Thread`. It does no rewrapping and
performs no computation of its own. If you run `Series`-level operations and want the
result usable by `Thread`-only methods again afterward, wrap it explicitly:
`Thread(scores.as_series() + 5.0)`. Forgetting this step is a common way to end up with a
`pluss::Series` where a `Thread` was expected.
```

## Parameters

`None`

## Returns

`const pluss::Series&`
    A reference to the underlying series payload, valid as long as the `Thread` itself
    stays alive.

## Examples

Example 1. Drop to `Series` for arithmetic, then wrap the result back into a `Thread`.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({-2.5, 0.0, 3.5}));

auto out = Thread(scores.as_series() + 5.0);
std::cout << out;
```

### Output

```text
0  2.5
1  5
2  8.5
ptype: Series
```

Example 2. Calling `as_series()` on a non-`Series` thread throws.

```cpp
auto names = Thread(pluss::make_note({"Ada", "Bola"}));
try {
  const auto& bad = names.as_series();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread: payload is not Series
```

## Notes

- Returns a reference into the thread's own storage — no copy, no allocation.
- Requires `Series`-kind; throws on every other payload kind.
- The result does not automatically become a `Thread` again — wrap it explicitly with
  `Thread(...)` when a `Thread`-only API needs it back.

## See Also

- [`Thread::as_note`](as_note.md) / [`as_timeline`](as_timeline.md): the equivalent escape hatch for the other three
  payload kinds.
- [`Thread::astype`](astype.md): stay on the `Thread` surface for a plain scalar-dtype
  conversion, instead of dropping to `Series` directly.
- [`Thread::kind`](kind.md): check the payload kind before calling a kind-specific
  accessor like this one.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="as_note.html">
    <span class="label">Previous</span>
    <span class="title">as_note</span>
  </a>
</div>
