<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 399b703f25a0 -->

# Thread::replace

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
// Text substring/regex replace (Note payload only)
Thread::replace(std::string_view from, std::string_view to) const -> Thread
Thread::replace(std::string_view from,
                std::string_view to,
                const TextOptions& options) const -> Thread

// Whole-value replace, by payload kind
Thread::replace(double from, double to) const -> Thread
Thread::replace(double from, pluss::null_t to) const -> Thread
Thread::replace(pluss::null_t from, double to) const -> Thread
Thread::replace(const pluss::Series& from, const pluss::Series& to) const -> Thread
Thread::replace(const pluss::Series& from, pluss::null_t to) const -> Thread
Thread::replace(pluss::null_t from, const pluss::Series& to) const -> Thread
Thread::replace(const pluss::Note& from, const pluss::Note& to) const -> Thread
Thread::replace(const pluss::Note& from, pluss::null_t to) const -> Thread
Thread::replace(pluss::null_t from, const pluss::Note& to) const -> Thread
Thread::replace(const pluss::Timeline& from, const pluss::Timeline& to) const -> Thread
Thread::replace(const pluss::Timeline& from, pluss::null_t to) const -> Thread
Thread::replace(pluss::null_t from, const pluss::Timeline& to) const -> Thread
Thread::replace(const pluss::TimeIndex& from, const pluss::TimeIndex& to) const -> Thread
Thread::replace(const pluss::TimeIndex& from, pluss::null_t to) const -> Thread
Thread::replace(pluss::null_t from, const pluss::TimeIndex& to) const -> Thread
```

## What It Does

`replace()` covers two conceptually different operations that happen to share a name:

- **Text substring/regex replace** — `replace(from, to)` and
  `replace(from, to, TextOptions)` — replaces occurrences of a substring or regex
  pattern *within* each cell's text. Requires a `Note`-kind thread. This is the same
  operation family as [`split()`](split.md) and the rest of the string/text methods.
- **Whole-value replace** — every other overload — replaces cells whose **entire
  value** exactly equals `from` with `to`, the way a lookup-table substitution works.
  The C++ type of `from`/`to` determines which payload kind participates: `double`
  pairs with a `Series`-kind thread, a scalar `pluss::Note` pairs with a `Note`-kind
  thread (matching the **whole cell's text**, not a substring), and scalar
  `pluss::Timeline`/`pluss::TimeIndex` pair with temporal threads. The `pluss::Series`
  scalar-wrapper overloads are typed convenience forms that unwrap to the `double`
  overload; passing `pluss::null_t` for `from` fills nulls (equivalent to
  [`fillnull()`](fillnull.md)), and passing it for `to` nullifies matching cells.

```{danger} replace(Note, Note) is whole-cell exact match, not substring replace
It is easy to reach for `replace()` on a `Note` thread and land on the wrong overload.
`replace("Tema", "Tamale")` (two `std::string_view` arguments) replaces the
**substring** `"Tema"` wherever it occurs inside each cell — a cell like
`"Tema West"` becomes `"Tamale West"`. `replace(pluss::make_note({"Tema"}),
pluss::make_note({"Tamale"}))` (two scalar `pluss::Note` arguments) instead requires
the **entire cell** to equal `"Tema"` exactly — `"Tema West"` is left completely
unchanged, since it isn't *equal to* `"Tema"`, only *contains* it. See Example 6 for
both side by side on the same data.
```

```{note} replace(from, to) and replace(from, to, {}) are byte-identical
When `options` is left at its default (`regex = false`, `case_sensitive = true`,
`limit = -1`), the three-argument text overload takes a fast path straight to the
same underlying primitive the two-argument overload uses. There is no behavioral
difference between the two spellings.
```

## Parameters

**from** / **to**
    The value(s) participating in the replacement. Their C++ type selects which
    overload (and which payload kind) applies — see What It Does above.

**options** (`TextOptions`, text overload only)
    `options.regex` (default `false`): treat `from` as a regular expression instead
    of literal text; supports `$1`, `$2`, ... backreferences in `to`.
    `options.case_sensitive` (default `true`): when `false`, matching ignores case
    (ASCII fold), but `to` is inserted exactly as given. `options.limit` (default
    `-1`): caps the number of replacements per cell; matches never overlap.
    `options.prefix` is not used by `replace()`.

## Returns

`Thread`
    A new thread, same length and kind as `*this` (except a `to = pluss::null_t`
    result, which nullifies matching cells rather than changing their kind).

## Examples

Example 1. The default text overload: literal, case-sensitive, unlimited
substring replacement.

```cpp
using namespace pluss::table;

auto cities = Thread(pluss::make_note({"cat and cat", "Cat there", "no match", "x"}),
    pluss::make_series(std::vector<bool>{true, true, true, false}));
