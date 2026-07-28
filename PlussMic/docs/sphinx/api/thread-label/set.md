<!-- pluss-hand-authored -->

# ThreadLabelAccessor::set

[Back to ThreadLabelAccessor API Reference](../../thread_label_accessor_api_reference)

## Signature

```cpp
ThreadLabelAccessor::set(const Thread& labels) const -> Thread
```

## What It Does

Replaces the bound thread's case labels with `labels`, used as-is — a direct
forward to `Thread::caselbl(labels)`. The thread's own values are completely
unchanged; only the companion case-label sequence shown alongside them (and
used by [`sort()`](sort.md), [`slice()`](slice.md), [`pos()`](pos.md), and
similar) is replaced.

## Parameters

**labels** (`const Thread&`)

The replacement case labels. Must be the same length as the bound thread.

## Returns

`Thread`
    A new thread, holding the same values as `*this`, with `labels` as its
    case labels.

## Examples

Example 1. Attach text labels to a numeric thread.

```cpp
using namespace pluss::table;

auto sales = Thread(pluss::make_series({100.0, 200.0, 150.0, 90.0}));
auto tagged = sales.label().set(
    Thread(pluss::make_note({"east", "west", "east", "north"})));
std::cout << tagged;
```

### Output

```text
east   100
west   200
east   150
north  90

ptype: Series
```

The values are exactly `sales`'s own — only the leftmost display column (the
case labels) changed.

## Notes

- The thread's own values and kind are never touched — only the case-label
  companion sequence changes.
- Identical to calling `Thread::caselbl(labels)` directly.
- Unlike `DataFrameLabelAccessor::set`, there is no feature-promotion overload
  here — a bare `Thread` has no named features to promote, so this is the
  only form.

## See Also

- [`ThreadLabelAccessor::reset`](reset.md): the inverse — clears case labels
  back to the default integer range.
- [`ThreadLabelAccessor::range`](range.md): explicitly reset to the default
  range labels.
- [`ThreadLabelAccessor::values`](values.md): read the current case labels
  back out.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="reset.html">
    <span class="label">Previous</span>
    <span class="title">reset</span>
  </a>
  <a class="next-link" href="slice.html">
    <span class="label">Next</span>
    <span class="title">slice</span>
  </a>
</div>
