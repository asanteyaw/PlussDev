<!-- pluss-hand-authored -->

# Thread::titlecase

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::titlecase() const -> Thread
```

## What It Does

Capitalizes the first letter of every "word" in a `Note` (string) thread's cells and
lowercases the rest — the classic display-formatting transform for names,
addresses, or titles typed in all-lowercase or inconsistent casing. Requires the
thread to be `Note`-kind.

```{warning} Word-boundary detection resets on *any* non-alphanumeric character
`titlecase()` decides where a new "word" starts the same way Python's `str.title()`
does: a word boundary is any position right after a non-alphanumeric character
(space, hyphen, apostrophe, digit-to-letter transitions — anything that isn't a
letter or digit), not just whitespace. This means an apostrophe inside a contraction
resets the boundary too, so `"don't"` titlecases to `"Don'T"`, not `"Don't"` — see
Example 2. This is a well-known quirk of this style of word-boundary detection, not
a bug specific to this implementation.
```

## Parameters

`None`

## Returns

`Thread`
    A new `Note`-kind thread, same length as `*this`, with every cell's text
    title-cased.

## Examples

Example 1. The common case: clean, space-separated words title-case exactly as
expected.

```cpp
using namespace pluss::table;

auto places = Thread(pluss::make_note(
    {"accra central market", "east legon", "north kaneshie"}));
auto out = places.titlecase();
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
auto phrases = Thread(pluss::make_note(
    {"don't stop", "rock 'n' roll", "co-op bank"}));
auto out2 = phrases.titlecase();
std::cout << out2;
```

### Output

```text
0  Don'T Stop
1  Rock 'N' Roll
2  Co-Op Bank

ptype: Note
```

`"don't"` becomes `"Don'T"`: the apostrophe is a non-alphanumeric character, so the
letter right after it (`t`) is treated as the start of a new "word" and capitalized.
The same thing happens to the `n` between the two apostrophes in `"rock 'n' roll"`,
and to the `o` right after the hyphen in `"co-op"`. There is no built-in way to make
`titlecase()` treat apostrophes or hyphens as internal-to-a-word — post-process the
result with [`replace()`](replace.md) for known quirks that matter for a given
column.

## Notes

- A null cell on input stays null on output.
- Word-boundary detection resets on *any* non-alphanumeric character, not just
  whitespace — see the warning above.

## See Also

- [`Thread::lower`](lower.md) / [`Thread::upper`](upper.md): whole-cell case
  transforms, unaffected by word boundaries.
- [`Thread::replace`](replace.md): a way to patch up specific known quirks in a
  `titlecase()` result.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tally.html">
    <span class="label">Previous</span>
    <span class="title">tally</span>
  </a>
  <a class="next-link" href="tl.html">
    <span class="label">Next</span>
    <span class="title">tl</span>
  </a>
</div>
