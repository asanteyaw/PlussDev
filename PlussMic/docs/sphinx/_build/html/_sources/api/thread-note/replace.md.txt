<!-- pluss-hand-authored -->

# ThreadNoteAccessor::replace

[Back to ThreadNoteAccessor API Reference](../../thread_note_accessor_api_reference)

## Signatures

```cpp
ThreadNoteAccessor::replace(std::string_view from, std::string_view to) const -> Thread
ThreadNoteAccessor::replace(std::string_view from,
                            std::string_view to,
                            const TextOptions& options) const -> Thread
```

## What It Does

Replaces occurrences of a substring or regex pattern *within* each cell's text —
direct forwards to [`Thread::replace(from, to[, options])`](../thread/replace.md).
Requires the bound thread to be `Note`-kind.

```{danger} Only the text-substring overloads are exposed here
`Thread::replace()` itself has 14 additional "whole-value" overloads (numeric,
whole-`Note`, `Timeline`, `TimeIndex` scalar replacement — an exact-cell-match
lookup-table substitution, not a substring search). `ThreadNoteAccessor` exposes
**only** the two text-substring overloads shown above; the whole-value forms are
reachable solely through `Thread::replace()` directly (they don't fit the
"already-known-to-be-Note-kind" framing this accessor exists for). Don't expect
`.nt().replace(pluss::make_note({"Tema"}), pluss::make_note({"Tamale"}))` to
compile — call `Thread::replace()` directly for that operation.
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

`Thread`
    A new `Note`-kind thread, same length as the bound thread, with matching
    text replaced.

## Examples

Example 1. Default literal, case-sensitive, unlimited substring replacement.

```cpp
using namespace pluss::table;

auto cities = Thread(pluss::make_note({"cat and cat", "Cat there", "no match"}));
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
auto words = Thread(pluss::make_note({"banana", "aardvark", "xyz"}));
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
auto emails = Thread(pluss::make_note({"alice@example", "bob@test", "invalid"}));
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

Example 4. A non-`Note` thread throws.

```cpp
auto numbers = Thread(pluss::make_series({1.0, 2.0, 3.0}));
try {
  auto bad = numbers.nt().replace("1", "one");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Thread::replace: expected Note payload
```

## Notes

- `ThreadNoteAccessor::replace()` forwards straight to
  [`Thread::replace()`](../thread/replace.md)'s text-substring overloads —
  identical behavior and throws.
- Only the two text-substring overloads are exposed here; see the danger box
  above for the 14 whole-value overloads only reachable via `Thread::replace()`
  directly.
- `replace(from, to)` and `replace(from, to, {})` are byte-identical.
- A null cell on input stays null on output.

## See Also

- [`Thread::replace`](../thread/replace.md): the direct, non-accessor form —
  also exposes the whole-value replacement overloads this accessor omits.
- [`ThreadNoteAccessor::has`](has.md): test for a substring/pattern without
  changing the text.
- [`ThreadNoteAccessor::split`](split.md) /
  [`ThreadNoteAccessor::partition`](partition.md): break text apart instead of
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
