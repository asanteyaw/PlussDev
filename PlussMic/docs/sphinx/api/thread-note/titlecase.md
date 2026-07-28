<!-- pluss-hand-authored -->

# ThreadNoteAccessor::titlecase

[Back to ThreadNoteAccessor API Reference](../../thread_note_accessor_api_reference)

## Signature

```cpp
ThreadNoteAccessor::titlecase() const -> Thread
```

## What It Does

Capitalizes the first letter of every "word" in the bound thread's cells and
lowercases the rest — a direct forward to
[`Thread::titlecase()`](../thread/titlecase.md). Requires the bound thread to be
`Note`-kind. The classic display-formatting transform for names, addresses, or
titles typed in inconsistent casing.

```{warning} Word-boundary detection resets on *any* non-alphanumeric character
`titlecase()` decides where a new "word" starts the same way Python's
`str.title()` does: a word boundary is any position right after a
non-alphanumeric character (space, hyphen, apostrophe, digit-to-letter
transitions), not just whitespace. An apostrophe inside a contraction resets the
boundary too, so `"don't"` titlecases to `"Don'T"`, not `"Don't"` — see
Example 2.
```

## Parameters

`None`

## Returns

`Thread`
    A new `Note`-kind thread, same length as the bound thread, with every cell's
    text title-cased.

## Examples

Example 1. The common case: clean, space-separated words title-case exactly as
expected.

```cpp
using namespace pluss::table;

auto places = Thread(pluss::make_note(
    {"accra central market", "east legon", "north kaneshie"}));
auto out = places.nt().titlecase();
std::cout << out;
```

### Output

```text
0  Accra Central Market
1  East Legon
2  North Kaneshie

ptype: Note
```

Example 2. The word-boundary quirk in action: apostrophes and hyphens reset the
"start of a word" detector.

```cpp
auto phrases = Thread(pluss::make_note({"don't stop", "co-op bank"}));
auto out2 = phrases.nt().titlecase();
std::cout << out2;
```

### Output

```text
0  Don'T Stop
1  Co-Op Bank

ptype: Note
```

`"don't"` becomes `"Don'T"`: the apostrophe is non-alphanumeric, so the letter
right after it is treated as the start of a new word and capitalized. There is
no built-in way to make `titlecase()` treat apostrophes or hyphens as
internal-to-a-word — post-process the result with [`replace()`](replace.md) for
known quirks that matter for a given column.

Example 3. A non-`Note` thread throws — the same message
[`Thread::titlecase()`](../thread/titlecase.md) produces directly.

```cpp
auto numbers = Thread(pluss::make_series({1.0, 2.0, 3.0}));
try {
  auto bad = numbers.nt().titlecase();
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Thread::titlecase: expected Note payload
```

## Notes

- `ThreadNoteAccessor::titlecase()` is a one-line forward to
  [`Thread::titlecase()`](../thread/titlecase.md) — identical behavior and
  throws.
- A null cell on input stays null on output.
- Word-boundary detection resets on *any* non-alphanumeric character, not just
  whitespace — see the warning above.

## See Also

- [`Thread::titlecase`](../thread/titlecase.md): the direct, non-accessor form
  this method forwards to.
- [`ThreadNoteAccessor::lower`](lower.md) /
  [`ThreadNoteAccessor::upper`](upper.md): whole-cell case transforms,
  unaffected by word boundaries.
- [`ThreadNoteAccessor::replace`](replace.md): a way to patch up specific known
  quirks in a `titlecase()` result.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="strip.html">
    <span class="label">Previous</span>
    <span class="title">strip</span>
  </a>
  <a class="next-link" href="upper.html">
    <span class="label">Next</span>
    <span class="title">upper</span>
  </a>
</div>
