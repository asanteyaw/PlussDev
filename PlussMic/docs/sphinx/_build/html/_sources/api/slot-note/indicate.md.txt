<!-- pluss-hand-authored -->

# SlotNoteAccessor::indicate

[Back to SlotNoteAccessor API Reference](../../slot_note_accessor_api_reference)

## Signature

```cpp
pluss::table::DataFrame SlotNoteAccessor::indicate(std::string_view delimiter = "|",
                                                   std::string prefix = "") const
```

## What It Does

Splits each value in the bound slot on `delimiter`, collects every
distinct non-empty token seen anywhere in the slot, and produces one
`Bool` feature per distinct token ("level") — the classic one-hot /
multi-label indicator ("dummy variable") encoding. Feature
`<prefix><level>` is `true` for a value if `level` is one of that value's
tokens, `false` otherwise.

Levels are collected into a sorted set, so output features always appear
in **alphabetical order** — not first-seen order.

## Parameters

**delimiter** (`std::string_view`, defaults to `"|"`)
    The literal text to split each value on before collecting tokens.
    Must not be empty.

**prefix** (`std::string`, defaults to `""`)
    Text prepended to each level to form its feature name.

## Returns

`pluss::table::DataFrame`
    A new frame with one `Bool` feature per distinct non-empty token,
    sorted alphabetically, named `<prefix><level>`.

## Examples

Example 1. The default `"|"` delimiter.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("red|blue"), pluss::make_note("blue|green"), pluss::make_note("red")});
auto out = slot.nt().indicate();
std::cout << out;
```

### Output

```text
==========================
     | blue   green  red  
--------------------------
0    | true   false  true 
1    | true   true   false
2    | false  false  true 
--------------------------
[3 cs x 3 feat]
```

Three distinct tokens appear — `"blue"`, `"green"`, `"red"` — in
alphabetical feature order.

## Notes

- Output features are sorted alphabetically by token text, not
  first-appearance order.
- An empty `delimiter` throws `"Thread::indicate: delimiter cannot be
  empty"`.
- Requires the slot to be `Note`-kind — a non-`Note` slot throws.

## See Also

- [`SlotNoteAccessor::split`](split.md): the same underlying
  delimiter-split mechanic, producing positional features instead of
  one-hot level features.
- [`SlotNoteAccessor::has`](has.md): test for a single specific
  substring instead of enumerating every token.
- [`Slot::nt`](../slot/nt.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has.html">
    <span class="label">Previous</span>
    <span class="title">has</span>
  </a>
  <a class="next-link" href="length.html">
    <span class="label">Next</span>
    <span class="title">length</span>
  </a>
</div>
