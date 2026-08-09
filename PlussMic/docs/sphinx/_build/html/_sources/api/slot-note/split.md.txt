<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 270ec9410ddf -->

# SlotNoteAccessor::split

[Back to SlotNoteAccessor API Reference](../../slot_note_accessor_api_reference)

## Signatures

```cpp
pluss::table::DataFrame SlotNoteAccessor::split(std::string_view delimiter,
                                                std::int64_t limit = -1,
                                                std::string prefix = "part") const

pluss::table::DataFrame SlotNoteAccessor::split(std::string_view pattern,
                                                const pluss::table::TextOptions& options) const
```

## What It Does

Splits every value in the bound slot on a delimiter, expanding the
result into one output feature per split position. Two overloads with
different delimiter semantics:

- **`split(delimiter, limit, prefix)`**: always a literal substring
  split (never regex).
- **`split(pattern, options)`**: `pattern` is literal or regex depending
  on `options.regex`; `options.limit` caps the split count,
  `options.prefix` names the features.

Both overloads produce ragged results — a value with fewer pieces than
the widest one is `NULL` in the extra features.

## Parameters

**delimiter** / **pattern** (`std::string_view`)
    The literal delimiter (first overload) or literal/regex pattern
    (second overload). Must not be empty.

**limit** (`std::int64_t`, first overload, default `-1`)
    Maximum number of splits to perform; `-1` means unlimited.

**prefix** (`std::string`, first overload, default `"part"`)
    Base name for the output features: `<prefix>_0`, `<prefix>_1`, ...

**options** (`pluss::table::TextOptions`, second overload)
    `options.regex` (default `false`), `options.limit` (default `-1`),
    `options.prefix` (default `"part"`).

## Returns

`pluss::table::DataFrame`
    A new frame with one `<prefix>_0`, `<prefix>_1`, ... feature per
    split position.

## Examples

Example 1. A plain literal split, no limit.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("a/b/c"), pluss::make_note("x/y"), pluss::make_note("solo")});
auto out = slot.nt().split("/");
std::cout << out;
```

### Output

```text
=============================
     | part_0  part_1  part_2
-----------------------------
0    | a       b       c     
1    | x       y       NULL  
2    | solo    NULL    NULL  
-----------------------------
[3 cs x 3 feat]
```

Example 2. `limit` caps the number of splits — the remainder stays
joined in the last feature.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("a/b/c"), pluss::make_note("x/y"), pluss::make_note("solo")});
auto out = slot.nt().split("/", 1);
std::cout << out;
```

### Output

```text
=====================
     | part_0  part_1
---------------------
0    | a       b/c   
1    | x       y     
2    | solo    NULL  
---------------------
[3 cs x 2 feat]
```

Example 3. The second overload: a regex pattern with a custom feature
prefix.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("a, b,c"), pluss::make_note("x , y"), pluss::make_note("single")});
auto out = slot.nt().split(R"(\s*,\s*)", {.regex = true, .prefix = "field"});
std::cout << out;
```

### Output

```text
================================
     | field_0  field_1  field_2
--------------------------------
0    | a        b        c      
1    | x        y        NULL   
2    | single   NULL     NULL   
--------------------------------
[3 cs x 3 feat]
```

## Notes

- An empty delimiter/pattern throws.
- `limit` caps the *number of splits*, not the *number of output
  features* directly.
- `options.case_sensitive` has no effect on `split()`.
- Requires the slot to be `Note`-kind — a non-`Note` slot throws.

## See Also

- [`SlotNoteAccessor::partition`](partition.md): a related but simpler
  split — always exactly 3 features, first-occurrence-only.
- [`Slot::nt`](../slot/nt.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rstrip.html">
    <span class="label">Previous</span>
    <span class="title">rstrip</span>
  </a>
  <a class="next-link" href="starts.html">
    <span class="label">Next</span>
    <span class="title">starts</span>
  </a>
</div>
