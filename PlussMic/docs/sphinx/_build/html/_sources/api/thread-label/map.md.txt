<!-- pluss-hand-authored -->

# ThreadLabelAccessor::map

[Back to ThreadLabelAccessor API Reference](../../thread_label_accessor_api_reference)

## Signature

```cpp
ThreadLabelAccessor::map(const std::function<ThreadValue(const ThreadValue&)>& fn) const -> Thread
```

## What It Does

Applies `fn` to every case label individually and reattaches the results as
the bound thread's new case labels — a direct forward to
`Thread::caselbl(Thread::caselbl().map(fn))`. The thread's own values are
untouched; only the case-label companion sequence changes.

```{note} Always reachable — unlike DataFrameLabelAccessor::map
`DataFrameLabelAccessor::map` throws on a structured (multi-level) frame,
since there's no single flat thread to map over. A `Thread`'s case labels
are always a single flat sequence, so `ThreadLabelAccessor::map` never has
that concern.
```

## Parameters

**fn** (`const std::function<ThreadValue(const ThreadValue&)>&`)

A callback receiving one case label at a time (wrapped in a `ThreadValue`)
and returning its replacement.

## Returns

`Thread`
    A new thread, holding the same values as `*this`, with case labels
    replaced by `fn`'s output.

## Examples

Example 1. Append a suffix to every case label.

```cpp
using namespace pluss::table;

auto tagged = Thread(pluss::make_series({100.0, 200.0, 150.0, 90.0}))
                  .label().set(Thread(pluss::make_note({"r1", "r2", "r3", "r4"})));
std::cout << tagged;

auto out = tagged.label().map([](const ThreadValue& v) {
  return ThreadValue(Thread(pluss::make_note({v.to_str() + "!"})));
});
std::cout << out;
```

### Output (before)

```text
r1  100
r2  200
r3  150
r4  90

ptype: Series
```

### Output (after)

```text
r1!  100
r2!  200
r3!  150
r4!  90

ptype: Series
```

Each case label is transformed by appending `"!"`; the thread's own `sales`-
style values are exactly as they were.

## Notes

- `fn` must return a `ThreadValue` of a kind consistent with the rest of the
  output (a length-1 `Thread` wrapped in `ThreadValue`, as constructed above).
- Only the case labels change — the bound thread's own values are untouched.

## See Also

- [`ThreadLabelAccessor::values`](values.md): pull the raw label thread if a
  transform outside `map(...)` is needed.
- [`ThreadLabelAccessor::set`](set.md): the underlying mechanism this method
  applies its result through.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isnull.html">
    <span class="label">Previous</span>
    <span class="title">isnull</span>
  </a>
  <a class="next-link" href="monotonic.html">
    <span class="label">Next</span>
    <span class="title">monotonic</span>
  </a>
</div>
