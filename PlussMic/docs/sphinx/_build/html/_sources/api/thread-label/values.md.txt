<!-- pluss-hand-authored -->

# ThreadLabelAccessor::values

[Back to ThreadLabelAccessor API Reference](../../thread_label_accessor_api_reference)

## Signature

```cpp
ThreadLabelAccessor::values() const -> Thread
```

## What It Does

Returns the bound thread's case labels as a standalone `Thread` — a direct
forward to `Thread::caselbl()`. Case labels are a separate companion
sequence of row labels that can ride alongside a thread's own values (the same
concept `DataFrameLabelAccessor` exposes for a whole frame's leftmost display
column, just here attached to a single `Thread`).

## Parameters

`None`

## Returns

`Thread`
    The case labels, as a bare `Thread`. If no case labels have ever been set,
    this is the default integer range (`0, 1, 2, ...`).

## Examples

Example 1. Read the default range labels back, then attach custom labels and
read those instead.

```cpp
using namespace pluss::table;

auto sales = Thread(pluss::make_series({100.0, 200.0, 150.0}));
std::cout << sales.label().values();

auto tagged = sales.label().set(Thread(pluss::make_note({"a", "b", "c"})));
std::cout << tagged.label().values();
```

### Output (default range labels)

```text
0  0
1  1
2  2

ptype: Series
```

### Output (after set())

```text
0  a
1  b
2  c

ptype: Note
```

## Notes

- Always defined: every thread has case labels of some kind, even if they're
  just the default integer range.
- Identical to calling `Thread::caselbl()` directly.

## See Also

- [`ThreadLabelAccessor::set`](set.md): attach new case labels.
- [`ThreadLabelAccessor::has`](has.md): whether case labels have been
  explicitly set at all.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="unite.html">
    <span class="label">Previous</span>
    <span class="title">unite</span>
  </a>
</div>
