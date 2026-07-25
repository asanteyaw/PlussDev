<!-- pluss-hand-authored -->

# Thread::ndistinct

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::ndistinct(bool dropnull = true) const -> ThreadValue
```

## What It Does

Returns the number of distinct values in the thread, as a scalar
`ThreadValue`. This is exactly `distinct(dropnull).size()` wrapped as a scalar
— the same grouping rule (each value's string form) and the same `dropnull`
convention as [`distinct()`](distinct.md) apply here; only the return shape
differs (a count instead of the values themselves).

See [`Thread::sum`'s note on printing a `ThreadValue`](sum.md) — streaming the
result directly (`std::cout << out`) prints its full diagnostic form, not a
bare number; both examples below use `.to_int()` for a clean integer instead.

## Parameters

**dropnull** (`bool`, default `true`)
    - `true` (the default): missing entries don't count toward the distinct
      total.
    - `false`: a missing entry counts as one additional distinct "value" (all
      missing entries collapse into that single extra count, they don't count
      individually).

## Returns

`ThreadValue`
    A scalar `Int64` holding the distinct-value count.

## Examples

Example 1. Count distinct values.

```cpp
using namespace pluss::table;

auto cities = Thread(pluss::make_note({"Accra", "Tema", "Accra", "Kumasi"}));
auto out = cities.ndistinct();
std::cout << out.to_int();
```

### Output

```text
3
```

Example 2. `dropnull = false` adds one to the count if any entry is missing —
regardless of how many are missing.

```cpp
auto with_gaps = Thread(
    pluss::make_note({"Accra", "", "Tema", ""}),
    pluss::make_series(std::vector<bool>{true, false, true, false})
);
auto out2 = with_gaps.ndistinct(false);
std::cout << out2.to_int();
```

### Output

```text
3
```

Two entries are missing, but they still only add `1` to the total (`"Accra"`,
`"Tema"`, plus one slot for `NULL`) — not `2`.

## Notes

- Equivalent to `distinct(dropnull).size()`; the same first-seen grouping rule
  applies, though `ndistinct()` doesn't preserve or expose the order.
- With `dropnull = false`, any number of missing entries contributes at most
  `1` to the total, never one per occurrence.

## See Also

- [`Thread::distinct`](distinct.md): the distinct values themselves, in
  first-seen order.
- [`Thread::tally`](tally.md): a full per-value frequency breakdown instead of
  a single count.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ncases.html">
    <span class="label">Previous</span>
    <span class="title">ncases</span>
  </a>
  <a class="next-link" href="ne.html">
    <span class="label">Next</span>
    <span class="title">ne</span>
  </a>
</div>
