<!-- pluss-hand-authored -->

# Thread::csmax

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::csmax() const -> ThreadValue
```

## What It Does

Returns the **case label** of the row holding the largest value in the thread
— not the value itself. The mirror image of [`csmin()`](csmin.md): `"cs"`
stands for case, so this is the "which row" answer (like `idxmax` elsewhere).
If the thread has no explicit case labels, that's the default `0`-based
position; if it does, it's that label's own value.

Ties keep the **first** occurrence — if two rows tie for the largest value, the
earlier row's label is returned. Works on every payload kind. Missing entries
are skipped when scanning; an empty thread, or one where every entry is
missing, throws.

See [`Thread::sum`'s note on printing a `ThreadValue`](sum.md) — the examples
below use `.to_int()` (default integer labels) or `.to_str()` (explicit `Note`
labels) instead of streaming the `ThreadValue` directly.

## Parameters

`None`

## Returns

`ThreadValue`
    A scalar holding the case label of the maximum-valued row — an `Int64`
    position by default, or whatever payload kind the thread's own case labels
    use if explicit labels are set.

## Examples

Example 1. Without explicit case labels, `csmax()` returns the `0`-based
position of the largest value.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({30.0, 10.0, 50.0, 20.0, 40.0}));
auto out = scores.csmax();
std::cout << out.to_int();
```

### Output

```text
2
```

`50.0` (the largest value) sits at position `2` — the position, not the value.

Example 2. With explicit `Note` case labels attached, `csmax()` returns that
row's actual label text.

```cpp
auto labeled = scores.caselbl(Thread(pluss::make_note({"Ada", "Bo", "Cy", "Dee", "Evan"})));
auto out2 = labeled.csmax();
std::cout << out2.to_str();
```

### Output

```text
Cy
```

Position `2` (still where `50.0` lives) is now labeled `"Cy"`.

Example 3. Ties keep the first occurrence.

```cpp
auto tied = Thread(pluss::make_series({30.0, 30.0, 10.0}));
auto out3 = tied.csmax();
std::cout << out3.to_int();
```

### Output

```text
0
```

Positions `0` and `1` are tied at the maximum value `30.0`; the earlier one
(`0`) wins.

Example 4. Missing entries are skipped when scanning, regardless of their
underlying stored value.

```cpp
auto with_gap = Thread(
    pluss::make_series({5.0, 100.0, 8.0}),
    pluss::make_series(std::vector<bool>{true, false, true})
);
auto out4 = with_gap.csmax();
std::cout << out4.to_int();
```

### Output

```text
2
```

Position `1` (stored value `100.0`, which would otherwise be the overall
largest) is missing and skipped entirely — the maximum among the *observed*
rows is `8.0` at position `2`.

Example 5. An empty or entirely-missing thread throws.

```cpp
auto all_missing = Thread(
    pluss::make_series({1.0, 2.0}),
    pluss::make_series(std::vector<bool>{false, false})
);
try {
  auto bad = all_missing.csmax();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread::csmax on an all-null Thread
```

## Notes

- Returns the **case label**, not the value — pair with [`max()`](max.md) if
  you want the value itself.
- Ties keep the first occurrence.
- Works on every payload kind, since row comparison is delegated to the same
  logic [`sort_values()`](sort_values.md) uses.
- An empty thread throws `"Thread::csmax on an empty Thread"`; a thread with no
  observed values throws `"Thread::csmax on an all-null Thread"`.

## See Also

- [`Thread::csmin`](csmin.md): the case label of the smallest-valued row.
- [`Thread::max`](max.md): the maximum value itself, rather than its row's
  label.
- [`Thread::label`](label.md): the full case-label accessor this method reads
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="covar.html">
    <span class="label">Previous</span>
    <span class="title">covar</span>
  </a>
  <a class="next-link" href="csmin.html">
    <span class="label">Next</span>
    <span class="title">csmin</span>
  </a>
</div>
