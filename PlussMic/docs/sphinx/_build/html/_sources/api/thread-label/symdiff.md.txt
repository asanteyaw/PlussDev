<!-- pluss-hand-authored -->

# ThreadLabelAccessor::symdiff

[Back to ThreadLabelAccessor API Reference](../../thread_label_accessor_api_reference)

## Signature

```cpp
ThreadLabelAccessor::symdiff(const Thread& other) const -> Thread
```

## What It Does

Treats the bound thread's case labels and `other` as two sets of values, and
returns the values present in **exactly one** of the two (the symmetric
difference) — deduplicated, in this accessor's first-occurrence order
followed by `other`'s.

## Parameters

**other** (`const Thread&`)

The values to compare against.

## Returns

`Thread`
    The distinct values found in exactly one of the two sides, `display_name`
    `"symdiff"`.

## Examples

Example 1. Symmetric difference between case labels with a repeat and
another set.

```cpp
using namespace pluss::table;

auto tagged = Thread(pluss::make_series({100.0, 200.0, 150.0, 90.0}))
                  .label().set(Thread(pluss::make_note({"east", "west", "east", "north"})));

auto other = Thread(pluss::make_note({"west", "south"}));
auto out = tagged.label().symdiff(other);
std::cout << out;
```

### Output

```text
0  east
1  north
2  south

ptype: Note
```

`"west"` is shared by both sides, so it's excluded; `"east"` and `"north"`
are only on the left, `"south"` only on the right — all three make it into
the result, left-side values first.

## Notes

- Unlike [`diff()`](diff.md), `symdiff()` is symmetric: swapping the operands
  produces the same set of values (order may differ).
- Duplicates on either side are collapsed; the result never repeats a value.

## See Also

- [`ThreadLabelAccessor::diff`](diff.md): the one-sided ("mine minus yours")
  set difference.
- [`ThreadLabelAccessor::unite`](unite.md),
  [`ThreadLabelAccessor::intersect`](intersect.md): the other set-algebra
  operations over case-label values.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sort.html">
    <span class="label">Previous</span>
    <span class="title">sort</span>
  </a>
  <a class="next-link" href="take.html">
    <span class="label">Next</span>
    <span class="title">take</span>
  </a>
</div>
