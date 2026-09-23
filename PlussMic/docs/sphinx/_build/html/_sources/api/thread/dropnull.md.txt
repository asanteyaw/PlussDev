<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6fcd1fc23531 -->

# Thread::dropnull

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::dropnull() const -> Thread
```

## What It Does

Returns a new, compacted thread containing only the entries **not** marked missing
by the validity mask. If the thread carries no validity mask at all, this is a
no-op — the thread is returned unchanged (there's nothing to drop). Otherwise, it
gathers the indices of every valid entry and calls [`take()`](take.md) with them,
so the result is a genuinely shorter thread (no null holes left in place) rather
than the original length with gaps. The result never carries a leftover validity
mask, even if `take()`'s bookkeeping would otherwise attach one — `dropnull()`
explicitly clears it, so a fully-valid result reports no mask at all.

```{note} mask-driven, not NaN-driven
`dropnull()` inspects the thread's validity mask only. A plain `NaN` inside a
numeric `Series` is not, by itself, "missing" to `dropnull()` unless that position
is also marked invalid in the mask — see [`isnull()`](isnull.md) for the same
mask-vs-`NaN` distinction.
```

## Parameters

`None`

## Returns

`Thread`
    A new thread holding only the valid entries, compacted (re-indexed from `0`),
    with no validity mask attached.

## Examples

Example 1. Drop the entries marked missing.

```cpp
using namespace pluss::table;

auto values = Thread(
    pluss::make_series({10.0, 20.0, 30.0, 40.0}),
    pluss::make_series(std::vector<bool>{true, false, true, false})
);
auto out = values.dropnull();
std::cout << out;
```

### Output

```text
0  10
1  30

ptype: Series
```

Only the two entries marked valid (positions `0` and `2`) survive, and the result
is re-indexed from `0` — not left with gaps at the old positions `1`/`3`.

Example 2. No validity mask at all is a no-op — the thread comes back unchanged.

```cpp
auto plain = Thread(pluss::make_series({1.0, 2.0, 3.0}));
auto out2 = plain.dropnull();
std::cout << out2.size();
```

### Output

```text
3
```

Example 3. Every entry marked invalid drops the whole thread to empty.

```cpp
auto all_missing = Thread(
    pluss::make_series({1.0, 2.0, 3.0}),
    pluss::make_series(std::vector<bool>{false, false, false})
);
auto out3 = all_missing.dropnull();
std::cout << out3;
```

### Output

```text
ptype: Series
```

## Notes

- No-op when there's no validity mask — returns the thread unchanged, not a copy
  with a fresh empty mask.
- The result is compacted: surviving entries are re-indexed, never left with holes
  at their old positions.
- The result never carries a leftover validity mask, even when every surviving
  entry happens to be valid.
- Driven entirely by the validity mask, not by `NaN`/sentinel values inside the
  payload — see the note above.

## See Also

- [`Thread::isnull`](isnull.md) / [`Thread::notnull`](notnull.md): inspect the
  mask without dropping anything.
- [`Thread::fillnull`](fillnull.md): replace missing entries instead of removing
  them.
- [`Thread::take`](take.md): the underlying selection mechanism this method uses.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="div.html">
    <span class="label">Previous</span>
    <span class="title">div</span>
  </a>
  <a class="next-link" href="dtype.html">
    <span class="label">Next</span>
    <span class="title">dtype</span>
  </a>
</div>
