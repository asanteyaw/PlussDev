<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 00f8fe5cc6f1 -->

# Thread::as_note

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::as_note() const -> const pluss::Note&
```

## What It Does

Exposes the underlying `pluss::Note` payload directly, by reference — the text escape
hatch for when you need `Note`'s own method surface directly rather than the subset
`Thread` re-exposes. Requires the thread to be `Note`-kind; every other payload kind
throws.

Same reference/no-rewrap contract as [`as_series()`](as_series.md) — see that page's
important callout for the full explanation.

## Parameters

`None`

## Returns

`const pluss::Note&`
    A reference to the underlying note payload, valid as long as the `Thread` itself
    stays alive.

## Examples

Example 1. Drop to `Note` for chained text cleanup, then wrap the result back into a
`Thread`.

```cpp
using namespace pluss::table;

auto names = Thread(pluss::make_note({" Ada ", "BOLA", " chen "}));

auto out = Thread(names.as_note().strip().lower());
std::cout << out;
```

### Output

```text
0  ada
1  bola
2  chen
ptype: Note
```

Example 2. Calling `as_note()` on a non-`Note` thread throws.

```cpp
auto numbers = Thread(pluss::make_series({1.0, 2.0}));
try {
  const auto& bad = numbers.as_note();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread: payload is not Note
```

## Notes

- Returns a reference into the thread's own storage — no copy, no allocation.
- Requires `Note`-kind; throws on every other payload kind.
- The result does not automatically become a `Thread` again — wrap it explicitly with
  `Thread(...)` when a `Thread`-only API needs it back.

## See Also

- [`Thread::as_series`](as_series.md): the same escape hatch for numeric payloads,
  including the full reference/no-rewrap explanation.
- [`Thread::as_timeline`](as_timeline.md): the
  equivalent accessors for the two temporal payload kinds.
- [`Thread::nt`](nt.md): the `Thread`-surface text accessor — prefer this in most
  application code; reach for `as_note()` only when you need a `Note`-only method `nt()`
  doesn't re-expose.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="arrange.html">
    <span class="label">Previous</span>
    <span class="title">arrange</span>
  </a>
  <a class="next-link" href="as_series.html">
    <span class="label">Next</span>
    <span class="title">as_series</span>
  </a>
</div>
