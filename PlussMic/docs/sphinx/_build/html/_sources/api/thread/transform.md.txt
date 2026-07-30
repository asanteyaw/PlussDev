<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c8fc531fed08 -->

# Thread::transform

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::transform(const std::function<ThreadValue(const ThreadValue&)>& fn) const -> Thread
Thread::transform(const std::function<Thread(const Thread&)>& fn, ResultShape shape = ResultShape::Reduce) const -> Thread
```

## What It Does

A dispatcher, not an independent implementation — `transform()` picks which functional
surface to use purely based on which overload you call:

- The **`ThreadValue`-callback overload** forwards straight to [`map()`](map.md): `fn` runs
  once per valid row.
- The **`Thread`-callback overload** forwards straight to [`fold()`](fold.md): `fn` runs
  once with the whole thread, and the result's size is checked against `shape`.

```{important} transform() has no behavior of its own
Both overloads are pure pass-throughs — `transform(threadvalue_fn)` is exactly
`map(threadvalue_fn)`, and `transform(thread_fn, shape)` is exactly `fold(thread_fn, shape)`,
argument for argument. Which overload C++ selects depends entirely on your callback's
signature: a lambda taking `const ThreadValue&` resolves to the `map()`-forwarding overload;
one taking `const Thread&` resolves to the `fold()`-forwarding overload. Read
[`map()`](map.md)'s and [`fold()`](fold.md)'s own pages for the full behavior — including
`fold()`'s important callout that it calls `fn` once with the whole thread, not once per
row despite `transform`'s row-suggestive name.
```

## Parameters

**fn**
    Either `const std::function<ThreadValue(const ThreadValue&)>&` (per-row, forwards to
    [`map()`](map.md)) or `const std::function<Thread(const Thread&)>&` (whole-thread,
    forwards to [`fold()`](fold.md)) — overload resolution picks based on the callback's
    parameter and return type.

**shape** (`ResultShape`, default `Reduce`, `Thread`-callback overload only)
    Forwarded unchanged to [`fold()`](fold.md) — see that page for what each value means.

## Returns

`Thread`
    Whatever the forwarded-to method ([`map()`](map.md) or [`fold()`](fold.md)) returns.

## Examples

Example 1. The `ThreadValue`-callback overload — identical behavior to calling
[`map()`](map.md) directly.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = scores.transform([](const ThreadValue& v) {
  return ThreadValue(Thread(pluss::make_series({v.to_dbl() * 2.0})));
});
std::cout << out;
```

### Output

```text
0  20
1  40
2  60
ptype: Series
```

Example 2. The `Thread`-callback overload with the default `shape` (`Reduce`) — identical
behavior to calling `fold(fn, ResultShape::Reduce)` directly.

```cpp
auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = scores.transform([](const Thread& t) {
  return Thread(pluss::make_series({t.sum().to_dbl()}));
});
std::cout << out;
```

### Output

```text
0  60
ptype: Series
```

Example 3. The `Thread`-callback overload with `ResultShape::Preserve` — identical
behavior to calling `fold(fn, ResultShape::Preserve)` directly.

```cpp
auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = scores.transform([](const Thread& t) {
  return t.astype(pluss::codem::ScalarType::Float64) * 2.0;
}, ResultShape::Preserve);
std::cout << out;
```

### Output

```text
0  20
1  40
2  60
ptype: Series
```

## Notes

- Purely a dispatcher — see the important callout above. There is no behavior documented
  here that isn't already documented on [`map()`](map.md) or [`fold()`](fold.md); this page
  exists to explain which overload maps to which.
- Overload selection is based on the callback's signature, not on any runtime flag — a
  lambda's parameter type (`const ThreadValue&` vs `const Thread&`) determines which
  underlying method actually runs.
- [`Thread::flow`](flow.md) is a related whole-thread transform but is **not** one of
  `transform()`'s targets — it has no `transform()`-overload equivalent.

## See Also

- [`Thread::map`](map.md): what the `ThreadValue`-callback overload forwards to — row-by-row
  transformation.
- [`Thread::fold`](fold.md): what the `Thread`-callback overload forwards to — whole-thread
  transformation with a size check.
- [`Thread::flow`](flow.md): whole-thread transformation with no size check at all, not
  reachable through `transform()`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="top.html">
    <span class="label">Previous</span>
    <span class="title">top</span>
  </a>
  <a class="next-link" href="truediv.html">
    <span class="label">Next</span>
    <span class="title">truediv</span>
  </a>
</div>
