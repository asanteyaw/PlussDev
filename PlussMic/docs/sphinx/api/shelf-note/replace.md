<!-- pluss-hand-authored -->

# ShelfNoteAccessor::replace

[Back to ShelfNoteAccessor API Reference](../../shelf_note_accessor_api_reference)

## Signatures

```cpp
Shelf ShelfNoteAccessor::replace(std::string_view from, std::string_view to) const
Shelf ShelfNoteAccessor::replace(std::string_view from,
                                  std::string_view to,
                                  const TextOptions& options) const
```

## What It Does

Replaces occurrences of a substring or regex pattern *within* each cell's text —
direct forwards to [`Shelf::replace(from, to[, options])`](../shelf/replace.md).
Requires the bound shelf to be `Note`-kind.

```{danger} Only the text-substring overloads are exposed here
`Shelf::replace()` itself has 14 additional "whole-value" overloads (numeric,
whole-`Note`, `Timeline`, `TimeIndex` scalar replacement — an exact-cell-match
lookup-table substitution, not a substring search). `ShelfNoteAccessor` exposes
**only** the two text-substring overloads shown above; the whole-value forms are
reachable solely through `Shelf::replace()` directly (they don't fit the
"already-known-to-be-Note-kind" framing this accessor exists for). Don't expect
`.nt().replace(pluss::make_note({"Tema"}), pluss::make_note({"Tamale"}))` to
compile — call `Shelf::replace()` directly for that operation.
```

`replace(from, to)` and `replace(from, to, {})` (default-constructed `TextOptions`)
are byte-identical — the two-argument overload is simply a fast path to the same
underlying primitive.

## Parameters

**from** (`std::string_view`)

The literal text (or, with `options.regex = true`, the regular expression) to
search for.

**to** (`std::string_view`)

The replacement text. With `options.regex = true`, may contain `$1`, `$2`, ...
backreferences to capture groups in `from`.

**options** (`TextOptions`, three-argument overload only)

`options.regex` (default `false`): treat `from` as a regular expression instead
of literal text. `options.case_sensitive` (default `true`): when `false`,
matching ignores case, but `to` is still inserted exactly as given.
`options.limit` (default `-1`, unlimited): caps the number of replacements per
cell; matches never overlap.

## Returns

`Shelf`
    A new `Note`-kind shelf, same length as the bound shelf, with matching
    text replaced.

## Examples

Example 1. Default literal, case-sensitive, unlimited substring replacement.

```cpp
using namespace pluss::table;

auto cities = Shelf(pluss::make_note({"cat and cat", "Cat there", "no match"}));
auto out = cities.nt().replace("cat", "dog");
std::cout << out;
```

### Output

```text
0  dog and dog
1  Cat there
2  no match

ptype: Note
```

Both occurrences of `"cat"` in row 0 are replaced. Row 1's capital `"Cat"`
doesn't match the case-sensitive, lowercase needle.

Example 2. `limit` caps replacements per cell; matches never overlap.

```cpp
auto words = Shelf(pluss::make_note({"banana", "aardvark", "xyz"}));
auto out2 = words.nt().replace("a", "X", {.limit = 2});
std::cout << out2;
```

### Output

```text
0  bXnXna
1  XXrdvark
2  xyz

ptype: Note
```

Example 3. Regex mode with backreferences, swapping the two sides of an `@`.

```cpp
auto emails = Shelf(pluss::make_note({"alice@example", "bob@test", "invalid"}));
auto out3 = emails.nt().replace(R"((\w+)@(\w+))", "$2#$1", {.regex = true});
std::cout << out3;
```

### Output

```text
0  example#alice
1  test#bob
2  invalid

ptype: Note
```

Row 2 has no `@` at all, so the pattern doesn't match and the text passes through
unchanged.

Example 4. A non-`Note` shelf throws.

```cpp
auto numbers = Shelf(pluss::make_series({1.0, 2.0, 3.0}));
try {
  auto bad = numbers.nt().replace("1", "one");
} catch (const pluss::Error& e) {
  std::string msg(e.message());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Shelf::replace: expected Note payload
```

## Notes

- `ShelfNoteAccessor::replace()` forwards straight to
  [`Shelf::replace()`](../shelf/replace.md)'s text-substring overloads —
  identical behavior and throws.
- Only the two text-substring overloads are exposed here; see the danger box
  above for the 14 whole-value overloads only reachable via `Shelf::replace()`
  directly.
- `replace(from, to)` and `replace(from, to, {})` are byte-identical.
- A null cell on input stays null on output.

## See Also

- [`Shelf::replace`](../shelf/replace.md): the direct, non-accessor form —
  also exposes the whole-value replacement overloads this accessor omits.
- [`ShelfNoteAccessor::has`](has.md): test for a substring/pattern without
  changing the text.
- [`ShelfNoteAccessor::split`](split.md) /
  [`ShelfNoteAccessor::partition`](partition.md): break text apart instead of
  substituting within it.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="partition.html">
    <span class="label">Previous</span>
    <span class="title">partition</span>
  </a>
  <a class="next-link" href="rstrip.html">
    <span class="label">Next</span>
    <span class="title">rstrip</span>
  </a>
</div>
