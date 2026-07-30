<!-- pluss-hand-authored -->

# ThreadLabelAccessor::monotonic

[Back to ThreadLabelAccessor API Reference](../../thread_label_accessor_api_reference)

## Signature

```cpp
ThreadLabelAccessor::monotonic(bool ascending = true, bool strict = false, bool nulls_last = true) const -> bool
```

## What It Does

Checks whether the bound thread's case labels are sorted, according to
`ascending` and `strict`.

## Parameters

**ascending** (`bool`, default `true`)

Check ascending order if `true`, descending if `false`.

**strict** (`bool`, default `false`)

If `true`, equal adjacent case labels fail the check (strictly
increasing/decreasing); if `false`, equal adjacent values are allowed.

**nulls_last** (`bool`, default `true`)

Where a null case label is expected to sort relative to real values.

## Returns

`bool`
    Whether the case labels satisfy the requested order.

## Examples

Example 1. An unsorted case-label axis, and a sorted rearrangement of the
same values.

```cpp
using namespace pluss::table;

auto tagged = Thread(pluss::make_series({100.0, 200.0, 150.0, 90.0}))
                  .label().set(Thread(pluss::make_note({"east", "west", "east", "north"})));

std::cout << tagged.label().monotonic();

auto sorted = tagged.label().sort();
std::cout << sorted.label().monotonic();
```

### Output

```text
false
true
```

`tagged`'s case labels (`"east"`, `"west"`, `"east"`, `"north"`) decrease from
`"west"` back down to `"east"` at row `2`, so they aren't sorted ascending.
After [`sort()`](sort.md) rearranges the rows, `monotonic()` is `true`.

Example 2. Use `strict = true` to reject equal adjacent case labels.

```cpp
auto with_repeat = Thread(pluss::make_series({100.0, 200.0, 150.0, 90.0}))
                        .label().set(Thread(pluss::make_note({"east", "east", "north", "west"})));

std::cout << with_repeat.label().monotonic();
std::cout << with_repeat.label().monotonic(true, true);
```

### Output

```text
true
false
```

The default `strict = false` allows the equal adjacent pair (`"east"`,
`"east"`), so `monotonic()` is `true`. Passing `strict = true` rejects that
same equal pair.

Example 3. Pass `nulls_last = false` to change where a null case label is
expected to sort.

```cpp
auto with_null = Thread(pluss::make_series({1.0, 2.0, 3.0}))
                      .label().set(Thread(pluss::make_series(
                          {10.0, 20.0, std::numeric_limits<double>::quiet_NaN()})));

std::cout << with_null.label().monotonic();
std::cout << with_null.label().monotonic(true, false, false);
```

### Output

```text
true
false
```

With the default `nulls_last = true`, the trailing `NULL` case label is
treated as sorting after every real value, so the ascending check holds.
Passing `nulls_last = false` instead expects nulls to sort *first*; since the
`NULL` here comes last, the check fails.

## Notes

- Operates on the bound thread's case labels, not its own data.

## See Also

- [`ThreadLabelAccessor::sort`](sort.md): make the case labels monotonic.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="map.html">
    <span class="label">Previous</span>
    <span class="title">map</span>
  </a>
  <a class="next-link" href="notnull.html">
    <span class="label">Next</span>
    <span class="title">notnull</span>
  </a>
</div>
