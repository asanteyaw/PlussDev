<!-- pluss-hand-authored -->

# ShelfNoteAccessor::matches

[Back to ShelfNoteAccessor API Reference](../../shelf_note_accessor_api_reference)

## Signature

```cpp
Shelf ShelfNoteAccessor::matches(std::string_view pattern, bool full = false) const
```

## What It Does

Checks each cell of the bound shelf against a regular expression, replacing each
cell with a `Bool`. Unlike [`Shelf::matches()`](../shelf/matches.md) (which always
does a start-anchored, prefix-only search), this accessor method adds a `full`
switch that dispatches between the two related checks:

- **`full = false`** (the default): a **start-anchored search**, not a full-string
  match — reports `true` as soon as `pattern` matches a *prefix* of the cell's text.
  Internally forwards to [`Shelf::matches(pattern)`](../shelf/matches.md).
- **`full = true`**: requires `pattern` to consume the **entire** cell's text.
  Internally forwards to [`Shelf::fullmatch(pattern)`](../shelf/fullmatch.md) —
  identical to calling [`fullmatch()`](fullmatch.md) directly.

Requires the bound shelf to be `Note`-kind.

## Parameters

**pattern** (`std::string_view`)

A regular expression (ECMAScript-flavored, via `std::regex`).

**full** (`bool`, default `false`)

When `true`, require the pattern to match the whole cell (equivalent to calling
[`fullmatch()`](fullmatch.md)); when `false` (the default), a prefix match suffices.

## Returns

`Shelf`
    A new `Bool`-typed `Series` shelf, same length as the bound shelf, holding the
    elementwise match result.

## Examples

Example 1. A pattern matching only part of the string still counts as `true` under
the default `full = false`.

```cpp
using namespace pluss::table;

auto codes = Shelf(pluss::make_note({"SKU-1001", "SKU-42", "SKU-4"}));
auto out = codes.nt().matches("SKU-[0-9]{2}");
std::cout << out;
```

### Output

```text
0  true
1  true
2  false

ptype: Series
```

`"SKU-1001"` matches `true` even though the pattern only asks for exactly two digits
after `SKU-` — the prefix `"SKU-10"` already satisfies it.

Example 2. The same pattern with `full = true` — now `"SKU-1001"` fails, since the
trailing `"01"` means the pattern doesn't consume the whole string.

```cpp
auto out2 = codes.nt().matches("SKU-[0-9]{2}", true);
std::cout << out2;
```

### Output

```text
0  false
1  true
2  false

ptype: Series
```

Identical to calling `codes.nt().fullmatch("SKU-[0-9]{2}")` directly.

Example 3. A non-`Note` shelf throws.

```cpp
auto numbers = Shelf(pluss::make_series({1.0, 2.0, 3.0}));
try {
  auto bad = numbers.nt().matches("1");
} catch (const pluss::Error& e) {
  std::string msg(e.message());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Shelf::matches: expected Note payload
```

## Notes

- `full = true` is exactly [`fullmatch()`](fullmatch.md) — the two are the same
  underlying call, just reached through a bool switch instead of a separate method
  name.
- An invalid regex pattern throws at call time.
- Changes the payload kind from `Note` to `Series` (`Bool`).
- A null cell on input stays null on output.

## See Also

- [`Shelf::matches`](../shelf/matches.md): the direct, non-accessor form, which has
  no `full` parameter and always does the prefix-only check.
- [`ShelfNoteAccessor::fullmatch`](fullmatch.md): the same operation as
  `matches(pattern, true)`, under its own dedicated name.
- [`ShelfNoteAccessor::extract`](extract.md): pull out capture groups instead of
  just a boolean result.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lstrip.html">
    <span class="label">Previous</span>
    <span class="title">lstrip</span>
  </a>
  <a class="next-link" href="normalize.html">
    <span class="label">Next</span>
    <span class="title">normalize</span>
  </a>
</div>
