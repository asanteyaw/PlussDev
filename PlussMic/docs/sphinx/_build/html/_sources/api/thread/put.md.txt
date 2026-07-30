<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: bd526418c5fe -->

# Thread::put

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::put(const Thread& bool_mask, const Thread& value) const -> Thread
Thread::put(const Thread& bool_mask, double value) const -> Thread
Thread::put(const Thread& bool_mask, std::string_view value) const -> Thread
Thread::put(const Thread& bool_mask, const char* value) const -> Thread
Thread::put(const Thread& bool_mask, pluss::null_t value) const -> Thread
Thread::put(const pluss::Series& bool_mask, const Thread& value) const -> Thread
Thread::put(const pluss::Series& bool_mask, double value) const -> Thread
Thread::put(const pluss::Series& bool_mask, std::string_view value) const -> Thread
Thread::put(const pluss::Series& bool_mask, const char* value) const -> Thread
Thread::put(const pluss::Series& bool_mask, pluss::null_t value) const -> Thread
```

## What It Does

Writes a replacement value into every row where `bool_mask` is `true`, leaving every other
row exactly as it was — a masked, non-mutating write (`put()` returns a new `Thread`;
`*this` is untouched). The `pluss::Series` overloads are pure convenience wrappers that
just call `Thread(bool_mask)` and forward to the `Thread`-mask overload.

Internally, `put(mask, value)` is a thin wrapper over
[`Thread::select`](select.md) — it's exactly
`Thread::select({mask}, {value}, *this)`: wherever the mask is true, use `value`;
everywhere else, fall back to the original thread. Understanding `put()` this way explains
its broadcast rule for free: a scalar `value` (numeric, text, or a size-1 `Thread`)
broadcasts to every masked row; a same-length `Thread` value supplies one replacement per
row instead.

```{important} put(mask, pluss::null) takes a different code path than every other overload
Unlike the value-writing overloads, `put(mask, pluss::null)` doesn't go through `select()`
at all — it directly nulls out the masked rows' validity mask, keeping their underlying
stored values untouched (just marked invalid). This is the masked equivalent of
[`fillnull`](fillnull.md) run in reverse — see Example 4.
```

## Parameters

**bool_mask** (`const Thread&` or `const pluss::Series&`)
    A boolean row mask, the same length as `*this`. `true` marks a row for replacement;
    `false` leaves it untouched. The `Series` overloads are equivalent to wrapping the mask
    in `Thread(bool_mask)` first.

**value** (`const Thread&` / `double` / `std::string_view` / `const char*` / `pluss::null_t`)
    The replacement written into every masked row. A scalar (`double`/text/`pluss::null`)
    or a size-1 `Thread` broadcasts to every masked row; a `Thread` matching `*this`'s
    length supplies one replacement value per row instead.

## Returns

`Thread`
    A new thread the same length as `*this`, with masked rows replaced and every other row
    unchanged.

## Examples

Example 1. Replace every row above a threshold with a fixed scalar.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto mask = scores.compare(15.0, ThreadBinaryCompareOp::Gt);

auto out = scores.put(mask, 0.0);
std::cout << out;
```

### Output

```text
0  10
1  0
2  0
ptype: Series
```

Example 2. Replace masked rows with a **rowwise** `Thread` of replacement values — row `i`
of `value` supplies row `i`'s replacement, rather than one value broadcasting everywhere.

```cpp
auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto mask = scores.compare(15.0, ThreadBinaryCompareOp::Gt);
auto replacements = Thread(pluss::make_series({-1.0, -2.0, -3.0}));

auto out = scores.put(mask, replacements);
std::cout << out;
```

### Output

```text
0  10
1  -2
2  -3
ptype: Series
```

Example 3. Replace masked rows in a `Note`-kind thread with a text scalar — useful for
redaction-style rewrites.

```cpp
auto names = Thread(pluss::make_note({"Ada", "Bola", "Chen"}));
auto mask = names.compare("Bola", ThreadBinaryCompareOp::Eq);

auto out = names.put(mask, "REDACTED");
std::cout << out;
```

### Output

```text
0  Ada
1  REDACTED
2  Chen
ptype: Note
```

Example 4. `put(mask, pluss::null)` nulls out masked rows instead of writing a value —
see the important callout above.

```cpp
auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto mask = scores.compare(15.0, ThreadBinaryCompareOp::Gt);

auto out = scores.put(mask, pluss::null);
std::cout << out;
```

### Output

```text
0  10
1  NULL
2  NULL
ptype: Series
```

## Notes

- Non-mutating — `put()` returns a new `Thread` and leaves `*this` unchanged; reassign the
  result (`thread = thread.put(...)`) if you want the effect to stick.
- `put(mask, value)` is exactly `Thread::select({mask}, {value}, *this)` — see
  [`Thread::select`](select.md) for the general multi-condition form this specializes.
- `put(mask, pluss::null)` is the one overload that doesn't go through `select()` — it nulls
  the validity mask directly rather than writing a value.
- The `pluss::Series`-mask overloads exist purely for convenience; they're identical to
  wrapping the mask in `Thread(...)` first.

## See Also

- [`Thread::select`](select.md): the general numpy-`select`-style construct `put()`
  specializes — multiple ordered conditions/choices instead of one mask/value pair.
- [`Thread::fillnull`](fillnull.md): the inverse direction — replace `NULL` rows with a
  value, rather than nulling out selected rows.
- [`Thread::replace`](replace.md): replace by *value* match instead of by boolean mask.
- [`Thread::compare`](compare.md): the usual way to build the `bool_mask` argument.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ptype.html">
    <span class="label">Previous</span>
    <span class="title">ptype</span>
  </a>
  <a class="next-link" href="radd.html">
    <span class="label">Next</span>
    <span class="title">radd</span>
  </a>
</div>
