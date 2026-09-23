<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d6a417c01668 -->

# Thread::all

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::all(bool skipnull = true) const -> ThreadValue
```

## What It Does

Returns whether **every** entry in a `Bool` `Series` thread is `true` — the
scalar universal-truth counterpart to [`any()`](any.md). Restricted to `Bool`
`Series` payloads; any other kind or dtype throws. With a validity mask
present, missing entries are excluded from consideration entirely (their
underlying stored value, whatever it happens to be, never contributes to the
result) — `skipnull = false` combined with a validity mask throws.

See [`Thread::sum`'s note on printing a `ThreadValue`](sum.md) — every example
below uses `.to_str()` to print a clean `"true"`/`"false"` instead of streaming
the `ThreadValue` directly.

## Parameters

**skipnull** (`bool`, default `true`)
    Must stay `true` when the thread has a validity mask — missing entries are
    always excluded from the reduction when a mask is present. `skipnull =
    false` together with a validity mask throws.

## Returns

`ThreadValue`
    A scalar `Bool`: `true` only if every observed entry is `true` (an empty or
    entirely-missing thread reports `true` — vacuously, there's no observed
    `false` to disprove it).

## Examples

Example 1. One `false` value makes the result `false`.

```cpp
using namespace pluss::table;

auto flags = Thread(pluss::make_series(std::vector<bool>{true, false, true}));
auto out = flags.all();
std::cout << out.to_str();
```

### Output

```text
false
```

Example 2. A missing entry is excluded, even if its underlying stored value
happens to be `false` — it never spuriously breaks the result.

```cpp
auto masked = Thread(
    pluss::make_series(std::vector<bool>{true, false, true}),
    pluss::make_series(std::vector<bool>{true, false, true})
);
auto out2 = masked.all();
std::cout << out2.to_str();
```

### Output

```text
true
```

Position `1` is missing; its stored `false` is excluded from consideration, so
only the two observed `true`s remain — the result is `true`, not `false`.

Example 3. `skipnull = false` together with a validity mask throws.

```cpp
try {
  auto bad = masked.all(false);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread::all(skipnull=false) is not supported for nullable Bool Series payload
```

Example 4. Restricted to `Bool` `Series` — any other dtype or payload kind
throws.

```cpp
auto numbers = Thread(pluss::make_series({1.0, 0.0}));
try {
  auto bad = numbers.all();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread::all is only valid for Bool Series payload
```

## Notes

- Missing entries are always excluded from consideration when a validity mask
  is present — their raw stored value never contributes.
- An empty or entirely-missing thread reports `true` (vacuous truth), not
  `false`.
- `skipnull = false` with a validity mask throws `"Thread::all(skipnull=false)
  is not supported for nullable Bool Series payload"`.
- Restricted to `Bool` `Series`; a non-`Series` kind throws `"Thread::all is
  only valid for Series payload"`, and a non-`Bool` `Series` throws
  `"Thread::all is only valid for Bool Series payload"`.

## See Also

- [`Thread::any`](any.md): require only one entry to be `true`, instead of
  every entry.
- [`Thread::count`](count.md): count `true`/`false` entries instead of a
  single universal-truth test.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="align.html">
    <span class="label">Previous</span>
    <span class="title">align</span>
  </a>
  <a class="next-link" href="any.html">
    <span class="label">Next</span>
    <span class="title">any</span>
  </a>
</div>
