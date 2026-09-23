<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: bb3064632490 -->

# Thread::csmin

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::csmin() const -> ThreadValue
```

## What It Does

Returns the **case label** of the row holding the smallest value in the thread
— not the value itself. `"cs"` stands for case: this is the "which row" answer
(like `idxmin` elsewhere), scanning every valid entry, comparing by the same
rule [`arrange()`](arrange.md) uses, and returning
[`caselbl()`](label.md) at the winning row's position. If the thread has no
explicit case labels, that's the default `0`-based position; if it does (via
[`label().set(...)`](label.md) or the `caselbl()` mutator), it's that label's
own value.

Ties keep the **first** occurrence — if two rows tie for the smallest value,
the earlier row's label is returned, not the later one. Works on every payload
kind (comparison is delegated to the same generic row-comparison logic
`arrange()` uses). Missing entries are skipped when scanning; an empty
thread, or one where every entry is missing, throws.

See [`Thread::sum`'s note on printing a `ThreadValue`](sum.md) — the examples
below use `.to_int()` (default integer labels) or `.to_str()` (explicit `Note`
labels) instead of streaming the `ThreadValue` directly.

## Parameters

`None`

## Returns

`ThreadValue`
    A scalar holding the case label of the minimum-valued row — an `Int64`
    position by default, or whatever payload kind the thread's own case labels
    use if explicit labels are set.

## Examples

Example 1. Without explicit case labels, `csmin()` returns the `0`-based
position of the smallest value.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({30.0, 10.0, 50.0, 20.0, 40.0}));
auto out = scores.csmin();
std::cout << out.to_int();
```

### Output

```text
1
```

`10.0` (the smallest value) sits at position `1` — not the value `10` itself,
the *position*.

Example 2. With explicit `Note` case labels attached, `csmin()` returns that
row's actual label text.

```cpp
auto labeled = scores.caselbl(Thread(pluss::make_note({"Ada", "Bo", "Cy", "Dee", "Evan"})));
auto out2 = labeled.csmin();
std::cout << out2.to_str();
```

### Output

```text
Bo
```

Position `1` (still where `10.0` lives) is now labeled `"Bo"` — that's what
comes back, not `1` and not `10`.

Example 3. Ties keep the first occurrence.

```cpp
auto tied = Thread(pluss::make_series({10.0, 10.0, 30.0}));
auto out3 = tied.csmin();
std::cout << out3.to_int();
```

### Output

```text
0
```

Positions `0` and `1` are tied at the minimum value `10.0`; the earlier one
(`0`) wins, not `1`.

Example 4. Missing entries are skipped when scanning, regardless of their
underlying stored value.

```cpp
auto with_gap = Thread(
    pluss::make_series({5.0, 1.0, 8.0}),
    pluss::make_series(std::vector<bool>{true, false, true})
);
auto out4 = with_gap.csmin();
std::cout << out4.to_int();
```

### Output

```text
0
```

Position `1` (stored value `1.0`, which would otherwise be the overall
smallest) is missing and skipped entirely — the minimum among the *observed*
rows is `5.0` at position `0`.

Example 5. An empty or entirely-missing thread throws.

```cpp
auto all_missing = Thread(
    pluss::make_series({1.0, 2.0}),
    pluss::make_series(std::vector<bool>{false, false})
);
try {
  auto bad = all_missing.csmin();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread::csmin on an all-null Thread
```

## Notes

- Returns the **case label**, not the value — pair with [`min()`](min.md) if
  you want the value itself.
- Ties keep the first occurrence.
- Works on every payload kind, since row comparison is delegated to the same
  logic [`arrange()`](arrange.md) uses.
- An empty thread throws `"Thread::csmin on an empty Thread"`; a thread with no
  observed values throws `"Thread::csmin on an all-null Thread"`.

## See Also

- [`Thread::csmax`](csmax.md): the case label of the largest-valued row.
- [`Thread::min`](min.md): the minimum value itself, rather than its row's
  label.
- [`Thread::label`](label.md): the full case-label accessor this method reads
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="csmax.html">
    <span class="label">Previous</span>
    <span class="title">csmax</span>
  </a>
  <a class="next-link" href="cummax.html">
    <span class="label">Next</span>
    <span class="title">cummax</span>
  </a>
</div>
