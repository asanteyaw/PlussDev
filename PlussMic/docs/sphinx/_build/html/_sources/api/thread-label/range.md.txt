<!-- pluss-hand-authored -->

# ThreadLabelAccessor::range

[Back to ThreadLabelAccessor API Reference](../../thread_label_accessor_api_reference)

## Signature

```cpp
ThreadLabelAccessor::range() const -> Thread
```

## What It Does

Replaces the bound thread's case labels with an explicit default integer
range (`0, 1, 2, ...`) — a direct forward to `Thread::rangelbl()`. Visually
identical to a thread that never had case labels set at all, but — unlike
[`reset()`](reset.md) — [`has()`](has.md) reports `true` afterward, since
`range()` actively attaches a labels thread rather than clearing one.

## Parameters

`None`

## Returns

`Thread`
    A new thread, holding the same values as `*this`, with an explicit
    default integer range as its case labels.

## Examples

Example 1. Reset custom labels back to a plain range, and confirm `has()`
still reports `true`.

```cpp
using namespace pluss::table;

auto tagged = Thread(pluss::make_series({100.0, 200.0, 150.0}))
                  .label().set(Thread(pluss::make_note({"a", "b", "c"})));
std::cout << tagged;

auto ranged = tagged.label().range();
std::cout << ranged;
std::cout << ranged.label().has();
```

### Output (before)

```text
a  100
b  200
c  150

ptype: Series
```

### Output (after range())

```text
0  100
1  200
2  150

ptype: Series
```

### Output (has())

```text
true
```

The display now looks like a thread with no case labels at all, but `has()`
still reports `true` — `range()` actively attached a labels thread, it didn't
clear one. Contrast with [`reset()`](reset.md), which produces the same
display but leaves `has()` `false`.

## Notes

- Produces the same visible `0, 1, 2, ...` display as a never-labeled thread,
  but [`has()`](has.md) reports `true` afterward — see
  [`reset()`](reset.md) for the version that leaves `has()` `false`.
- Identical to calling `Thread::rangelbl()` directly.

## See Also

- [`ThreadLabelAccessor::reset`](reset.md): visually identical result, but
  clears case labels entirely rather than attaching a range.
- [`ThreadLabelAccessor::set`](set.md): attach arbitrary case labels instead
  of the default range.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pos.html">
    <span class="label">Previous</span>
    <span class="title">pos</span>
  </a>
  <a class="next-link" href="repeated.html">
    <span class="label">Next</span>
    <span class="title">repeated</span>
  </a>
</div>
