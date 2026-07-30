<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 289054f2dfca -->

# Thread::indicate

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::indicate(std::string_view delimiter = "|",
                 std::string prefix = "") const -> DataFrame
```

## What It Does

Splits every cell of a `Note` (string) thread on `delimiter`, collects every distinct
non-empty token seen anywhere in the thread, and produces one `Bool` column per
distinct token ("level") — the classic one-hot / multi-label indicator ("dummy
variable") encoding, for a text thread that holds delimiter-separated tags,
categories, or labels rather than a single value per cell. Requires the thread to be
`Note`-kind. Column `<prefix><level>` is `true` for a row if `level` is one of that
row's tokens, `false` otherwise.

Levels are collected into a `std::set<std::string>`, so the output columns always
appear in **sorted (alphabetical) order** — not first-seen order, and not any order
tied to the input data's row sequence.

```{danger} indicate() never attaches a validity mask — invalid rows are just all-false, not NULL
Unlike every other method in this family, `indicate()`'s output columns carry **no
validity mask at all**. A row that was invalid on input doesn't show `NULL` in the
indicator columns — it shows `false` in every one of them, exactly the same as a row
that's genuinely valid but happens to match none of the levels. There is no way to
tell "this row is invalid" apart from "this row matched nothing" by looking at
`indicate()`'s output alone. See Example 1.
```

```{warning} An invalid row's hidden text can still create a phantom output column
The set of levels is collected from **every row's underlying text**, including rows
marked invalid — the delimiter split that builds the level set runs over the whole
thread before any validity check happens. An invalid cell whose hidden underlying
text contains a token no valid row ever uses still produces its own indicator column,
one that's `false` for every row in the entire result (since no valid row can ever
match it, and the invalid row itself is skipped when indicators are actually
computed). See Example 2 — this is the same category of surprise documented on
[`split()`](split.md)'s single-delimiter overload.
```

## Parameters

**delimiter** (`std::string_view`, default `"|"`)
    The literal text to split each cell on before collecting tokens. Must not be
    empty — `indicate("")` throws `"Thread::indicate: delimiter cannot be empty"`.

**prefix** (`std::string`, default `""`)
    Text prepended to each level to form its column name — with the default empty
    prefix, a column is named exactly after the level text itself (e.g. a `"red"`
    token produces a column literally named `red`).

## Returns

`DataFrame`
    A new frame with one `Bool` feature per distinct non-empty token seen in the
    thread, sorted alphabetically by token text, named `<prefix><level>`.

## Examples

Example 1. The default `"|"` delimiter, and the all-false-for-invalid-rows behavior
from the danger box above.

```cpp
using namespace pluss::table;

auto tagged = Thread(pluss::make_note({"red|blue", "blue|green", "red", "x"}),
    pluss::make_series(std::vector<bool>{true, true, true, false}));
std::cout << tagged;

auto out1 = tagged.indicate();
std::cout << out1;
```

### Output (before)

```text
0  red|blue
1  blue|green
2  red
3  NULL

ptype: Note
```

### Output

```text
==========================
     | blue   green  red  
--------------------------
0    | true   false  true 
1    | true   true   false
2    | false  false  true 
3    | false  false  false
--------------------------
[4 cs x 3 feat]
```

Three distinct tokens appear across the valid rows — `"blue"`, `"green"`, `"red"` —
in alphabetical column order (not the order they first appear in the data). Row 3 was
invalid on input; notice it shows `false` in every column, not `NULL` — indistinguishable,
from this table alone, from a valid row that simply matched none of the levels.

Example 2. A custom delimiter and prefix, and the phantom-column effect from the
warning above: row 3's hidden invalid text introduces a level no valid row ever uses.

```cpp
auto langs = Thread(pluss::make_note({"python, rust", "python, go", "just-one-word", "mars"}),
    pluss::make_series(std::vector<bool>{true, true, true, false}));
std::cout << langs;

auto out2 = langs.indicate(", ", "know_");
std::cout << out2;
```

### Output (before — row 3 is invalid, its underlying text is invisible here)

```text
0  python, rust
1  python, go
2  just-one-word
3  NULL

ptype: Note
```

### Output

```text
=====================================================================
     | know_go  know_just-one-word  know_mars  know_python  know_rust
---------------------------------------------------------------------
0    | false    false               false      true         true     
1    | true     false               false      true         false    
2    | false    true                false      false        false    
3    | false    false               false      false        false    
---------------------------------------------------------------------
[4 cs x 5 feat]
```

Row 3's hidden text is `"mars"` — a level none of the three valid rows ever mention.
It still produces a `know_mars` column, one that reads `false` for every single row
in the table: no valid row can match it (they never say `"mars"`), and row 3 itself is
skipped when indicators are computed (since it's invalid), so the column simply
exists as inert padding.

Example 3. An empty delimiter throws immediately.

```cpp
try {
  auto bad = tagged.indicate("");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread::indicate: delimiter cannot be empty
```

Example 4. A non-`Note` thread throws.

```cpp
auto numbers = Thread(pluss::make_series({1.0, 2.0, 3.0}));
try {
  auto bad = numbers.indicate();
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Thread::indicate: expected Note payload
```

## Notes

- Requires the calling thread to be `Note`-kind.
- Output columns are sorted alphabetically by token text, not by first-appearance
  order.
- No validity mask is attached to the result at all — invalid input rows show
  `false` everywhere, not `NULL`. See the danger box above.
- The level set is collected from every row's raw text regardless of validity, so an
  invalid row can introduce an all-false phantom column. See the warning above and
  Example 2.
- An empty `delimiter` throws `"Thread::indicate: delimiter cannot be empty"`.

## See Also

- [`Thread::split`](split.md): the same underlying delimiter-split mechanic,
  producing positional columns instead of one-hot level columns — and sharing the
  same invalid-row-inflates-output quirk on its single-delimiter overload.
- [`Thread::has`](has.md): test for a single specific substring instead of
  enumerating every distinct delimited token.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="head.html">
    <span class="label">Previous</span>
    <span class="title">head</span>
  </a>
  <a class="next-link" href="info.html">
    <span class="label">Next</span>
    <span class="title">info</span>
  </a>
</div>
