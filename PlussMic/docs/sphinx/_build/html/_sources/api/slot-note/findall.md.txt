<!-- pluss-hand-authored -->

# SlotNoteAccessor::findall

[Back to SlotNoteAccessor API Reference](../../slot_note_accessor_api_reference)

## Signatures

```cpp
pluss::table::DataFrame SlotNoteAccessor::findall(std::string_view pattern) const

pluss::table::DataFrame SlotNoteAccessor::findall(std::string_view pattern,
                                                  const pluss::table::TextOptions& options) const
```

## What It Does

Finds every match of `pattern` in each value of the bound slot and
expands the result into one output feature per match position — feature
`match_0` holds each value's first match, `match_1` its second, and so
on. A value with fewer matches than another has `NULL` in the features
beyond its own match count; the feature count equals however many
matches the *most-matching* value produced.

```{danger} The single-argument overload defaults to regex mode, same gotcha as count()
Exactly like [`count()`](count.md), the single-argument `findall(pattern)`
defaults internally to `{.regex = true}` — a literal `"."` is interpreted
as "match any character," not a literal period.
```

## Parameters

**pattern** (`std::string_view`)
    A regular expression pattern under the single-argument overload
    (always regex mode), or literal/regex text under the two-argument
    overload depending on `options.regex`.

**options** (`pluss::table::TextOptions`, two-argument overload only)
    `options.regex` (default `false`): treat `pattern` as regex.
    `options.case_sensitive` (default `true`): case-insensitive when
    `false`. `options.limit` (default `-1`): caps matches collected per
    value, and therefore an upper bound on feature count. `options.prefix`
    is **not** used — feature names are always `match_0`, `match_1`, ...

## Returns

`pluss::table::DataFrame`
    A new frame with `match_0`, `match_1`, ... features (as many as the
    value with the most matches needed).

## Examples

Example 1. Extract every run of digits, producing a ragged, `NULL`-padded
result.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("1 2 3 4"), pluss::make_note("10 20")});
auto out = slot.nt().findall(R"(\d+)", {.regex = true});
std::cout << out;
```

### Output

```text
=========================================
     | match_0  match_1  match_2  match_3
-----------------------------------------
0    | 1        2        3        4      
1    | 10       20       NULL     NULL   
-----------------------------------------
[2 cs x 4 feat]
```

The first value has four number matches, driving the output to four
features; the second (two matches) has `NULL` in `match_2`/`match_3`.

## Notes

- The single-argument overload defaults to `{.regex = true}` — see the
  danger callout above.
- Output features are always named `match_0`, `match_1`, ...
- Requires the slot to be `Note`-kind — a non-`Note` slot throws
  `"Slot::nt: slot kind must be Note"`.

## See Also

- [`SlotNoteAccessor::count`](count.md): just the match count, without
  the matched text.
- [`SlotNoteAccessor::split`](split.md): delimiter-based splitting,
  another feature-expanding method.
- [`Slot::nt`](../slot/nt.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ends.html">
    <span class="label">Previous</span>
    <span class="title">ends</span>
  </a>
  <a class="next-link" href="fullmatch.html">
    <span class="label">Next</span>
    <span class="title">fullmatch</span>
  </a>
</div>
