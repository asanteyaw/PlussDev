<!-- pluss-hand-authored -->
# pluss::note::select

[Back to Note Factory API Reference](../../note_factory_api_reference)

## Signatures

```cpp
Note pluss::note::select(series_range conditions, note_range choices,
                         const Note& default_value)
Note pluss::note::select(std::initializer_list<Series> conditions,
                         std::initializer_list<Note> choices,
                         const Note& default_value)
Note pluss::note::select(const Series& condition, const Note& choice,
                         const Note& default_value)
Note pluss::note::select(series_range conditions, note_range choices,
                         string_view default_value)
Note pluss::note::select(std::initializer_list<Series> conditions,
                         std::initializer_list<Note> choices,
                         string_view default_value)
Note pluss::note::select(const Series& condition, string_view choice,
                         string_view default_value)
```

The single-`condition` forms are one-branch shorthands. Any `string_view`
choice or default is a scalar text cell — sugar for
`pluss::note::make(text)`.

## Build A `Note` By Ordered Boolean Conditions

`select()` assembles one `Note` from a list of boolean `Series` conditions
and a matching list of text choices. At every output cell it walks the
conditions **first to last** and takes the choice paired with the first one
that is true there; if none is true, `default_value` supplies the cell.

This is the text counterpart of `pluss::series::select` — an *n*-way,
priority-ordered generalisation of "where". It is the
tool for turning several overlapping masks into a single labelled column:
bucket names from numeric thresholds, status strings from a set of flags, a
category per row from a cascade of predicates.

### Ordering matters

Conditions are a priority list, not a set. Where two conditions are true at
the same cell, the **earlier** one wins — later branches are fallbacks, not
overrides. Put the most specific or highest-priority condition first.

### Shapes and scalar broadcasting

An operand (condition, choice, or default) is *scalar-like* when it holds
exactly one element; such an operand applies to every output cell. Every
operand that is **not** scalar-like must have the same shape, and that shape
is the output shape. So you can:

- pair full-shaped conditions with full-shaped choices, or
- pair full-shaped conditions with a single scalar choice that fills wherever
  that condition wins, or
- give a scalar-like condition (a one-element `true`/`false`) that turns its
  whole branch on or off.

### Requirements

- At least one condition, and `conditions.size() == choices.size()`.
- Every condition must be a **defined `Bool`-dtype `Series`**. Numeric
  "truthiness" is not accepted — convert with a comparison first.
- Every choice and the default must be defined.
- The result is **UTF-8 mode**. All branches are read and shape-checked
  before assembly, so a malformed branch is an error even where its
  condition is false everywhere.

## Example 1: Single Condition, Text Shorthand

```cpp
const auto flags = pluss::make_series({true, false, true});
std::cout << pluss::note::print(pluss::note::select(flags, "YES", "no")) << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["YES", "no", "YES"]
```

The scalar choice `"YES"` and default `"no"` broadcast across the
condition's three cells.

## Example 2: Single Condition With Explicit `Note` Operands

```cpp
const auto flags = pluss::make_series({true, false, true});
const pluss::Note n = pluss::note::select(
    flags, pluss::note::make("HIT"), pluss::note::make("miss"));
std::cout << n << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["HIT", "miss", "HIT"]
```

Identical behavior to the shorthand — use this form when the choice is
already a `Note` (for instance a full-shaped column).

## Example 3: Two Ordered Conditions, First True Wins

```cpp
const auto early = pluss::make_series({true, true, false, false});
const auto late  = pluss::make_series({false, true, true, false});

const pluss::Note n = pluss::note::select(
    {early, late},
    {pluss::note::make("early"), pluss::note::make("late")},
    "none");
pluss::note::print(std::cout, n) << "\n";
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["early", "early", "late", "none"]
```

At cell 1 both conditions are true and the earlier branch supplies
`"early"`. Cell 3 satisfies neither, so it takes the default.

## Example 4: A Scalar Choice Broadcast Under A 2-D Condition

```cpp
const auto mask = pluss::make_series({{true, false}, {false, true}});
std::cout << pluss::note::print(
    pluss::note::select(mask, pluss::note::make("X"), pluss::note::make("."))) << "\n";
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["X", "."], 
  [".", "X"]
]
```

The `2×2` condition sets the output shape; the scalar choice fills wherever
the mask is true and the scalar default fills the rest.

## Example 5: A Non-Boolean Condition Is Rejected

```cpp
try {
  pluss::note::select(pluss::make_series({1, 0, 1}), "y", "n");
} catch (const pluss::Error& e) {
  const std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
pluss::note::select: conditions must use Bool dtype
```

Pass a real boolean mask — `series > 0`, `series == value`, and so on.

## Example 6: Condition And Choice Counts Must Match

```cpp
const auto a = pluss::make_series({true});
try {
  pluss::note::select({a, a}, {pluss::note::make("x")}, "z");
} catch (const pluss::Error& e) {
  const std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
pluss::note::select: condition/choice count mismatch
```

## See Also

- [`pluss::note::make`](make.md): build the choice and default columns.
- [`pluss::note::full`](full.md) / [`pluss::note::rep`](rep.md): quick
  same-shaped choice or default columns.
- [`pluss::note::format`](format.md): assemble a single string rather than
  choosing among prepared ones.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rep.html">
    <span class="label">Previous</span>
    <span class="title">rep</span>
  </a>
  <a class="next-link" href="unpack.html">
    <span class="label">Next</span>
    <span class="title">unpack</span>
  </a>
</div>