std::cout << cities;

auto out1 = cities.replace("cat", "dog");
std::cout << out1;
```

### Output (before)

```text
0  cat and cat
1  Cat there
2  no match
3  NULL

ptype: Note
```

### Output

```text
0  dog and dog
1  Cat there
2  no match
3  NULL

ptype: Note
```

Both occurrences of `"cat"` in row 0 are replaced. Row 1's `"Cat"` (capital C)
doesn't match the lowercase, case-sensitive `"cat"` needle, so it's left untouched.
Row 3 is invalid and stays `NULL`.

Example 2. `case_sensitive = false`: matching ignores case, but the replacement text
is still inserted literally as given.

```cpp
auto out2 = cities.replace("cat", "dog", {.case_sensitive = false});
std::cout << out2;
```

### Output

```text
0  dog and dog
1  dog there
2  no match
3  NULL

ptype: Note
```

Row 1's `"Cat"` now matches too (case-insensitively) and is replaced with the literal
lowercase `"dog"` — the original capitalization of the matched text is discarded, not
preserved or transferred to the replacement.

Example 3. `limit` caps replacements per cell, and matches never overlap.

```cpp
auto words = Thread(pluss::make_note({"banana", "aardvark", "xyz"}));
auto out3 = words.replace("a", "X", {.limit = 2});
std::cout << out3;
```

### Output

```text
0  bXnXna
1  XXrdvark
2  xyz

ptype: Note
```

`"banana"` has three `a`s, but only the first two are replaced (`limit = 2`), leaving
the trailing `"na"` untouched. `"aardvark"` has two `a`s *adjacent* to each other at
the start — both get replaced, reaching the limit before the third `a` (inside
`"vark"`) is ever considered. `"xyz"` has no `a` at all, so it's unchanged.

Example 4. Regex mode with backreferences: swap the two sides of an `@` using capture
groups.

```cpp
auto emails = Thread(pluss::make_note({"alice@example", "bob@test", "invalid"}));
auto out4 = emails.replace(R"((\w+)@(\w+))", "$2#$1", {.regex = true});
std::cout << out4;
```

### Output

```text
0  example#alice
1  test#bob
2  invalid

ptype: Note
```

`(\w+)@(\w+)` captures the text before and after `@`; `"$2#$1"` swaps them around a
`#`. Row 2 (`"invalid"`) has no `@` at all, so the pattern doesn't match and the text
is returned unchanged.

Example 5. Whole-value numeric replace: cells that exactly equal `from` become `to`;
everything else is untouched. Requires a `Series`-kind thread.

```cpp
auto codes = Thread(pluss::make_series({404.0, 200.0, 404.0, 500.0}));
auto out5 = codes.replace(404.0, 0.0);
std::cout << out5;
```

### Output

```text
0  0
1  200
2  0
3  500

ptype: Series
```

Both cells equal to `404.0` become `0.0`; `200.0` and `500.0` are left as they are —
this is exact numeric equality, not a range or threshold.

Example 6. The critical distinction from the danger box above: substring replace vs.
whole-cell exact-match replace, on the same data.

```cpp
auto towns = Thread(pluss::make_note({"Tema", "Tema West", "Kumasi"}));

auto substring = towns.replace("Tema", "Tamale");
auto exact = towns.replace(pluss::make_note({"Tema"}), pluss::make_note({"Tamale"}));
std::cout << substring;
std::cout << exact;
```

### Output (`replace("Tema", "Tamale")`, substring overload)

```text
0  Tamale
1  Tamale West
2  Kumasi

ptype: Note
```

### Output (`replace(Note{"Tema"}, Note{"Tamale"})`, whole-cell overload)

```text
0  Tamale
1  Tema West
2  Kumasi

ptype: Note
```

Row 0 (`"Tema"` exactly) changes under both overloads. Row 1 (`"Tema West"`) changes
under the substring overload — the substring `"Tema"` is found and replaced inside
it — but stays completely untouched under the whole-cell overload, since
`"Tema West"` as a whole doesn't equal `"Tema"`.

Example 7. Whole-value replace can also target-and-nullify (`to = null`) or
fill-nulls-only (`from = null`, equivalent to [`fillnull()`](fillnull.md)).

```cpp
auto readings = Thread(pluss::make_series({-1.0, 12.5, -1.0, 30.0}));

auto nullified = readings.replace(-1.0, pluss::series::null);
auto filled = nullified.replace(pluss::series::null, 0.0);
std::cout << nullified;
std::cout << filled;
```

### Output (`replace(-1.0, null)`)

```text
0  NULL
1  12.5
2  NULL
3  30

ptype: Series
```

### Output (`replace(null, 0.0)`)

