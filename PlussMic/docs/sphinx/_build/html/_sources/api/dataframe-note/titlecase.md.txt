<!-- pluss-hand-authored -->

# DataFrameNoteAccessor::titlecase

[Back to DataFrameNoteAccessor API Reference](../../dataframe_note_accessor_api_reference)

## Signature

```cpp
DataFrameNoteAccessor::titlecase() const -> DataFrame
```

## What It Does

Capitalizes the first letter of every "word" in every selected text
feature's cells and lowercases the rest — the classic display-formatting
transform for names, addresses, or titles typed in all-lowercase or
inconsistent casing.

```{warning} Word-boundary detection resets on *any* non-alphanumeric character
`titlecase()` decides where a new "word" starts the same way Python's
`str.title()` does: a word boundary is any position right after a
non-alphanumeric character (space, hyphen, apostrophe, digit-to-letter
transitions, punctuation — anything that isn't a letter or digit), not just
whitespace. This means an apostrophe inside a contraction resets the
boundary too, so `"don't"` titlecases to `"Don'T"`, not `"Don't"` — see
Example 2. This is a well-known quirk of this style of word-boundary
detection, not a bug specific to this implementation, but it's worth knowing
before running `titlecase()` over free-text data that contains contractions,
hyphenated words, or embedded digits.
```

Like every method on this accessor, `titlecase()` applies to **every feature
the accessor was built with** in one call — see [`strip()`](strip.md) for
the shared mechanic and how null cells pass through unaffected.

## Parameters

`None`

## Returns

`DataFrame`
    A new frame with every selected feature's text title-cased.

## Examples

Example 1. The common case: clean, space-separated words title-case exactly
as expected.

```cpp
using namespace pluss::table;

auto places = DataFrame({
    {"name", Thread(pluss::make_note(
        {"accra central market", "east legon", "north kaneshie"}))},
});
std::cout << places;

auto out = places.nt().titlecase();
std::cout << out;
```

### Output (before)

```text
===========================
     | name                
---------------------------
0    | accra central market
1    | east legon          
2    | north kaneshie      
---------------------------
[3 cs x 1 feat]
```

### Output (after)

```text
===========================
     | name                
---------------------------
0    | Accra Central Market
1    | East Legon          
2    | North Kaneshie      
---------------------------
[3 cs x 1 feat]
```

Every space-separated word gets its first letter capitalized and the rest
lowercased — exactly the display transform most people expect from
"titlecase."

Example 2. The word-boundary quirk in action: apostrophes and hyphens reset
the "start of a word" detector, capitalizing letters most people wouldn't
expect.

```cpp
auto phrases = DataFrame({
    {"phrase", Thread(pluss::make_note(
        {"don't stop", "rock 'n' roll", "co-op bank"}))},
});
std::cout << phrases;

auto out2 = phrases.nt().titlecase();
std::cout << out2;
```

### Output (before)

```text
====================
     | phrase       
--------------------
0    | don't stop   
1    | rock 'n' roll
2    | co-op bank   
--------------------
[3 cs x 1 feat]
```

### Output (after)

```text
====================
     | phrase       
--------------------
0    | Don'T Stop   
1    | Rock 'N' Roll
2    | Co-Op Bank   
--------------------
[3 cs x 1 feat]
```

`"don't"` becomes `"Don'T"`: the apostrophe is a non-alphanumeric character,
so the letter right after it (`t`) is treated as the start of a new "word"
and capitalized, exactly like `"stop"`'s leading `s`. The same thing happens
to the `n` between the two apostrophes in `"rock 'n' roll"`, and to the `o`
right after the hyphen in `"co-op"`. There is no built-in way to make
`titlecase()` treat apostrophes or hyphens as internal-to-a-word; if that
matters for a given column, post-process the result with
[`replace()`](replace.md) for the specific patterns you need to fix, or avoid
`titlecase()` on columns known to contain contractions.

## Notes

- Applies to every feature the accessor was built with, in one call — see
  [`strip()`](strip.md)'s Notes for the mechanics shared by every method on
  this accessor.
- A null cell on input stays null on output.
- Word-boundary detection resets on *any* non-alphanumeric character, not
  just whitespace — see the warning above and Example 2.

## See Also

- [`DataFrameNoteAccessor::lower`](lower.md) / [`DataFrameNoteAccessor::upper`](upper.md):
  whole-cell case transforms, unaffected by word boundaries.
- [`DataFrameNoteAccessor::replace`](replace.md): a way to patch up specific
  known quirks in a `titlecase()` result.

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
