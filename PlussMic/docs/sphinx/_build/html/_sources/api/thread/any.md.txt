<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e36b288711cd -->

# Thread::any

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::any(bool skipnull = true) const -> ThreadValue
```

## What It Does

Returns whether **at least one** entry in a `Bool` `Series` thread is `true` —
a scalar existence test. Restricted to `Bool` `Series` payloads; any other kind
or dtype throws. With a validity mask present, missing entries are excluded
from consideration entirely (their underlying stored value, whatever it happens
to be, never contributes to the result) — `skipnull = false` combined with a
validity mask throws instead of attempting to define what "missing counts as
true or false" should mean.

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
    A scalar `Bool`: `true` if any observed entry is `true`, `false` otherwise
    (including for an all-`false` or entirely-missing thread).

## Examples

Example 1. At least one `true` value makes the result `true`.

```cpp
using namespace pluss::table;

auto flags = Thread(pluss::make_series(std::vector<bool>{true, false, true}));
auto out = flags.any();
std::cout << out.to_str();
```

### Output

```text
true
```

Example 2. A missing entry is excluded, even if its underlying stored value
happens to be `true` — it never spuriously flips the result.

```cpp
auto masked = Thread(
    pluss::make_series(std::vector<bool>{false, true, false}),
    pluss::make_series(std::vector<bool>{true, false, true})
);
auto out2 = masked.any();
std::cout << out2.to_str();
```

### Output

```text
false
```

Position `1` is missing; its stored `true` is excluded from consideration, so
only the two observed `false`s remain — the result is `false`, not `true`.

Example 3. `skipnull = false` together with a validity mask throws.

```cpp
try {
  auto bad = masked.any(false);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread::any(skipnull=false) is not supported for nullable Bool Series payload
```

Example 4. Restricted to `Bool` `Series` — any other dtype or payload kind
throws.

```cpp
auto numbers = Thread(pluss::make_series({1.0, 0.0}));
try {
  auto bad = numbers.any();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread::any is only valid for Bool Series payload
```

## Notes

- Missing entries are always excluded from consideration when a validity mask
  is present — their raw stored value never contributes.
- `skipnull = false` with a validity mask throws `"Thread::any(skipnull=false)
  is not supported for nullable Bool Series payload"`.
- Restricted to `Bool` `Series`; a non-`Series` kind throws `"Thread::any is
  only valid for Series payload"`, and a non-`Bool` `Series` throws
  `"Thread::any is only valid for Bool Series payload"`.

## See Also

- [`Thread::all`](all.md): require every entry to be `true`, instead of just
  one.
- [`Thread::count`](count.md): count `true`/`false` entries instead of a
  single existence test.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="all.html">
    <span class="label">Previous</span>
    <span class="title">all</span>
  </a>
  <a class="next-link" href="append.html">
    <span class="label">Next</span>
    <span class="title">append</span>
  </a>
</div>