```text
0  0
1  12.5
2  0
3  30

ptype: Series
```

The sentinel value `-1.0` (a common "no reading" placeholder) is converted to a
genuine `NULL` first; a second call then fills every `NULL` with `0.0` — the same
result [`fillnull(0.0)`](fillnull.md) would give directly.

Example 8. A `pluss::Series` scalar wrapper is a typed convenience form — it unwraps
to the same `double` overload as Example 5.

```cpp
auto via_series = codes.replace(pluss::make_series({404.0}), pluss::make_series({0.0}));
// via_series is identical to out5 from Example 5.
```

Example 9. Whole-value replace on a `Timeline`-kind thread: cells equal to a given
timestamp are swapped for another.

```cpp
auto events = Thread(pluss::make_timeline(
    {"2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-01T00:00:00Z"}));
auto out9 = events.replace(
    pluss::make_timeline({"2024-01-01T00:00:00Z"}),
    pluss::make_timeline({"2024-06-15T00:00:00Z"}));
std::cout << out9;
```

### Output

```text
0  2024-06-15
1  2024-01-02
2  2024-06-15

ptype: Timeline
```

Both cells matching `2024-01-01T00:00:00Z` are replaced; `2024-01-02T00:00:00Z` is
untouched. `replace(Timeline, Timeline)` and `replace(TimeIndex, TimeIndex)` both
accept either a `Timeline`- or `TimeIndex`-kind calling thread — see Example 9b.

Example 9b. The same replacement, called through the `TimeIndex` overload instead —
on a plain `Timeline`-kind thread. Both temporal overloads accept both temporal
kinds symmetrically.

```cpp
auto out9b = events.replace(
    pluss::make_timeindex({"2024-01-01T00:00:00Z"}),
    pluss::make_timeindex({"2024-06-15T00:00:00Z"}));
std::cout << out9b;
```

### Output

```text
0  2024-06-15
1  2024-01-02
2  2024-06-15

ptype: Timeline
```

Identical result to Example 9, just reached through the `TimeIndex`-argument
overload instead of the `Timeline`-argument one — `events` itself is `Timeline`-kind
in both cases.

Example 10. Payload-kind mismatches throw.

```cpp
auto numbers = Thread(pluss::make_series({1.0, 2.0, 3.0}));
try {
  auto bad = numbers.replace("1", "one");
} catch (const std::exception& e) {
  std::cout << e.what() << "\n";
}
try {
  auto bad2 = numbers.replace("1", "one", TextOptions{});
} catch (const std::exception& e) {
  std::cout << e.what() << "\n";
}

auto notes = Thread(pluss::make_note({"a", "b"}));
try {
  auto bad3 = notes.replace(1.0, 2.0);
} catch (const std::exception& e) {
  std::cout << e.what() << "\n";
}
```

### Output

```text
Thread::replace: expected Note payload
Thread::replace: expected Note payload
Thread::replace(double, double) is only valid for Series payload
```

Both text overloads now share the exact same message — an earlier build had the
two-argument overload wired to a different string (`"...is only valid for Note
payload"`, no colon) than the three-argument overload; that inconsistency has been
fixed so both spellings fail identically.

## Notes

- Two unrelated operations share the `replace()` name — substring/regex replace on
  `Note` text, and whole-value exact-match replace across any payload kind. See the
  danger box above.
- `replace(from, to)` and `replace(from, to, {})` (the text overloads) are the exact
  same operation.
- Whole-value replace requires `from`/`to` scalar wrapper objects (`numel() == 1` /
  `size() == 1`) — a multi-element `pluss::Series`/`pluss::Note`/etc. throws.
- `replace(Timeline, Timeline)` and `replace(TimeIndex, TimeIndex)` both accept
  either a `Timeline`- or `TimeIndex`-kind calling thread — symmetric on both sides.
- `replace(null_t, to)` is equivalent to [`fillnull(to)`](fillnull.md).
- `replace(from, null_t)` nullifies cells matching `from` rather than changing the
  thread's kind.

## See Also

- [`Thread::fillnull`](fillnull.md): fill nulls with a value directly — what
  `replace(null_t, to)` delegates to.
- [`Thread::split`](split.md) / [`Thread::partition`](partition.md): other members
  of the substring-oriented text family, sharing `TextOptions`.
- [`Thread::eq`](eq.md): produce the boolean mask `replace()`'s whole-value overloads
  compute internally, if you need the mask itself rather than the replaced values.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="repeated.html">
    <span class="label">Previous</span>
    <span class="title">repeated</span>
  </a>
  <a class="next-link" href="rfloordiv.html">
    <span class="label">Next</span>
    <span class="title">rfloordiv</span>
  </a>
</div>
