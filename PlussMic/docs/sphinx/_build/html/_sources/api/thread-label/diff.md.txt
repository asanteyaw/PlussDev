<!-- pluss-hand-authored -->

# ThreadLabelAccessor::diff

[Back to ThreadLabelAccessor API Reference](../../thread_label_accessor_api_reference)

## Signature

```cpp
ThreadLabelAccessor::diff(const Thread& other) const -> Thread
```

## What It Does

Treats the bound thread's case labels and `other` as two sets of values, and
returns the case-label values present in the case labels but **absent** from
`other` — deduplicated, in the case labels' own first-occurrence order.

## Parameters

**other** (`const Thread&`)

The values to subtract.

## Returns

`Thread`
    The distinct case-label values not found in `other`, in first-occurrence
    order, `display_name` `"diff"`.

## Examples

Example 1. Subtract a target set from case labels with a repeat.

```cpp
using namespace pluss::table;

auto tagged = Thread(pluss::make_series({100.0, 200.0, 150.0, 90.0}))
                  .label().set(Thread(pluss::make_note({"east", "west", "east", "north"})));

auto other = Thread(pluss::make_note({"west", "south"}));
auto out = tagged.label().diff(other);
std::cout << out;
```

### Output

```text
0  east
1  north

ptype: Note
```

`"west"` is removed since it appears in `other`; `"east"` (deduplicated to one
entry) and `"north"` remain, in the order they first appear among the case
labels.

## Notes

- Duplicates in the case labels are collapsed; the result never repeats a
  value.
- Not symmetric: `a.label().diff(b)` and `b.diff(a)` generally differ, unlike
  [`intersect()`](intersect.md) which is symmetric in content.
- When the two operands share a common representable dtype (same `Series`
  scalar type, both numeric, or the same non-`Series` kind on the same
  platform), the result preserves that dtype and takes directly from the case
  labels; otherwise it falls back to a `Note` of each value's string form.

## See Also

- [`ThreadLabelAccessor::unite`](unite.md),
  [`ThreadLabelAccessor::intersect`](intersect.md),
  [`ThreadLabelAccessor::symdiff`](symdiff.md): the other set-algebra
  operations over case-label values.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="align.html">
    <span class="label">Previous</span>
    <span class="title">align</span>
  </a>
  <a class="next-link" href="distinct.html">
    <span class="label">Next</span>
    <span class="title">distinct</span>
  </a>
</div>
