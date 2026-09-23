<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4412d242dbe4 -->

# Thread::fold

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::fold(const std::function<Thread(const Thread&)>& fn, ResultShape shape = ResultShape::Reduce) const -> Thread
```

## What It Does

Calls `fn` **exactly once**, passing the entire current thread, and returns whatever `fn`
returns — after checking the result's size against `shape`. Despite the name, this is
**not** a row-by-row accumulator-style fold (there's no running accumulator, and `fn` never
sees individual rows in isolation) — it's a whole-thread escape hatch for a transform that
doesn't fit `Thread`'s built-in vectorized methods, with `shape` documenting (and
enforcing) what kind of result you're expected to produce.

```cpp
enum class ResultShape : std::uint8_t {
  Infer = 0,     // no size check
  Reduce = 1,    // result must have exactly one row
  Preserve = 2,  // result must have the same number of rows as the input
  Expand = 3,    // no size check
};
```

```{important} fold() calls fn() once with the whole thread — it does not iterate rows
If you want `fn` invoked once per row instead, with a running per-row callback, use
[`map()`](map.md) — its `ThreadValue(const ThreadValue&)` callback runs once for each valid
row. `fold()`'s `shape` parameter only validates the *size* of whatever `fn` returns after
that single call; it does not change how many times `fn` is invoked.
```

## Parameters

**fn** (`const std::function<Thread(const Thread&)>&`)
    Called exactly once with `*this`. Must return a defined `Thread`; must not be empty.

**shape** (`ResultShape`, default `Reduce`)
    What size the returned `Thread` is expected to have. `Reduce` requires exactly one row
    (throws otherwise); `Preserve` requires the same row count as the input (throws
    otherwise); `Infer` and `Expand` perform no size check at all.

## Returns

`Thread`
    Exactly what `fn` returned, once its size has been checked against `shape`.

## Examples

Example 1. `ResultShape::Reduce` (the default) — collapse the whole thread to one value.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = scores.fold([](const Thread& t) {
  return Thread(pluss::make_series({t.sum().to_dbl()}));
}, ResultShape::Reduce);
std::cout << out;
```

### Output

```text
0  60
ptype: Series
```

Example 2. `ResultShape::Preserve` — the result must come back the same length as the
input; here, doubling every value naturally satisfies that.

```cpp
auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = scores.fold([](const Thread& t) {
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

Example 3. `ResultShape::Infer` performs no size check at all — a result of any length is
accepted, even one that neither reduces to one row nor preserves the input length.

```cpp
auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = scores.fold([](const Thread& t) {
  return Thread(pluss::make_series({1.0, 2.0}));
}, ResultShape::Infer);
std::cout << out;
```

### Output

```text
0  1
1  2
ptype: Series
```

Example 4. A size mismatch against the requested `shape` throws — here, a two-row result
against the default `Reduce` (which requires exactly one row).

```cpp
auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0}));
try {
  auto out = scores.fold([](const Thread& t) {
    return Thread(pluss::make_series({1.0, 2.0}));
  });
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread::fold: Reduce requires one value
```

## Notes

- `fn` is called exactly once, with the whole thread — not once per row. See the important
  callout above; use [`map()`](map.md) for genuine row-by-row iteration.
- `shape` only validates the *size* of `fn`'s return value; it never changes how `fn` is
  invoked or how many times.
- `Infer` and `Expand` currently behave identically (no size check) — pick whichever name
  reads more clearly at the call site.
- [`Thread::transform`](transform.md)'s `Thread`-callback overload is a pure alias for
  `fold()`, forwarding `fn` and `shape` unchanged.

## See Also

- [`Thread::flow`](flow.md): the same "call `fn` once with the whole thread" shape, but with
  no size validation at all — not even the loose `Infer`/`Expand` bookkeeping.
- [`Thread::map`](map.md): the row-by-row counterpart — calls `fn` once per valid row
  instead of once for the whole thread.
- [`Thread::transform`](transform.md): dispatches to `fold()` for the `Thread`-callback
  overload.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="flow.html">
    <span class="label">Previous</span>
    <span class="title">flow</span>
  </a>
  <a class="next-link" href="freq.html">
    <span class="label">Next</span>
    <span class="title">freq</span>
  </a>
</div>
